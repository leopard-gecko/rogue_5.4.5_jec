/*
    state.c - Portable Rogue Save State Code

    Copyright (C) 1999, 2000, 2005 Nicholas J. Kisseberth
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name(s) of the author(s) nor the names of other contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS ``AS IS'' AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/

#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <errno.h>
#include "rogue.h"

/* JEC: reject corrupted combat levels while allowing extra headroom for
 * monsters whose level may exceed the dungeon depth. */
#define JROGUE_MAX_COMBAT_LEVEL (JROGUE_MAX_DUNGEON_LEVEL + 32)

/************************************************************************/
/* Save State Code                                                      */
/************************************************************************/

#define RSID_STATS        0xABCD0001
#define RSID_THING        0xABCD0002
#define RSID_THING_NULL   0xDEAD0002
#define RSID_OBJECT       0xABCD0003
#define RSID_MAGICITEMS   0xABCD0004
#define RSID_KNOWS        0xABCD0005
#define RSID_GUESSES      0xABCD0006
#define RSID_OBJECTLIST   0xABCD0007
#define RSID_BAGOBJECT    0xABCD0008
#define RSID_MONSTERLIST  0xABCD0009
#define RSID_MONSTERSTATS 0xABCD000A
#define RSID_MONSTERS     0xABCD000B
#define RSID_TRAP         0xABCD000C
#define RSID_WINDOW       0xABCD000D
#define RSID_DAEMONS      0xABCD000E
#define RSID_IWEAPS       0xABCD000F
#define RSID_IARMOR       0xABCD0010
#define RSID_SPELLS       0xABCD0011
#define RSID_ILIST        0xABCD0012
#define RSID_HLIST        0xABCD0013
#define RSID_DEATHTYPE    0xABCD0014
#define RSID_CTYPES       0XABCD0015
#define RSID_COORDLIST    0XABCD0016
#define RSID_ROOMS        0XABCD0017

/* Bounds the total number of restored linked-list nodes, including monster
 * inventories.  The limit is deliberately much larger than the 80x24 map;
 * a shared budget prevents corrupted nested list counts from multiplying
 * into excessive allocations.  This does not change the save format. */
#define RS_MAX_SAVED_LIST_NODES 16384
/* Restored stack counts must remain far below signed-int overflow.  This
 * is deliberately larger than any normal or wizard-created stack; it does
 * not change the number of items that may be created during play. */
#define RS_MAX_SAVED_STACK_COUNT 65536
/* Bounds are only for values that can affect arithmetic or array access.
 * The wizard can charge sticks to 10000; leave generous headroom for normal
 * play while rejecting corrupt INT_MIN/INT_MAX-sized item modifiers. */
#define RS_MAX_SAVED_ITEM_MODIFIER 1000000
#define RS_MAX_SAVED_STICK_CHARGES 1000000
#define RS_PACK_LETTERS 26
static int rs_saved_list_nodes_left = RS_MAX_SAVED_LIST_NODES;

/* Saved damage strings are consumed by roll_em() using atoi()/strchr().
 * Validate the *whole* NUL-terminated text before it can become live state:
 *   dice := unsigned 'x' unsigned ( '/' unsigned 'x' unsigned )*
 * Ordinary attacks have positive counts/sides; 0x0 is the original
 * no-damage spelling (also stored as 000x0 and 0x0/0x0).
 * Venus flytrap's original %%%x0 is a deliberate pre-initialization marker.
 * Non-weapon objects can have empty strings (e.g. wizard-created items).
 * The cap keeps dice iterations and per-roll sums bounded.  The flytrap
 * counter is itself capped at JROGUE_MAX_FLYTRAP_HITS, so its valid
 * generated N x 1 attacks remain loadable.  No save format changes. */
#define RS_MAX_SAVED_DICE_TOTAL JROGUE_MAX_FLYTRAP_HITS

static int
rs_read_dice_number(const char **cursor, const char *end, int *number)
{
    const char *p = *cursor;
    int value = 0;
    int digit;

    if (p == end || *p < '0' || *p > '9')
        return FALSE;
    do
    {
        digit = *p - '0';
        if (value > (RS_MAX_SAVED_DICE_TOTAL - digit) / 10)
            return FALSE;
        value = value * 10 + digit;
        p++;
    } while (p < end && *p >= '0' && *p <= '9');

    *cursor = p;
    *number = value;
    return TRUE;
}

static int
rs_saved_dice_ok(const char *s, size_t capacity, int allow_empty,
                 int allow_flytrap_marker)
{
    const char *end = memchr(s, '\0', capacity);
    const char *p = s;
    int ndice, nsides;

    if (end == NULL)
        return FALSE;
    if (end == s)
        return allow_empty;
    if ((size_t)(end - s) == sizeof("%%%x0") - 1 &&
        memcmp(s, "%%%x0", sizeof("%%%x0") - 1) == 0)
        return allow_flytrap_marker;

    for (;;)
    {
        if (!rs_read_dice_number(&p, end, &ndice) ||
            p == end || *p++ != 'x' ||
            !rs_read_dice_number(&p, end, &nsides))
            return FALSE;

        if (ndice == 0)
        {
            if (nsides != 0)
                return FALSE;
        }
        else if (nsides == 0 ||
                 ndice > RS_MAX_SAVED_DICE_TOTAL / nsides)
            return FALSE;

        if (p == end)
            return TRUE;
        if (*p++ != '/' || p == end)
            return FALSE;
    }
}

static int endian = 0x01020304;
#define  big_endian ( *((char *)&endian) == 0x01 )

void
rs_write(FILE *savef, const void *ptr, size_t size)
{
    encwrite(ptr, size, savef);
}

void
rs_read(FILE *savef, void *ptr, size_t size)
{
    encread(ptr, size, savef);
}

void
rs_write_int(FILE *savef, int c)
{
    char bytes[4];
    char *buf = (char *) &c;

    if (big_endian)
    {
        bytes[3] = buf[0];
        bytes[2] = buf[1];
        bytes[1] = buf[2];
        bytes[0] = buf[3];
        buf = bytes;
    }

    rs_write(savef, buf, 4);
}

void
rs_read_int(FILE *savef, int *i)
{
    char bytes[4];
    int input = 0;
    char *buf = (char *)&input;

    rs_read(savef, &input, 4);

    if (encerror())
	return;

    if (big_endian)
    {
        bytes[3] = buf[0];
        bytes[2] = buf[1];
        bytes[1] = buf[2];
        bytes[0] = buf[3];
        buf = bytes;
    }

    *i = *((int *) buf);
}

void
rs_write_uint(FILE *savef, unsigned int c)
{
    char bytes[4];
    char *buf = (char *) &c;

    if (big_endian)
    {
        bytes[3] = buf[0];
        bytes[2] = buf[1];
        bytes[1] = buf[2];
        bytes[0] = buf[3];
        buf = bytes;
    }

    rs_write(savef, buf, 4);
}

void
rs_read_uint(FILE *savef, unsigned int *i)
{
    char bytes[4];
    int input = 0;
    char *buf = (char *)&input;

    rs_read(savef, &input, 4);

    if (encerror())
	return;

    if (big_endian)
    {
        bytes[3] = buf[0];
        bytes[2] = buf[1];
        bytes[1] = buf[2];
        bytes[0] = buf[3];
        buf = bytes;
    }

    *i = *((unsigned int *) buf);
}

void
rs_write_chars(FILE *savef, const char *c, int cnt)
{
    rs_write_int(savef, cnt);
    rs_write(savef, c, cnt);
}

void
rs_read_chars(FILE *savef, char *i, int cnt)
{
    int value = 0;

    /* Reject corrupt lengths before they can become huge size_t values. */
    if (encerror())
        return;
    if (cnt < 0 || (cnt > 0 && i == NULL))
    {
        encseterr(EILSEQ);
        return;
    }

    rs_read_int(savef, &value);
    if (encerror())
        return;
    if (value != cnt)
    {
        encseterr(EILSEQ);
        return;
    }

    if (cnt > 0)
        rs_read(savef, i, (size_t)cnt);
}

/* These four save fields are fixed-size *text*, unlike the binary arrays
 * also handled by rs_read_chars().  A damaged save with no NUL must not
 * reach string functions after restoration.  Keep the on-disk layout and
 * preserve long legacy values whenever they are properly terminated. */
static void
rs_read_fixed_text(FILE *savef, char *text, int capacity)
{
    if (encerror())
        return;
    rs_read_chars(savef, text, capacity);
    if (!encerror() && memchr(text, '\0', (size_t)capacity) == NULL)
        encseterr(EILSEQ);
}

void
rs_write_ints(FILE *savef, int *c, int cnt)
{
    int n = 0;

    rs_write_int(savef, cnt);

    for(n = 0; n < cnt; n++)
        rs_write_int(savef,c[n]);
}

void
rs_read_ints(FILE *savef, int *i, int cnt)
{
    int n, value = 0;

    rs_read_int(savef, &value);
    if (encerror())
        return;

    /* This is a fixed-size array; a shortened save is not a valid variant. */
    if (value != cnt)
    {
        encseterr(EILSEQ);
        return;
    }

    for (n = 0; n < cnt; n++)
    {
        rs_read_int(savef, &i[n]);
        if (encerror())
            return;
    }
}

void
rs_write_marker(FILE *savef, int id)
{
    rs_write_int(savef, id);
}

void
rs_read_marker(FILE *savef, int id)
{
    int nid;

    rs_read_int(savef, &nid);

    if (!encerror() && (id != nid))
	encseterr(EILSEQ);
}

/******************************************************************************/

/*
 * User-entered strings are saved verbatim.  They are treated as UTF-8 bytes
 * and are not translated when a save file is restored in another language.
 */
void
rs_write_string(FILE *savef, const char *s)
{
    int len = 0;

    len = (s == NULL) ? 0 : (int) strlen(s) + 1;

    rs_write_int(savef, len);
    rs_write_chars(savef, s, len);
}

/* The dynamically saved names and labels are user-entered strings.
 * A generous byte limit rejects corrupt lengths without constraining
 * normal UTF-8 input.  It does not change the on-disk save format. */
#define RS_MAX_SAVED_STRING (64 * MAXSTR)

void
rs_read_string(FILE *savef, char *s, int max)
{
    int len = 0;

    rs_read_int(savef, &len);
    if (encerror())
        return;
    if (max < 0 || len < 0 || len > max || (len > 0 && s == NULL))
    {
        encseterr(EILSEQ);
        return;
    }

    rs_read_chars(savef, s, len);
    if (encerror())
        return;

    /* The length includes exactly one trailing NUL byte. */
    if (len > 0 && (s[len - 1] != '\0' ||
                    memchr(s, '\0', (size_t)len) != s + len - 1))
        encseterr(EILSEQ);
}

void
rs_read_new_string(FILE *savef, char **s)
{
    int len = 0;
    char *buf = NULL;

    rs_read_int(savef, &len);
    if (encerror())
        return;
    if (s == NULL || len < 0 || len > RS_MAX_SAVED_STRING)
    {
        encseterr(EILSEQ);
        return;
    }

    if (len > 0)
    {
        buf = malloc((size_t)len);
        if (buf == NULL)
        {
            encseterr(ENOMEM);
            return;
        }
    }

    /* Even NULL strings have a saved zero-length data marker. */
    rs_read_chars(savef, buf, len);
    if (encerror())
    {
        free(buf);
        return;
    }

    if (len > 0 && (buf[len - 1] != '\0' ||
                    memchr(buf, '\0', (size_t)len) != buf + len - 1))
    {
        free(buf);
        encseterr(EILSEQ);
        return;
    }

    *s = buf;
}


static int
rs_string_index(const char *master[], int max, const char *str)
{
    int i;

    for (i = 0; i < max; i++)
        if (str == master[i])
            return i;

    return -1;
}


void
rs_write_string_index(FILE *savef, const char *master[], int max, const char *str)
{
    rs_write_int(savef, rs_string_index(master, max, str));
}

void
rs_read_string_index(FILE *savef, const char *master[], int maxindex, const char **str)
{
    int i;

    rs_read_int(savef, &i);

    if (encerror())
        return;

    /* -1 is the saved NULL value; valid indices are 0..maxindex-1. */
    if (i < -1 || i >= maxindex)
        encseterr(EILSEQ);
    else if (i == -1)
        *str = NULL;
    else
        *str = master[i];
}

void
rs_write_coord(FILE *savef, coord c)
{
    rs_write_int(savef, c.x);
    rs_write_int(savef, c.y);
}

void
rs_read_coord(FILE *savef, coord *c)
{
    coord in;

    rs_read_int(savef,&in.x);
    rs_read_int(savef,&in.y);

    if (!encerror())
    {
        c->x = in.x;
        c->y = in.y;
    }
}

void
rs_write_window(FILE *savef, WINDOW *win)
{
    int row,col,height,width;

    /* Only stdscr is serialized by rs_save_file().  Its curses dimensions
     * can exceed Rogue's logical 80 x (24..32) display on large terminals.
     * Store the exact logical region, matching the size saved in save.c's
     * header.  Preserve the existing marker/dimensions/cells wire format. */
    width = _COLS;
    height = _LINES;
    if (width < NUMCOLS || height < NUMLINES ||
        width > getmaxx(win) || height > getmaxy(win) ||
        width > MAXCOLS || height > MAXLINES)
    {
        encseterr(EILSEQ);
        return;
    }

    rs_write_marker(savef,RSID_WINDOW);
    rs_write_int(savef,height);
    rs_write_int(savef,width);

    for(row=0;row<height;row++)
        for(col=0;col<width;col++)
            rs_write_int(savef, mvwinch(win,row,col));
}

void
rs_read_window(FILE *savef, WINDOW *win)
{
    int row,col,maxlines,maxcols,value,width,height;

    width  = getmaxx(win);
    height = getmaxy(win);

    rs_read_marker(savef, RSID_WINDOW);
    if (encerror())
        return;

    rs_read_int(savef, &maxlines);
    rs_read_int(savef, &maxcols);

    if (encerror())
        return;

    /* Legacy JEC saved the entire physical stdscr, often larger than the
     * 80x32 logical display announced by its header.  Consume ALL of its
     * cells to preserve the serialized format, but draw only the overlap
     * with the current logical region.  A smaller 80x24 terminal can thus
     * restore an old 120x40 save, and vice versa.  Bound the legacy size
     * before looping: these limits admit ordinary large terminals without
     * trusting corrupt dimensions as unbounded iteration counts. */
    if (maxlines < NUMLINES || maxcols < NUMCOLS ||
        maxlines > 512 || maxcols > 1024 ||
        (long long)maxlines * maxcols > 262144)
    {
        encseterr(EILSEQ);
        return;
    }

    /* Do not redraw data outside the current logical game screen, even
     * when stdscr itself covers a larger physical terminal. */
    if (height > _LINES)
        height = _LINES;
    if (width > _COLS)
        width = _COLS;

    for(row = 0; row < maxlines; row++)
        for(col = 0; col < maxcols; col++)
        {
            rs_read_int(savef, &value);
            if (encerror())
                return;

            if ((row < height) && (col < width))
            {
#ifdef _WIN32
                /* Empty cells from some PDCurses saves are NUL rather than
                 * spaces.  Passing them to PDCurses WIDE shows ^@ on screen. */
                if ((value & A_CHARTEXT) == 0)
                    value = (value & ~A_CHARTEXT) | ' ';
#endif
                jrogue_mvwaddch(win,row,col,value);
            }
        }
}

/******************************************************************************/

void *
get_list_item(THING *l, int i)
{
    int cnt;

    for(cnt = 0; l != NULL; cnt++, l = l->l_next)
        if (cnt == i)
            return(l);

    return(NULL);
}

int
find_list_ptr(THING *l, void *ptr)
{
    int cnt;

    for(cnt = 0; l != NULL; cnt++, l = l->l_next)
        if (l == ptr)
            return(cnt);

    return(-1);
}

int
list_size(THING *l)
{
    int cnt;

    for(cnt = 0; l != NULL; cnt++, l = l->l_next)
        ;

    return(cnt);
}

/******************************************************************************/

void
rs_write_stats(FILE *savef, struct stats *s)
{
    rs_write_marker(savef, RSID_STATS);
    rs_write_int(savef, s->s_str);
    rs_write_int(savef, s->s_exp);
    rs_write_int(savef, s->s_lvl);
    rs_write_int(savef, s->s_arm);
    rs_write_int(savef, s->s_hpt);
    rs_write_chars(savef, s->s_dmg, sizeof(s->s_dmg));
    rs_write_int(savef,s->s_maxhp);
}

void
rs_read_stats(FILE *savef, struct stats *s)
{
    rs_read_marker(savef, RSID_STATS);
    if (encerror())
        return;
    rs_read_int(savef, &s->s_str);
    if (encerror())
        return;
    /* roll_em() uses strength as an index into two 32-entry tables.
     * Apply the table bounds to all saved stats (player, monsters and
     * templates), without imposing the player's usual 3..31 range on
     * non-player records. */
    if (s->s_str < 0 || s->s_str >= 32)
    {
        encseterr(EILSEQ);
        return;
    }
    rs_read_int(savef,&s->s_exp);
    rs_read_int(savef,&s->s_lvl);
    rs_read_int(savef,&s->s_arm);
    rs_read_int(savef,&s->s_hpt);
    rs_read_chars(savef,s->s_dmg,sizeof(s->s_dmg));
    if (encerror())
        return;
    /* Besides the NUL terminator, verify the complete dice grammar and
     * bounded values before roll_em() can parse or roll this string. */
    if (!rs_saved_dice_ok(s->s_dmg, sizeof(s->s_dmg), FALSE, TRUE))
    {
        encseterr(EILSEQ);
        return;
    }
    rs_read_int(savef,&s->s_maxhp);
    if (encerror())
        return;
    /* Templates have s_maxhp == 0, whereas living creatures do not.
     * Higher-level validity is checked after identifying the record. */
    if (s->s_exp < 0 || s->s_lvl < 1 ||
        s->s_lvl > JROGUE_MAX_COMBAT_LEVEL ||
        s->s_hpt < 0 || s->s_maxhp < 0)
        encseterr(EILSEQ);
}

void
rs_write_stone_index(FILE *savef, const STONE master[], int max, const char *str)
{
    int i;

    for(i = 0; i < max; i++)
        if (str == master[i].st_name)
	{
            rs_write_int(savef,i);
	    return;
	}

    rs_write_int(savef,-1);
}

void
rs_read_stone_index(FILE *savef, const STONE master[], int maxindex, const char **str)
{
    int i = 0;

    rs_read_int(savef, &i);

    if (encerror())
        return;

    /* -1 is the saved NULL value; valid indices are 0..maxindex-1. */
    if (i < -1 || i >= maxindex)
        encseterr(EILSEQ);
    else if (i == -1)
        *str = NULL;
    else
        *str = master[i].st_name;
}

/*
 * Scroll titles are saved as syllable IDs, not as localized strings.
 * They are rebuilt from the current runtime language when restored.
 */
void
rs_write_scrolls(FILE *savef)
{
    int i, n;

    for(i = 0; i < MAXSCROLLS; i++)
    {
	rs_write_int(savef, scroll_syll_count[i]);
	for (n = 0; n < MAX_SCROLL_SYLLABLES; n++)
	{
	    rs_write_int(savef, scroll_syll_ids[i][n]);
	    rs_write_int(savef, scroll_syll_breaks[i][n]);
	}
    }
}

void
rs_read_scrolls(FILE *savef)
{
    int i, n;

    for(i = 0; i < MAXSCROLLS; i++)
    {
	rs_read_int(savef, &scroll_syll_count[i]);
	if (!encerror() &&
	    (scroll_syll_count[i] < 0 ||
	     scroll_syll_count[i] > MAX_SCROLL_SYLLABLES))
	    encseterr(EILSEQ);
	for (n = 0; n < MAX_SCROLL_SYLLABLES; n++)
	{
	    rs_read_int(savef, &scroll_syll_ids[i][n]);
	    rs_read_int(savef, &scroll_syll_breaks[i][n]);
	}
    }
    if (!encerror())
	jrogue_rebuild_scroll_names();
}

void
rs_write_potions(FILE *savef)
{
    int i;

    for(i = 0; i < MAXPOTIONS; i++)
        rs_write_string_index(savef, rainbow, cNCOLORS, p_colors[i]);
}

void
rs_read_potions(FILE *savef)
{
    int i;

    for(i = 0; i < MAXPOTIONS; i++)
        rs_read_string_index(savef, rainbow, cNCOLORS, &p_colors[i]);
}

void
rs_write_rings(FILE *savef)
{
    int i;

    for(i = 0; i < MAXRINGS; i++)
        rs_write_stone_index(savef, stones, cNSTONES, r_stones[i]);
}

void
rs_read_rings(FILE *savef)
{
    int i;

    for(i = 0; i < MAXRINGS; i++)
        rs_read_stone_index(savef, stones, cNSTONES, &r_stones[i]);
}

void
rs_write_sticks(FILE *savef)
{
    int i;

    for (i = 0; i < MAXSTICKS; i++)
    {
        if (rs_string_index(wood, cNWOOD, ws_made[i]) >= 0)
        {
            rs_write_int(savef,0);
            rs_write_string_index(savef, wood, cNWOOD, ws_made[i]);
        }
        else
        {
            rs_write_int(savef,1);
            rs_write_string_index(savef, metal, cNMETAL, ws_made[i]);
        }
    }
}

void
rs_read_sticks(FILE *savef)
{
    int i = 0, list = 0;

    for(i = 0; i < MAXSTICKS; i++)
    {
        rs_read_int(savef,&list);

        if (list == 0)
        {
            rs_read_string_index(savef, wood, cNWOOD, &ws_made[i]);
            ws_type[i] = JSTR("つえ", "staff");
        }
        else
        {
            rs_read_string_index(savef, metal, cNMETAL, &ws_made[i]);
            ws_type[i] = JSTR("魔法の棒", "wand");
        }
    }
}

void
rs_write_daemons(FILE *savef, struct delayed_action *dlist, int cnt)
{
    int i = 0;
    int func = 0;

    rs_write_marker(savef, RSID_DAEMONS);
    rs_write_int(savef, cnt);

    for(i = 0; i < cnt; i++)
    {
        if (dlist[i].d_func == rollwand)
            func = 1;
        else if (dlist[i].d_func == doctor)
            func = 2;
        else if (dlist[i].d_func == stomach)
            func = 3;
        else if (dlist[i].d_func == runners)
            func = 4;
        else if (dlist[i].d_func == swander)
            func = 5;
        else if (dlist[i].d_func == nohaste)
            func = 6;
        else if (dlist[i].d_func == unconfuse)
            func = 7;
        else if (dlist[i].d_func == unsee)
            func = 8;
        else if (dlist[i].d_func == sight)
            func = 9;
        else if (dlist[i].d_func == turn_see_off)
            func = 10;
        /* Keep IDs 1-10 unchanged for existing save files. */
        else if (dlist[i].d_func == visuals)
            func = 11;
        else if (dlist[i].d_func == come_down)
            func = 12;
        else if (dlist[i].d_func == land)
            func = 13;
        else if (dlist[i].d_func == NULL)
            func = 0;
        else
            func = -1;

        rs_write_int(savef, dlist[i].d_type);
        rs_write_int(savef, func);
        rs_write_int(savef, dlist[i].d_arg);
        rs_write_int(savef, dlist[i].d_time);
    }
}

void
rs_read_daemons(FILE *savef, struct delayed_action *dlist, int cnt)
{
    int i = 0;
    int func = 0;
    int value = 0;
    int monster_see_slot = -1;

    rs_read_marker(savef, RSID_DAEMONS);
    if (encerror())
        return;
    rs_read_int(savef, &value);
    if (encerror())
        return;

    /* The writer always stores all 20 slots, including empty ones. */
    if (value != cnt)
    {
        encseterr(EILSEQ);
        return;
    }

    for(i=0; i < cnt; i++)
    {
        func = 0;
        rs_read_int(savef, &dlist[i].d_type);
        rs_read_int(savef, &func);
        rs_read_int(savef, &dlist[i].d_arg);
        rs_read_int(savef, &dlist[i].d_time);

	if (encerror())
	    return;

        switch(func)
        {
            case 1: dlist[i].d_func = rollwand;
                    break;
            case 2: dlist[i].d_func = doctor;
                    break;
            case 3: dlist[i].d_func = stomach;
                    break;
            case 4: dlist[i].d_func = runners;
                    break;
            case 5: dlist[i].d_func = swander;
                    break;
            case 6: dlist[i].d_func = nohaste;
                    break;
            case 7: dlist[i].d_func = unconfuse;
                    break;
            case 8: dlist[i].d_func = unsee;
                    break;
            case 9: dlist[i].d_func = sight;
                    break;
            case 10: dlist[i].d_func = turn_see_off;
                     break;
            case 11: dlist[i].d_func = visuals;
                     break;
            case 12: dlist[i].d_func = come_down;
                     break;
            case 13: dlist[i].d_func = land;
                     break;
            default:dlist[i].d_func = NULL;
                    break;
        }

	if (dlist[i].d_func == NULL)
	{
	    dlist[i].d_type = 0;
	    dlist[i].d_arg = 0;
	    dlist[i].d_time = 0;
	}
    }

    /* Older saves can contain multiple turn_see_off fuses from repeated
     * monster-detection potions.  Preserve the latest expiry, remove the
     * earlier duplicates, and free their slots.  No RNG is involved. */
    for (i = 0; i < cnt; i++)
    {
        if (dlist[i].d_type == EMPTY ||
            dlist[i].d_func != turn_see_off || dlist[i].d_time <= 0)
            continue;
        if (monster_see_slot < 0)
            monster_see_slot = i;
        else
        {
            if (dlist[i].d_time > dlist[monster_see_slot].d_time)
                dlist[monster_see_slot].d_time = dlist[i].d_time;
            dlist[i].d_type = EMPTY;
            dlist[i].d_func = NULL;
            dlist[i].d_arg = 0;
            dlist[i].d_time = 0;
        }
    }
}

/*
 * Object names themselves are not saved.  They are regenerated from the
 * internal object IDs and the current runtime language.  Only player-entered
 * guesses/call names are saved as UTF-8 strings.
 */
void
rs_write_obj_info(FILE *savef, struct obj_info *i, int cnt)
{
    int n;

    rs_write_marker(savef, RSID_MAGICITEMS);
    rs_write_int(savef, cnt);

    for(n = 0; n < cnt; n++)
    {
        /* oi_name is constant, defined at compile time in all cases */
        rs_write_int(savef,i[n].oi_prob);
        rs_write_int(savef,i[n].oi_worth);
        rs_write_string(savef,i[n].oi_guess);
        rs_write_int(savef,i[n].oi_know);
    }
}

void
rs_read_obj_info(FILE *savef, struct obj_info *mi, int cnt)
{
    int n;
    int value = 0;

    rs_read_marker(savef, RSID_MAGICITEMS);
    if (encerror())
        return;
    rs_read_int(savef, &value);
    if (encerror())
        return;

    /* All entries are always saved, even when they are unidentified. */
    if (value != cnt)
    {
        encseterr(EILSEQ);
        return;
    }

    for (n = 0; n < cnt; n++)
    {
        /* oi_name is const, defined at compile time in all cases. */
        rs_read_int(savef, &mi[n].oi_prob);
        if (encerror())
            return;
        rs_read_int(savef, &mi[n].oi_worth);
        if (encerror())
            return;
        rs_read_new_string(savef, &mi[n].oi_guess);
        if (encerror())
            return;
        rs_read_int(savef, &mi[n].oi_know);
        if (encerror())
            return;
    }
}

void
rs_write_room(FILE *savef, struct room *r)
{
    rs_write_coord(savef, r->r_pos);
    rs_write_coord(savef, r->r_max);
    rs_write_coord(savef, r->r_gold);
    rs_write_int(savef,   r->r_goldval);
    rs_write_int(savef, r->r_flags);
    rs_write_int(savef, r->r_nexits);
    rs_write_coord(savef, r->r_exit[0]);
    rs_write_coord(savef, r->r_exit[1]);
    rs_write_coord(savef, r->r_exit[2]);
    rs_write_coord(savef, r->r_exit[3]);
    rs_write_coord(savef, r->r_exit[4]);
    rs_write_coord(savef, r->r_exit[5]);
    rs_write_coord(savef, r->r_exit[6]);
    rs_write_coord(savef, r->r_exit[7]);
    rs_write_coord(savef, r->r_exit[8]);
    rs_write_coord(savef, r->r_exit[9]);
    rs_write_coord(savef, r->r_exit[10]);
    rs_write_coord(savef, r->r_exit[11]);
}

void
rs_read_room(FILE *savef, struct room *r)
{
    int n;

    rs_read_coord(savef, &r->r_pos);
    rs_read_coord(savef, &r->r_max);
    rs_read_coord(savef, &r->r_gold);
    rs_read_int(savef, &r->r_goldval);
    rs_read_int(savef, &r->r_flags);
    rs_read_int(savef, &r->r_nexits);
    if (encerror())
        return;

    /* chase.c uses r_nexits as a bound for the 12-element r_exit[]. */
    if (r->r_nexits < 0 ||
        r->r_nexits > (int)(sizeof(r->r_exit) / sizeof(r->r_exit[0])))
    {
        encseterr(EILSEQ);
        return;
    }

    /* The file contains all exit slots, not just r_nexits entries. */
    for (n = 0; n < (int)(sizeof(r->r_exit) / sizeof(r->r_exit[0])); n++)
    {
        rs_read_coord(savef, &r->r_exit[n]);
        if (encerror())
            return;
    }
}

void
rs_write_rooms(FILE *savef, struct room r[], int cnt)
{
    int n = 0;

    rs_write_int(savef, cnt);

    for(n = 0; n < cnt; n++)
        rs_write_room(savef, &r[n]);
}

void
rs_read_rooms(FILE *savef, struct room *r, int cnt)
{
    int value = 0, n;

    rs_read_int(savef, &value);
    if (encerror())
        return;
    if (value != cnt)
    {
        encseterr(EILSEQ);
        return;
    }

    for (n = 0; n < cnt; n++)
    {
        rs_read_room(savef, &r[n]);
        if (encerror())
            return;
    }
}

void
rs_write_room_reference(FILE *savef, struct room *rp)
{
    int i, room = -1;

    for (i = 0; i < MAXROOMS; i++)
        if (&rooms[i] == rp)
            room = i;

    rs_write_int(savef, room);
}

void
rs_read_room_reference(FILE *savef, struct room **rp)
{
    int i;

    rs_read_int(savef, &i);

    if (encerror())
        return;

    /* Older saves encode a passage as -1: do not form &rooms[-1].
     * The correct room or passage is reconstructed from the saved map
     * and position after rs_restore_file() has read both arrays. */
    if (i == -1)
        *rp = NULL;
    else if (i >= 0 && i < MAXROOMS)
        *rp = &rooms[i];
    else
        encseterr(EILSEQ);
}

void
rs_write_monsters(FILE *savef, struct monster *m, int cnt)
{
    int n;

    rs_write_marker(savef, RSID_MONSTERS);
    rs_write_int(savef, cnt);

    for(n=0;n<cnt;n++)
        rs_write_stats(savef, &m[n].m_stats);
}

void
rs_read_monsters(FILE *savef, struct monster *m, int cnt)
{
    int value = 0, n;

    rs_read_marker(savef, RSID_MONSTERS);
    if (encerror())
        return;
    rs_read_int(savef, &value);
    if (encerror())
        return;
    if (value != cnt)
    {
        encseterr(EILSEQ);
        return;
    }

    for (n = 0; n < cnt; n++)
    {
        rs_read_stats(savef, &m[n].m_stats);
        if (encerror())
            return;
        if (n != 'F' - 'A' &&
            strcmp(m[n].m_stats.s_dmg, "%%%x0") == 0)
        {
            encseterr(EILSEQ);
            return;
        }
    }
}

void
rs_write_object(FILE *savef, THING *o)
{
    rs_write_marker(savef, RSID_OBJECT);
    rs_write_int(savef, o->_o._o_type);
    rs_write_coord(savef, o->_o._o_pos);
    rs_write_int(savef, o->_o._o_launch);
    rs_write_int(savef, o->_o._o_packch);
    rs_write_chars(savef, o->_o._o_damage, sizeof(o->_o._o_damage));
    rs_write_chars(savef, o->_o._o_hurldmg, sizeof(o->_o._o_hurldmg));
    rs_write_int(savef, o->_o._o_count);
    rs_write_int(savef, o->_o._o_which);
    rs_write_int(savef, o->_o._o_hplus);
    rs_write_int(savef, o->_o._o_dplus);
    rs_write_int(savef, o->_o._o_arm);
    rs_write_int(savef, o->_o._o_flags);
    rs_write_int(savef, o->_o._o_group);
    rs_write_string(savef, o->_o._o_label);
}

/* Validate the meaning of shared object fields according to the item type.
 * In particular, o_arm is a defense value, a ring bonus, a wand charge count,
 * or a gold amount, depending on o_type.  Other object types leave o_arm as
 * an unused initial value (typically 0 or 11), which is not constrained here
 * to avoid needlessly rejecting older saves.
 *
 * These checks only reject malformed saved values; they do not change item
 * generation, enchantment, the RNG, or the on-disk save layout. */
static int
rs_object_attributes_ok(const THING *o)
{
    int allowed_flags = ISCURSED | ISKNOW | ISFOUND;

    if (o->o_type == ARMOR)
        allowed_flags |= ISPROT;
    if (o->o_type == WEAPON)
        allowed_flags |= ISMISL | ISMANY;
    if (o->o_type == GOLD)
        allowed_flags |= ISMANY;
    if ((o->o_flags & ~allowed_flags) != 0)
        return FALSE;

    /* Only a weapon uses hit/damage enchantments and a launcher reference. */
    if (o->o_type != WEAPON &&
        (o->o_hplus != 0 || o->o_dplus != 0 || o->o_launch != 0))
        return FALSE;

    switch (o->o_type)
    {
        case WEAPON:
            if (o->o_hplus < -RS_MAX_SAVED_ITEM_MODIFIER ||
                o->o_hplus > RS_MAX_SAVED_ITEM_MODIFIER ||
                o->o_dplus < -RS_MAX_SAVED_ITEM_MODIFIER ||
                o->o_dplus > RS_MAX_SAVED_ITEM_MODIFIER)
                return FALSE;
            /* The arrow is launched with a bow; other saved weapons use -1.
             * Dragon-breath's FLAME is an ephemeral weapon, never saved. */
            if (o->o_launch != (o->o_which == ARROW ? BOW : -1))
                return FALSE;
            /* 1 is reserved for gold; zero means an ungrouped weapon. */
            return o->o_group != 1;

        case ARMOR:
            return o->o_group == 0 &&
                o->o_arm >= -RS_MAX_SAVED_ITEM_MODIFIER &&
                o->o_arm <= RS_MAX_SAVED_ITEM_MODIFIER;

        case RING:
            /* Only four ring types use o_arm as a bonus.  The others
             * normally hold 0 or 11, but the wizard '~' command can set
             * o_arm to 10000 even for a non-stick item.  Do not reject
             * that otherwise valid wizard save. */
            return o->o_group == 0 &&
                o->o_arm >= -RS_MAX_SAVED_ITEM_MODIFIER &&
                o->o_arm <= RS_MAX_SAVED_ITEM_MODIFIER;

        case STICK:
            /* Normally 0..19; the wizard's '~' command sets 10000. */
            return o->o_group == 0 &&
                o->o_charges >= 0 &&
                o->o_charges <= RS_MAX_SAVED_STICK_CHARGES;

        case GOLD:
            /* Old wizard saves may contain zero-valued gold. */
            return o->o_goldval >= 0 &&
                (o->o_group == 0 || o->o_group == 1);

        case POTION:
        case SCROLL:
        case FOOD:
        case AMULET:
            return o->o_group == 0;

        default:
            return FALSE;
    }
}

void
rs_read_object(FILE *savef, THING *o)
{
    int max_which;

    rs_read_marker(savef, RSID_OBJECT);
    rs_read_int(savef, &o->_o._o_type);
    rs_read_coord(savef, &o->_o._o_pos);
    rs_read_int(savef, &o->_o._o_launch);
    rs_read_int(savef, &o->_o._o_packch);
    if (encerror())
        return;
    /* A floor object (including a dropped former pack item) or a monster's
     * possession may have no pack letter or retain a former a-z letter.
     * Player-pack-specific checks are performed after restoring that list. */
    if (o->_o._o_packch != 0 &&
        (o->_o._o_packch < 'a' || o->_o._o_packch > 'z'))
    {
        encseterr(EILSEQ);
        return;
    }
    rs_read_chars(savef, o->_o._o_damage, sizeof(o->_o._o_damage));
    if (encerror())
        return;
    rs_read_chars(savef, o->_o._o_hurldmg, sizeof(o->_o._o_hurldmg));
    if (encerror())
        return;
    /* A saved weapon needs two valid attack expressions.  Non-weapons
     * may have empty fields; all nonempty values must still be safe dice. */
    if (!rs_saved_dice_ok(o->_o._o_damage, sizeof(o->_o._o_damage),
                          o->o_type != WEAPON, FALSE) ||
        !rs_saved_dice_ok(o->_o._o_hurldmg, sizeof(o->_o._o_hurldmg),
                          o->o_type != WEAPON, FALSE))
    {
        encseterr(EILSEQ);
        return;
    }
    rs_read_int(savef, &o->_o._o_count);
    if (encerror())
        return;
    /* Normal gold and the Amulet start with count zero, while wizard-created
     * versions can have count one.  Other objects must be present in positive
     * quantities.  Armor, rings and sticks cannot form stacks. */
    if (o->_o._o_count < 0 ||
        o->_o._o_count > RS_MAX_SAVED_STACK_COUNT)
    {
        encseterr(EILSEQ);
        return;
    }
    switch (o->_o._o_type)
    {
        case GOLD:
        case AMULET:
            if (o->_o._o_count > 1)
                encseterr(EILSEQ);
            break;
        case ARMOR:
        case RING:
        case STICK:
            if (o->_o._o_count != 1)
                encseterr(EILSEQ);
            break;
        case POTION:
        case SCROLL:
        case FOOD:
        case WEAPON:
            if (o->_o._o_count == 0)
                encseterr(EILSEQ);
            break;
        default:
            encseterr(EILSEQ);
            break;
    }
    if (encerror())
        return;
    rs_read_int(savef, &o->_o._o_which);
    if (encerror())
        return;

    /* Saved objects must have an index into the table for their actual
     * type.  FLAME is an ephemeral weapon for dragon breath, not a saved
     * weapon entry.  Gold and the Amulet use index zero. */
    switch (o->_o._o_type)
    {
        case POTION: max_which = MAXPOTIONS; break;
        case SCROLL: max_which = MAXSCROLLS; break;
        case FOOD:   max_which = 2; break;
        case WEAPON: max_which = MAXWEAPONS; break;
        case ARMOR:  max_which = MAXARMORS; break;
        case RING:   max_which = MAXRINGS; break;
        case STICK:  max_which = MAXSTICKS; break;
        case GOLD:
        case AMULET: max_which = 1; break;
        default:
            encseterr(EILSEQ);
            return;
    }
    if (o->_o._o_which < 0 || o->_o._o_which >= max_which)
    {
        encseterr(EILSEQ);
        return;
    }
    rs_read_int(savef, &o->_o._o_hplus);
    rs_read_int(savef, &o->_o._o_dplus);
    rs_read_int(savef, &o->_o._o_arm);
    rs_read_int(savef, &o->_o._o_flags);
    rs_read_int(savef, &o->_o._o_group);
    if (encerror())
        return;
    if (o->_o._o_group < 0 || !rs_object_attributes_ok(o))
    {
        encseterr(EILSEQ);
        return;
    }
    rs_read_new_string(savef, &o->_o._o_label);
}

void
rs_write_object_list(FILE *savef, THING *l)
{
    rs_write_marker(savef, RSID_OBJECTLIST);
    rs_write_int(savef, list_size(l));

    for( ;l != NULL; l = l->l_next)
        rs_write_object(savef, l);
}

void
rs_read_object_list(FILE *savef, THING **list)
{
    int i, cnt = 0;
    THING *l = NULL, *previous = NULL, *head = NULL;

    if (encerror())
        return;
    rs_read_marker(savef, RSID_OBJECTLIST);
    rs_read_int(savef, &cnt);

    if (encerror())
        return;
    if (list == NULL || cnt < 0 || cnt > rs_saved_list_nodes_left)
    {
        encseterr(EILSEQ);
        return;
    }
    rs_saved_list_nodes_left -= cnt;

    for (i = 0; i < cnt; i++)
    {
        l = new_item();
        if (l == NULL)
        {
            encseterr(ENOMEM);
            break;
        }

        l->l_prev = previous;
        if (previous != NULL)
            previous->l_next = l;
        else
            head = l;
        previous = l;

        rs_read_object(savef, l);
        if (encerror())
            break;
    }

    if (encerror())
    {
        free_object_list(head);
        return;
    }

    *list = head;
}

void
rs_write_object_reference(FILE *savef, THING *list, THING *item)
{
    int i;

    i = find_list_ptr(list, item);

    rs_write_int(savef, i);
}

void
rs_read_object_reference(FILE *savef, THING *list, THING **item)
{
    int i;

    rs_read_int(savef, &i);

    if (!encerror())
	*item = get_list_item(list,i);
    else
	*item = NULL;
}

int
find_room_coord(const struct room *rmlist,const coord *c, int n)
{
    int i = 0;

    for(i = 0; i < n; i++)
        if(&rmlist[i].r_gold == c)
            return(i);

    return(-1);
}

int
find_thing_coord(THING *monlist, const coord *c)
{
    THING *mitem;
    THING *tp;
    int i = 0;

    for(mitem = monlist; mitem != NULL; mitem = mitem->l_next)
    {
        tp = mitem;

        if (c == &tp->t_pos)
            return(i);

        i++;
    }

    return(-1);
}

int
find_object_coord(THING *objlist, const coord *c)
{
    THING *oitem;
    THING *obj;
    int i = 0;

    for(oitem = objlist; oitem != NULL; oitem = oitem->l_next)
    {
        obj = oitem;

        if (c == &obj->o_pos)
            return(i);

        i++;
    }

    return(-1);
}

void
rs_write_thing(FILE *savef, THING *t)
{
    int i = -1;

    rs_write_marker(savef, RSID_THING);

    if (t == NULL)
    {
        rs_write_int(savef, 0);
        return;
    }

    rs_write_int(savef, 1);
    rs_write_coord(savef, t->_t._t_pos);
    rs_write_int(savef, t->_t._t_turn);
    rs_write_int(savef, t->_t._t_type);
    rs_write_int(savef, t->_t._t_disguise);
    rs_write_int(savef, t->_t._t_oldch);

    /*
        t_dest can be:
        0,0: NULL
        0,1: location of hero
        1,i: location of a thing (monster)
        2,i: location of an object
        3,i: location of gold in a room

        We need to remember what we are chasing rather than
        the current location of what we are chasing.
    */

    if (t->t_dest == &hero)
    {
        rs_write_int(savef,0);
        rs_write_int(savef,1);
    }
    else if (t->t_dest != NULL)
    {
        i = find_thing_coord(mlist, t->t_dest);

        if (i >=0 )
        {
            rs_write_int(savef,1);
            rs_write_int(savef,i);
        }
        else
        {
            i = find_object_coord(lvl_obj, t->t_dest);

            if (i >= 0)
            {
                rs_write_int(savef,2);
                rs_write_int(savef,i);
            }
            else
            {
                i = find_room_coord(rooms, t->t_dest, MAXROOMS);

                if (i >= 0)
                {
                    rs_write_int(savef,3);
                    rs_write_int(savef,i);
                }
                else
                {
                    rs_write_int(savef, 0);
                    rs_write_int(savef,1); /* chase the hero anyway */
                }
            }
        }
    }
    else
    {
        rs_write_int(savef,0);
        rs_write_int(savef,0);
    }

    rs_write_int(savef, t->_t._t_flags);
    rs_write_stats(savef, &t->_t._t_stats);
    rs_write_room_reference(savef, t->_t._t_room);
    rs_write_object_list(savef, t->_t._t_pack);
}

void
rs_read_thing(FILE *savef, THING *t)
{
    int listid = 0, index = -1;
    THING *item;

    rs_read_marker(savef, RSID_THING);
    rs_read_int(savef, &index);

    if (encerror())
	return;

    if (index == 0)
        return;

    rs_read_coord(savef,&t->_t._t_pos);
    rs_read_int(savef,&t->_t._t_turn);
    rs_read_int(savef,&t->_t._t_type);
    if (encerror())
        return;
    /* The hero normally has t_type == 0.  Only saved monsters use
     * t_type as an index into monsters[t_type - 'A'].  Do not apply
     * the monster restriction to the hero's saved record. */
    if (t != &player &&
        (t->_t._t_type < 'A' || t->_t._t_type > 'Z'))
    {
        encseterr(EILSEQ);
        return;
    }
    rs_read_int(savef,&t->_t._t_disguise);
    rs_read_int(savef,&t->_t._t_oldch);

    /*
        t_dest can be (listid,index):
        0,0: NULL
        0,1: location of hero
        1,i: location of a thing (monster)
        2,i: location of an object
        3,i: location of gold in a room

        We need to remember what we are chasing rather than
        the current location of what we are chasing.
    */

    rs_read_int(savef, &listid);
    rs_read_int(savef, &index);
    t->_t._t_reserved = -1;

    if (encerror())
	return;

    if (listid == 0) /* hero or NULL */
    {
        if (index == 1)
            t->_t._t_dest = &hero;
        else
            t->_t._t_dest = NULL;
    }
    else if (listid == 1) /* monster/thing */
    {
        t->_t._t_dest     = NULL;
        t->_t._t_reserved = index;
    }
    else if (listid == 2) /* object */
    {
        THING *obj;

        item = get_list_item(lvl_obj, index);

        if (item != NULL)
        {
            obj = item;
            t->_t._t_dest = &obj->o_pos;
        }
    }
    else if (listid == 3) /* gold */
    {
        /* Never form a room pointer from a corrupt saved gold index. */
        if (index < 0 || index >= MAXROOMS)
        {
            encseterr(EILSEQ);
            return;
        }
        t->_t._t_dest = &rooms[index].r_gold;
    }
    else
        t->_t._t_dest = NULL;

    rs_read_int(savef,&t->_t._t_flags);
    rs_read_stats(savef,&t->_t._t_stats);
    if (encerror())
        return;
    if (t->t_type != 'F' && strcmp(t->t_stats.s_dmg, "%%%x0") == 0)
    {
        encseterr(EILSEQ);
        return;
    }
    /* Only the player's experience level is limited to 21; deep
     * dungeon monsters can have levels above the experience table. */
    if (t == &player)
    {
        if (t->t_stats.s_lvl > JROGUE_MAX_EXP_LEVEL ||
            t->t_stats.s_hpt < 1 ||
            t->t_stats.s_maxhp < t->t_stats.s_hpt)
        {
            encseterr(EILSEQ);
            return;
        }
    }
    else if (t->t_stats.s_hpt < 1 ||
             t->t_stats.s_maxhp < t->t_stats.s_hpt)
    {
        encseterr(EILSEQ);
        return;
    }
    rs_read_room_reference(savef, &t->_t._t_room);
    rs_read_object_list(savef,&t->_t._t_pack);
}

void
rs_fix_thing(THING *t)
{
    THING *item;
    THING *tp;

    if (t->t_reserved < 0)
        return;

    item = get_list_item(mlist,t->t_reserved);

    if (item != NULL)
    {
        tp = item;
        t->t_dest = &tp->t_pos;
    }
}

void
rs_write_thing_list(FILE *savef, THING *l)
{
    int cnt = 0;

    rs_write_marker(savef, RSID_MONSTERLIST);

    cnt = list_size(l);

    rs_write_int(savef, cnt);

    if (cnt < 1)
        return;

    while (l != NULL) {
        rs_write_thing(savef, l);
        l = l->l_next;
    }
}

/* Each monster owns an object list; release it before discarding the
 * monster when a later element in the saved monster list is corrupt. */
static void
rs_free_saved_thing_list(THING *head)
{
    THING *next_item;

    while (head != NULL)
    {
        next_item = head->l_next;
        free_object_list(head->t_pack);
        discard(head);
        head = next_item;
    }
}

void
rs_read_thing_list(FILE *savef, THING **list)
{
    int i, cnt = 0;
    THING *l = NULL, *previous = NULL, *head = NULL;

    if (encerror())
        return;
    rs_read_marker(savef, RSID_MONSTERLIST);
    rs_read_int(savef, &cnt);

    if (encerror())
        return;
    if (list == NULL || cnt < 0 || cnt > rs_saved_list_nodes_left)
    {
        encseterr(EILSEQ);
        return;
    }
    rs_saved_list_nodes_left -= cnt;

    for (i = 0; i < cnt; i++)
    {
        l = new_item();
        if (l == NULL)
        {
            encseterr(ENOMEM);
            break;
        }

        l->l_prev = previous;
        if (previous != NULL)
            previous->l_next = l;
        else
            head = l;
        previous = l;

        rs_read_thing(savef, l);
        if (encerror())
            break;
    }

    if (encerror())
    {
        rs_free_saved_thing_list(head);
        return;
    }

    *list = head;
}

void
rs_fix_thing_list(THING *list)
{
    THING *item;

    for(item = list; item != NULL; item = item->l_next)
        rs_fix_thing(item);
}

void
rs_write_thing_reference(FILE *savef, THING *list, THING *item)
{
    int i;

    if (item == NULL)
        rs_write_int(savef,-1);
    else
    {
        i = find_list_ptr(list, item);

        rs_write_int(savef, i);
    }
}

void
rs_read_thing_reference(FILE *savef, THING *list, THING **item)
{
    int i;

    rs_read_int(savef, &i);

    if (encerror())
	return;

    if (i == -1)
        *item = NULL;
    else
        *item = get_list_item(list,i);

}

void
rs_write_thing_references(FILE *savef, THING *list, THING *items[], int cnt)
{
    int i;

    for(i = 0; i < cnt; i++)
        rs_write_thing_reference(savef,list,items[i]);
}

void
rs_read_thing_references(FILE *savef, THING *list, THING *items[], int cnt)
{
    int i;

    for(i = 0; i < cnt; i++)
        rs_read_thing_reference(savef,list,&items[i]);
}

void
rs_write_places(FILE *savef, PLACE *p, int cnt)
{
    int i = 0;

    for(i = 0; i < cnt; i++)
    {
        rs_write_int(savef, p[i].p_ch);
        rs_write_int(savef, p[i].p_flags);
        rs_write_thing_reference(savef, mlist, p[i].p_monst);
    }
}

void
rs_read_places(FILE *savef, PLACE *p, int cnt)
{
    int i = 0;

    for(i = 0; i < cnt; i++)
    {
        rs_read_int(savef,&p[i].p_ch);
        rs_read_int(savef,&p[i].p_flags);
        rs_read_thing_reference(savef, mlist, &p[i].p_monst);
    }
}

/* The on-disk pack occupancy bitmap stores integer booleans. */
static void
rs_validate_pack_used_flags(void)
{
    int i;

    for (i = 0; i < RS_PACK_LETTERS; i++)
        if (pack_used[i] != FALSE && pack_used[i] != TRUE)
        {
            encseterr(EILSEQ);
            return;
        }
}

/* Verify the letters actually owned by the player's pack against the
 * saved occupancy bitmap.  Floor objects and monster possessions do not
 * own player pack letters, even if they still remember an old character. */
static void
rs_validate_player_pack_letters(void)
{
    int used[RS_PACK_LETTERS] = { 0 };
    int i, occupied = 0;
    THING *obj;

    for (obj = player.t_pack; obj != NULL; obj = obj->l_next)
    {
        if (obj->o_packch < 'a' || obj->o_packch > 'z')
        {
            encseterr(EILSEQ);
            return;
        }
        i = obj->o_packch - 'a';
        if (used[i])
        {
            encseterr(EILSEQ);
            return;
        }
        used[i] = 1;
        occupied++;
    }

    /* More pack nodes than slots, or stale bitmap bits, could cause
     * pack_char() to walk beyond pack_used[] on a later pickup. */
    if (occupied > MAXPACK)
    {
        encseterr(EILSEQ);
        return;
    }
    for (i = 0; i < RS_PACK_LETTERS; i++)
        if (pack_used[i] != used[i])
        {
            encseterr(EILSEQ);
            return;
        }
}

int
rs_save_file(FILE *savef)
{
    int i;

    encclearerr();

    rs_write_int(savef, noscore);
    rs_write_int(savef, seenstairs);
    rs_write_int(savef, amulet);
    rs_write_int(savef, fight_flush);
    rs_write_int(savef, jump);
    rs_write_int(savef, passgo);
    rs_write_int(savef, see_floor);
    rs_write_int(savef, terse);
    rs_write_int(savef, tombstone);

    i = 0;
    if (idscr_md)	i |= 1;
    if (color_mode)	i |= 2;
    if (show_cursor)	i |= 4;
    rs_write_int(savef, i);

    rs_write_ints(savef, pack_used, RS_PACK_LETTERS);
    rs_write_chars(savef, file_name, MAXSTR);
    rs_write_chars(savef, huh, MAXSTR);
    rs_write_potions(savef);
    rs_write_rings(savef);
    rs_write_scrolls(savef);
    rs_write_chars(savef, whoami, MAXSTR);
    rs_write_sticks(savef);
    rs_write_chars(savef, fruit, MAXSTR);
    rs_write_int(savef,n_objs);
    rs_write_int(savef, ntraps);
    rs_write_int(savef, hungry_state);
    rs_write_int(savef, inpack);
    rs_write_int(savef, inv_type);
    rs_write_int(savef, level);
    rs_write_int(savef, max_level);
    rs_write_int(savef, no_food);
    rs_write_int(savef, food_left);
    rs_write_int(savef, lastscore);
    rs_write_int(savef, no_move);
    rs_write_int(savef, purse);
    rs_write_int(savef, quiet);
    rs_write_int(savef, vf_hit);
    rs_write_uint(savef, seed);
    rs_write_coord(savef, stairs);
    rs_write_thing(savef, &player);
    rs_write_object_reference(savef, player.t_pack, cur_armor);
    rs_write_object_reference(savef, player.t_pack, cur_ring[0]);
    rs_write_object_reference(savef, player.t_pack, cur_ring[1]);
    rs_write_object_reference(savef, player.t_pack, cur_weapon);
    rs_write_object_list(savef, lvl_obj);
    rs_write_thing_list(savef, mlist);
    rs_write_places(savef,places,MAXLINES*MAXCOLS);
    rs_write_stats(savef,&max_stats);
    rs_write_rooms(savef, rooms, MAXROOMS);
    rs_write_rooms(savef, passages, MAXPASS);
    rs_write_monsters(savef,monsters,26);
    rs_write_obj_info(savef, things,  NUMTHINGS);
    rs_write_obj_info(savef, arm_info,  MAXARMORS);
    rs_write_obj_info(savef, pot_info,  MAXPOTIONS);
    rs_write_obj_info(savef, ring_info,  MAXRINGS);
    rs_write_obj_info(savef, scr_info,  MAXSCROLLS);
    rs_write_obj_info(savef, weap_info,  MAXWEAPONS+1);
    rs_write_obj_info(savef, ws_info, MAXSTICKS);
    rs_write_daemons(savef, &d_list[0], 20);
    rs_write_int(savef,between);
    rs_write_int(savef, group);
    rs_write_window(savef,stdscr);

    return( encclearerr() );
}

/* A thing's t_room can point into rooms[] OR passages[].  The original
 * save format only encodes room indices, writing -1 for passage pointers.
 * Restore t_room from its map position once places[], rooms[] and
 * passages[] have all been loaded.  This also repairs existing JEC saves
 * written while the player or a monster was in a corridor. */
static void
rs_restore_thing_room(THING *tp)
{
    const coord *pos = &tp->t_pos;
    struct room *rp;

    if (encerror())
        return;

    /* Guard roomin(), which indexes places[] using the saved position. */
    if (pos->x < 0 || pos->x >= NUMCOLS ||
        pos->y < 0 || pos->y >= NUMLINES)
    {
        encseterr(EILSEQ);
        return;
    }
    if ((flat(pos->y, pos->x) & F_PASS) &&
        (flat(pos->y, pos->x) & F_PNUM) >= MAXPASS)
    {
        encseterr(EILSEQ);
        return;
    }

    rp = roomin(pos);
    if (rp == NULL)
        encseterr(EILSEQ);
    else
        tp->t_room = rp;
}

static void
rs_restore_thing_rooms(void)
{
    THING *tp;

    rs_restore_thing_room(&player);
    for (tp = mlist; tp != NULL && !encerror(); tp = tp->l_next)
        rs_restore_thing_room(tp);
}

int
rs_restore_file(FILE *savef)
{
    int i;

    encclearerr();
    rs_saved_list_nodes_left = RS_MAX_SAVED_LIST_NODES;

    rs_read_int(savef, &noscore);
    rs_read_int(savef, &seenstairs);
    rs_read_int(savef, &amulet);
    rs_read_int(savef, &fight_flush);
    rs_read_int(savef, &jump);
    rs_read_int(savef, &passgo);
    rs_read_int(savef, &see_floor);
    rs_read_int(savef, &terse);
    rs_read_int(savef, &tombstone);

    rs_read_int(savef, &i);
    idscr_md	= (i&1) ? TRUE : FALSE;
    color_mode	= (i&2) ? TRUE : FALSE;
    show_cursor	= (i&4) ? TRUE : FALSE;

    rs_read_ints(savef, pack_used, RS_PACK_LETTERS);
    if (encerror())
        return encclearerr();
    /* Reject non-boolean occupancy values before reading other state.
     * Compare occupied letters with the player pack after its list loads. */
    rs_validate_pack_used_flags();
    if (encerror())
        return encclearerr();
    rs_read_fixed_text(savef, file_name, MAXSTR);
    rs_read_fixed_text(savef, huh, MAXSTR);
    if (encerror())
        return encclearerr();
    jrogue_message_forget_history(); /* Discard any previous live Ctrl-P cache. */
    rs_read_potions(savef);
    rs_read_rings(savef);
    rs_read_scrolls(savef);
    rs_read_fixed_text(savef, whoami, MAXSTR);
    if (encerror())
        return encclearerr();
    rs_read_sticks(savef);
    rs_read_fixed_text(savef, fruit, MAXSTR);
    rs_read_int(savef, &n_objs);
    if (encerror())
        return encclearerr();
    /* inventory() may count floor items in wizard mode, so MAXPACK is not
     * the appropriate limit.  Use the generous saved-list safety budget. */
    if (n_objs < 0 || n_objs > RS_MAX_SAVED_LIST_NODES)
    {
        encseterr(EILSEQ);
        return encclearerr();
    }
    rs_read_int(savef, &ntraps);
    if (encerror())
        return encclearerr();
    /* The saved trap count is bounded, but need not match visible traps. */
    if (ntraps < 0 || ntraps > MAXTRAPS)
    {
        encseterr(EILSEQ);
        return encclearerr();
    }
    rs_read_int(savef, &hungry_state);
    if (encerror())
        return encclearerr();
    /* The status line indexes a four-element state_name[] with this value. */
    if (hungry_state < 0 || hungry_state > 3)
    {
        encseterr(EILSEQ);
        return encclearerr();
    }
    rs_read_int(savef, &inpack);
    if (encerror())
        return encclearerr();
    /* A pack slot count is not necessarily its linked-list node count. */
    if (inpack < 0 || inpack > MAXPACK)
    {
        encseterr(EILSEQ);
        return encclearerr();
    }
    rs_read_int(savef, &inv_type);
    if (encerror())
        return encclearerr();
    /* options.c indexes inv_t_name[] using the saved inventory style. */
    if (inv_type < INV_OVER || inv_type > INV_CLEAR)
    {
        encseterr(EILSEQ);
        return encclearerr();
    }
    rs_read_int(savef, &level);
    rs_read_int(savef, &max_level);
    rs_read_int(savef, &no_food);
    rs_read_int(savef, &food_left);
    rs_read_int(savef, &lastscore);
    rs_read_int(savef, &no_move);
    rs_read_int(savef, &purse);
    rs_read_int(savef, &quiet);
    rs_read_int(savef, &vf_hit);
    if (encerror())
        return encclearerr();
    /* Only nonnegative counters can arise in normal play.  A bogus huge
     * flytrap count would cause billions of dice rolls on the next attack. */
    if (level < 1 || level > JROGUE_MAX_DUNGEON_LEVEL ||
        max_level < level || max_level > JROGUE_MAX_DUNGEON_LEVEL ||
        purse < 0 || no_food < 0 || no_move < 0 || quiet < 0 ||
        vf_hit < 0 || vf_hit > JROGUE_MAX_FLYTRAP_HITS)
    {
        encseterr(EILSEQ);
        return encclearerr();
    }
    rs_read_uint(savef, &seed);
    rs_read_coord(savef, &stairs);
    rs_read_thing(savef, &player);
    if (encerror())
        return encclearerr();
    rs_validate_player_pack_letters();
    if (encerror())
        return encclearerr();
    rs_read_object_reference(savef, player.t_pack, &cur_armor);
    rs_read_object_reference(savef, player.t_pack, &cur_ring[0]);
    rs_read_object_reference(savef, player.t_pack, &cur_ring[1]);
    rs_read_object_reference(savef, player.t_pack, &cur_weapon);
    rs_read_object_list(savef, &lvl_obj);
    rs_read_thing_list(savef, &mlist);
    rs_fix_thing(&player);
    rs_fix_thing_list(mlist);
    rs_read_places(savef,places,MAXLINES*MAXCOLS);
    rs_read_stats(savef, &max_stats);
    if (encerror())
        return encclearerr();
    if (strcmp(max_stats.s_dmg, "%%%x0") == 0)
    {
        encseterr(EILSEQ);
        return encclearerr();
    }
    rs_read_rooms(savef, rooms, MAXROOMS);
    rs_read_rooms(savef, passages, MAXPASS);
    rs_read_monsters(savef,monsters,26);
    rs_read_obj_info(savef, things,  NUMTHINGS);
    rs_read_obj_info(savef, arm_info,   MAXARMORS);
    rs_read_obj_info(savef, pot_info,  MAXPOTIONS);
    rs_read_obj_info(savef, ring_info,  MAXRINGS);
    rs_read_obj_info(savef, scr_info,  MAXSCROLLS);
    rs_read_obj_info(savef, weap_info, MAXWEAPONS+1);
    rs_read_obj_info(savef, ws_info, MAXSTICKS);
    rs_read_daemons(savef, d_list, 20);
    rs_read_int(savef,&between);
    rs_read_int(savef,&group);
    if (encerror())
        return encclearerr();
    /* The wandering-monster counter runs 0..3; weapon stack group IDs
     * begin at 2 (0 means ungrouped, 1 is reserved for gold). */
    if (between < 0 || between > 3 || group < 2)
    {
        encseterr(EILSEQ);
        return encclearerr();
    }
    rs_read_window(savef,stdscr);

    /* All map and room data is available now.  Fix the player (proom)
     * and monster room pointers before normal play resumes. */
    if (!encerror())
        rs_restore_thing_rooms();

    return( encclearerr() );
}
