/*
 * This file has all the code for the option command.  I would rather
 * this command were not necessary, but it is the only way to keep the
 * wolves off of my back.
 *
 * @(#)options.c	4.24 (Berkeley) 05/10/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <curses.h>
#include <ctype.h>
#include <string.h>
#include "rogue.h"
#include "jec/unicode_text.h"
#ifdef _WIN32
#include "windows/rogue_windows_console.h"
#endif

/* JEC: limit newly assigned player names, not restored save names. */
static void
jrogue_player_name_limit(char *name)
{
    if (name != NULL)
        name[jrogue_text_prefix_cells(name, JROGUE_PLAYER_NAME_MAX_CELLS)] = '\0';
}

#define	EQSTR(a, b, c)	(strncmp(a, b, c) == 0)

#define	NUM_OPTS	(sizeof optlist / sizeof (OPTION))
#define OPT_REDRAW	3

/*
 * description of an option and what to do with it
 */
struct optstruct {
    const char	*o_name;	/* option name */
    const char	*o_prompt;	/* prompt for interactive entry */
    void 	*o_opt;		/* pointer to thing to set */
				/* function to print value */
    void 	(*o_putfunc)(void *opt);
				/* function to get value interactively */
    int		(*o_getfunc)(void *opt, WINDOW *win);
};

typedef struct optstruct	OPTION;

void	pr_optname(const OPTION *op);
static int get_fixed_bool(void *vp, WINDOW *win);
static void put_lang(void *vp);
static int get_lang(void *vp, WINDOW *win);
static void option_redraw(void);

static void
option_refresh(WINDOW *win)
{
    wrefresh(win);
#ifdef _WIN32
    jrogue_windows_option_vt_restore_nonbmp(win);
#endif
}

static OPTION	optlist[] = {
    {"terse",	 JSTR_STATIC("簡潔な表示", "Terse output"),
		 &terse,	put_bool,	get_bool	},
    {"flush",	 JSTR_STATIC("戦闘時に先行入力をキャンセル", "Flush typeahead during battle"),
		 &fight_flush,	put_bool,	get_bool	},
    {"jump",	 JSTR_STATIC("移動中の表示を行わない", "Show position only at end of run"),
		 &jump,		put_bool,	get_bool	},
    {"seefloor", JSTR_STATIC("ランプで照らされた床を表示", "Show the lamp-illuminated floor"),
		 &see_floor,	put_bool,	get_sf		},
    {"passgo",	JSTR_STATIC("通路の角で止まらずに進む", "Follow turnings in passageways"),
		 &passgo,	put_bool,	get_bool	},
    {"tombstone", JSTR_STATIC("死んだときに墓石を表示", "Print out tombstone when killed"),
		 &tombstone,	put_bool,	get_bool	},
    {"inven",	JSTR_STATIC("持ちものの表示方式", "Inventory style"),
		 &inv_type,	put_inv_t,	get_inv_t	},
    {"name",	 JSTR_STATIC("プレイヤー名", "Name"),
		 whoami,	put_str,	get_str		},
    {"fruit",	 JSTR_STATIC("好きな果物", "Fruit"),
		 fruit,		put_str,	get_str		},
    {"idscrl",	 JSTR_STATIC("識別の巻き物を１種類に設定（変更不可）", "1 kind of identify scroll [fixed]"),
		 &idscr_md,	put_bool,	get_fixed_bool	},
    {"color", JSTR_STATIC("カラーで表示", "Use color mode"),
		 &color_mode,	put_bool,	get_bool	},
    {"cursor", JSTR_STATIC("プレイ中のカーソルを表示", "Show cursor during play"),
		 &show_cursor,	put_bool,	get_bool	},
    {"language", JSTR_STATIC("表示する言語", "Language"),
		 &jrogue_lang_is_japanese,	put_lang,	get_lang	},
    {"file",	 JSTR_STATIC("セーブファイル名", "Save file"),
		 file_name,	put_str,	get_str		}
};
void
jrogue_apply_options_language(void)
{
    optlist[0].o_prompt = JSTR("簡潔な表示", "Terse output");
    optlist[1].o_prompt = JSTR("戦闘時に先行入力をキャンセル", "Flush typeahead during battle");
    optlist[2].o_prompt = JSTR("移動中の表示を行わない", "Show position only at end of run");
    optlist[3].o_prompt = JSTR("ランプで照らされた床を表示", "Show the lamp-illuminated floor");
    optlist[4].o_prompt = JSTR("通路の角で止まらずに進む", "Follow turnings in passageways");
    optlist[5].o_prompt = JSTR("死んだときに墓石を表示", "Print out tombstone when killed");
    optlist[6].o_prompt = JSTR("持ちものの表示方式", "Inventory style");
    optlist[7].o_prompt = JSTR("プレイヤー名", "Name");
    optlist[8].o_prompt = JSTR("好きな果物", "Fruit");
    optlist[9].o_prompt = JSTR("識別の巻き物を１種類に設定（変更不可）", "1 kind of identify scroll [fixed]");
    optlist[10].o_prompt = JSTR("カラーで表示", "Use color mode");
    optlist[11].o_prompt = JSTR("プレイ中のカーソルを表示", "Show cursor during play");
    optlist[12].o_prompt = JSTR("表示する言語", "Language");
    optlist[13].o_prompt = JSTR("セーブファイル名", "Save file");
}



static void
option_redraw(void)
{
    const OPTION *op;

    wclear(hw);
    jrogue_wset_normal_attr(hw);
    for (op = optlist; op <= &optlist[NUM_OPTS-1]; op++)
    {
	pr_optname(op);
	(*op->o_putfunc)(op->o_opt);
	jrogue_waddch(hw, '\n');
    }
}


/*
 * option:
 *	Print and then set options from the terminal
 */

static int get_str_use_initial = FALSE;

void
option(void)
{
    const OPTION *op;
    int		retval;


    /*
     * Display current values of options
     */
    option_redraw();
    /*
     * Set values
     */
    wmove(hw, 0, 0);
    for (op = optlist; op <= &optlist[NUM_OPTS-1]; op++)
    {
	pr_optname(op);
	get_str_use_initial = TRUE;
	retval = (*op->o_getfunc)(op->o_opt, hw);
	get_str_use_initial = FALSE;
	if (retval)
	{
	    if (retval == QUIT)
		break;
	    else if (retval == OPT_REDRAW)
	    {
		option_redraw();
		wmove(hw, (int)(op - optlist) + 1, 0);
	    }
	    else if (op > optlist) {	/* MINUS */
		wmove(hw, (int)(op - optlist) - 1, 0);
		op -= 2;
	    }
	    else	/* trying to back up beyond the top */
	    {
		putchar('\007');
		wmove(hw, 0, 0);
		op--;
	    }
	}
    }
    /*
     * Switch back to original screen
     */
    wmove(hw, _LINES - 1, 0);
    waddstr(hw, JSTR("--スペースを押してください--", "--Press space to continue--"));
    option_refresh(hw);
    jrogue_show_cursor(TRUE);
    wait_for(hw, ' ');
    jrogue_restore_cursor();
    /* Replace the whole option window with the dungeon and resynchronize
     * the physical cursor position, including after a language switch. */
    clearok(stdscr, TRUE);
    touchwin(stdscr);
    status();
    wrefresh(stdscr);

    after = FALSE;
}

/*
 * pr_optname:
 *	Print out the option name prompt
 */

void
pr_optname(const OPTION *op)
{
    wprintw(hw, "%s (\"%s\"): ", op->o_prompt, op->o_name);
}

/*
 * put_bool
 *	Put out a boolean
 */

void
put_bool(void *b)
{
    waddstr(hw, *(int *) b ? JSTR("有効(T)", "True") : JSTR("無効(F)", "False"));
}

/*
 * put_str:
 *	Put out a string
 */

/* Draw option strings in one terminal row.  A long value must never
 * wrap over the following option labels while the menu is displayed. */
static void
option_waddstr_utf8(WINDOW *win, const char *text)
{
#ifdef _WIN32
    jrogue_waddstr_utf8(win, text);
#else
    waddstr(win, text);
#endif
}

/* Draw a complete-grapheme slice, reserving the final cell for an optional
 * '>' marker or for the editor's cursor.  A '<' means that earlier characters
 * are hidden.  The return value is the visible part's cell count. */
static int
str_draw_view(WINDOW *win, const char *text, size_t start, int cells)
{
    char shown[MAXSTR + 2];
    size_t at, next, out, len;
    int used, width;

    if (cells <= 0)
        return 0;
    len = strlen(text);
    if (start > len)
        start = len;
    out = 0;
    used = 0;
    if (start > 0 && cells > 1)
    {
        shown[out++] = '<';
        used++;
    }
    at = start;
    while (at < len)
    {
        next = jrogue_text_next(text, at);
        width = jrogue_text_width_n(text, next) -
                jrogue_text_width_n(text, at);
        if (used + width >= cells || out + next - at >= sizeof(shown))
            break;
        memcpy(shown + out, text + at, next - at);
        out += next - at;
        used += width;
        at = next;
    }
    if (at < len && used < cells)
    {
        shown[out++] = '>';
        used++;
    }
    shown[out] = '\0';
    option_waddstr_utf8(win, shown);
    return used;
}

void
put_str(void *str)
{
    int y, x, rows, cols;

    getyx(hw, y, x);
    getmaxyx(hw, rows, cols);
    (void)y;
    (void)rows;
    /* Leave the terminal's final column untouched to avoid auto-wrap. */
    str_draw_view(hw, (const char *)str, 0, cols - x - 1);
}

/*
 * put_inv_t:
 *	Put out an inventory type
 */

void
put_inv_t(void *ip)
{
    waddstr(hw, inv_t_name[*(int *) ip]);
}


static void
put_lang(void *vp)
{
    int *jp = (int *) vp;

    if (*jp)
	waddstr(hw, JSTR("日本語(J)", "Japanese(J)"));
    else
	waddstr(hw, JSTR("英語(E)", "English(E)"));
}

static int
get_lang(void *vp, WINDOW *win)
{
    int *jp = (int *) vp;
    int old_lang;
    int oy, ox;

    old_lang = *jp;
    getyx(win, oy, ox);
    jrogue_wset_normal_attr(win);
    put_lang(vp);

    for (;;)
    {
	wmove(win, oy, ox);
	option_refresh(win);
	jrogue_show_cursor(TRUE);
	switch (wreadchar(win))
	{
	    case 'j':
	    case 'J':
		*jp = TRUE;
		break;
	    case 'e':
	    case 'E':
		*jp = FALSE;
		break;
	    case '\n':
	    case '\r':
		wmove(win, oy, ox);
		put_lang(vp);
		wclrtoeol(win);
		jrogue_waddch(win, '\n');
		return NORM;
	    case ESCAPE:
		return QUIT;
	    case '-':
		return MINUS;
	    default:
		wmove(win, oy, ox + 15);
		waddstr(win, JSTR("［日本語(J)、英語(E)］", "(J or E)"));
		continue;
	}
	break;
    }

    jrogue_apply_language();
    wmove(win, oy, ox);
    put_lang(vp);
    wclrtoeol(win);
    jrogue_waddch(win, '\n');

    return (*jp != old_lang) ? OPT_REDRAW : NORM;
}


/*
 * get_bool:
 *	Allow changing a boolean option and print it out
 */
int
get_bool(void *vp, WINDOW *win)
{
    int *bp = (int *) vp;
    int oy, ox;
    int op_bad;

    op_bad = TRUE;
    getyx(win, oy, ox);
    jrogue_wset_normal_attr(win);
    waddstr(win, *bp ? JSTR("有効(T)", "True") : JSTR("無効(F)", "False"));
    while (op_bad)	
    {
	wmove(win, oy, ox);
	option_refresh(win);
	jrogue_show_cursor(TRUE);
	switch (wreadchar(win))
	{
	    case 't':
	    case 'T':
		*bp = TRUE;
		op_bad = FALSE;
		break;
	    case 'f':
	    case 'F':
		*bp = FALSE;
		op_bad = FALSE;
		break;
	    case '\n':
	    case '\r':
		op_bad = FALSE;
		break;
	    case ESCAPE:
		return QUIT;
	    case '-':
		return MINUS;
	    default:
		wmove(win, oy, ox + 10);
		waddstr(win, JSTR("［有効(T)、無効(F)］", "(T or F)"));
	}
    }
    wmove(win, oy, ox);
    waddstr(win, *bp ? JSTR("有効(T)", "True") : JSTR("無効(F)", "False"));
    jrogue_waddch(win, '\n');
    return NORM;
}


/*
 * get_fixed_bool:
 *	Print a boolean option that cannot be changed interactively.
 *
 * idscrl must be set before the game starts, since changing it after the
 * scroll table has been initialized would desynchronize object identities.
 * Keep the option visible, but do not let the option screen temporarily show
 * a changed value.
 */
static int
get_fixed_bool(void *vp, WINDOW *win)
{
    int *bp = (int *) vp;
    int oy, ox;

    getyx(win, oy, ox);
    jrogue_wset_normal_attr(win);
    waddstr(win, *bp ? JSTR("有効(T)", "True") : JSTR("無効(F)", "False"));

    for (;;)
    {
	wmove(win, oy, ox);
	option_refresh(win);
	jrogue_show_cursor(TRUE);
	switch (wreadchar(win))
	{
	    case '\n':
	    case '\r':
		wmove(win, oy, ox);
		waddstr(win, *bp ? JSTR("有効(T)", "True") : JSTR("無効(F)", "False"));
		wclrtoeol(win);
		jrogue_waddch(win, '\n');
		return NORM;
	    case ESCAPE:
		return QUIT;
	    case '-':
		return MINUS;
	    default:
		wmove(win, oy, ox + 10);
		waddstr(win, JSTR("［変更不可］", "(fixed)"));
	}
    }
}


/*
 * get_sf:
 *	Change value and handle transition problems from see_floor to
 *	!see_floor.
 */
int
get_sf(void *vp, WINDOW *win)
{
    int	*bp = (int *) vp;
    int	was_sf;
    int		retval;

    was_sf = see_floor;
    retval = get_bool(bp, win);
    /* Preserve MINUS as well as QUIT so "-" returns to the previous option. */
    if (retval != NORM)
        return retval;
    if (was_sf != see_floor)
    {
	if (!see_floor) {
	    see_floor = TRUE;
	    erase_lamp(&hero, proom);
	    see_floor = FALSE;
	}
	else
	    look(FALSE);
    }
    return(NORM);
}

/*
 * get_str:
 *	Set a string option
 */
#define MAXINP	48	/* legacy string input; wizard numbers remain unchanged */
#define JROGUE_CALL_MAX_CELLS 50

/* The on-disk format and MAXSTR buffers do not change.  A special input
 * context changes only the interactive entry limits for call names and
 * save file paths, leaving unrelated get_str callers at MAXINP bytes. */
enum jrogue_string_input {
    STRING_LEGACY,
    STRING_CALLED_NAME,
    STRING_SAVE_FILENAME
};


static int
str_utf8_len(unsigned char c)
{
    if (c < 0x80)
	return 1;
    if ((c & 0xe0) == 0xc0)
	return 2;
    if ((c & 0xf0) == 0xe0)
	return 3;
    if ((c & 0xf8) == 0xf0)
	return 4;
    return 1;
}

static int
str_read_utf8_char(WINDOW *win, int first, char *out)
{
    int i, need, c;

    if (first < 0 || first > 255)
	return 0;
    need = str_utf8_len((unsigned char)first);
    out[0] = (char)first;

    for (i = 1; i < need; i++)
    {
	jrogue_prepare_input_attr(win);
	c = wgetch(win);
	if (c < 0 || c > 255 || (((unsigned char)c & 0xc0) != 0x80))
	    return i;
	out[i] = (char)c;
    }
    return need;
}


/* Keep the cursor visible in the single-line editor without truncating
 * the underlying UTF-8 string.  view_start is always a grapheme boundary. */
static void
str_redraw_edit(WINDOW *win, int y, int x, const char *buf, int pos,
                size_t *view_start)
{
    int rows, cols, available, cursor_cell;
    size_t cursor = (size_t)pos;

    getmaxyx(win, rows, cols);
    (void)rows;
    available = cols - x - 1;
    if (available < 1)
        available = 1;
    if (*view_start > cursor)
        *view_start = cursor;
    /* Reserve a cell for the cursor, and one for the '<' indicator if
     * the beginning of the value has scrolled out of view. */
    while (*view_start < cursor &&
           jrogue_text_width_n(buf, cursor) -
           jrogue_text_width_n(buf, *view_start) +
           (*view_start > 0 ? 1 : 0) >= available)
        *view_start = jrogue_text_next(buf, *view_start);

    cursor_cell = jrogue_text_width_n(buf, cursor) -
                  jrogue_text_width_n(buf, *view_start);
    if (*view_start > 0 && available > 1)
        cursor_cell++;
    if (cursor_cell >= available)
        cursor_cell = available - 1;

    jrogue_wset_normal_attr(win);
    wmove(win, y, x);
    str_draw_view(win, buf, *view_start, available);
    wclrtoeol(win);
    wmove(win, y, x + cursor_cell);
    option_refresh(win);
}


static int
str_read_edit_key(WINDOW *win, int *is_unicode)
{
    int c, c2, c3;

    *is_unicode = FALSE;
#ifdef _WIN32
    /* Read the full UTF-16 key events on Windows, preserving emoji and IME
     * text in WinCon and Windows Terminal.  Use curses on non-console input. */
    if (jrogue_windows_read_option_key(&c))
    {
        *is_unicode = TRUE;
        return c;
    }
#endif
    c = wgetch(win);
    if (c != ESCAPE)
        return c;

    nodelay(win, TRUE);
    c2 = wgetch(win);
    nodelay(win, FALSE);
    if (c2 == ERR)
        return ESCAPE;

    if (c2 == '[' || c2 == 'O')
    {
        c3 = wgetch(win);
        switch (c3)
        {
            case 'D': return KEY_LEFT;
            case 'C': return KEY_RIGHT;
            case '3':
                (void) wgetch(win);   /* usually '~' */
                return KEY_DC;
        }
    }
    return ESCAPE;
}

static int
get_str_context(void *vopt, WINDOW *win, enum jrogue_string_input context)
{
    char *opt = (char *) vopt;
    int oy, ox;
    int c, len, pos, next, clen, is_unicode, max_input, changed = FALSE;
    int is_fruit = (get_str_use_initial && opt == fruit);
    int is_player_name = (get_str_use_initial && opt == whoami);
    int is_save_filename = (context == STRING_SAVE_FILENAME ||
                            (get_str_use_initial && opt == file_name));
    int cell_limit = context == STRING_CALLED_NAME ? JROGUE_CALL_MAX_CELLS :
                     (is_fruit ? JROGUE_FRUIT_MAX_CELLS :
                     (is_player_name ? JROGUE_PLAYER_NAME_MAX_CELLS : 0));
    size_t view_start = 0;
    char chbuf[5];
    static char buf[MAXSTR];

    getyx(win, oy, ox);
    keypad(win, TRUE);
    /* Menu options, call names and save paths may use the full MAXSTR
     * buffer; all other command inputs retain the old 48-byte bound. */
    max_input = (get_str_use_initial || context == STRING_CALLED_NAME ||
                 is_save_filename) ? MAXSTR - 1 : MAXINP;
    if (get_str_use_initial)
    {
        len = (int)jrogue_text_prefix(opt, (size_t)max_input);
        memcpy(buf, opt, (size_t)len);
        buf[len] = '\0';
    }
    else
    {
        buf[0] = '\0';
        len = 0;
    }
    pos = len;
    str_redraw_edit(win, oy, ox, buf, pos, &view_start);
    jrogue_show_cursor(TRUE);

    /*
     * Loop reading in the string, and put it in a temporary buffer.
     * Prepare the terminal attribute immediately before wgetch(); this is
     * important for IME/preedit text, which may be drawn by the terminal
     * before curses receives the confirmed characters.
     */
    for (;;)
    {
	jrogue_prepare_input_attr(win);
	c = str_read_edit_key(win, &is_unicode);

	if (c == '\n' || c == '\r')
        {
            /* Existing saves may contain names longer than the new limit.
             * Enter without edits preserves them.  An edited replacement
             * must satisfy its applicable cell limit before commit. */
            if (cell_limit && changed &&
                jrogue_text_width(buf) > cell_limit)
            {
                putchar(CTRL('G'));
                continue;
            }
            break;
        }
        if (c == ESCAPE)
            break;
	if (c == -1)
	    continue;
	if (c == KEY_LEFT)
	{
	    pos = (int)jrogue_text_prev(buf, (size_t)pos);
	    str_redraw_edit(win, oy, ox, buf, pos, &view_start);
	    continue;
	}
	if (c == KEY_RIGHT)
	{
	    pos = (int)jrogue_text_next(buf, (size_t)pos);
	    str_redraw_edit(win, oy, ox, buf, pos, &view_start);
	    continue;
	}
	if (c == KEY_BACKSPACE || c == erasechar() || c == '\b' || c == 0177)
	{
	    if (pos > 0)
	    {
		int prev = (int)jrogue_text_prev(buf, (size_t)pos);
		memmove(&buf[prev], &buf[pos], (size_t)(len - pos + 1));
		len -= pos - prev;
		pos = prev;
                changed = TRUE;
	    }
	    str_redraw_edit(win, oy, ox, buf, pos, &view_start);
	    continue;
	}
	if (c == KEY_DC)
	{
	    if (pos < len)
	    {
		next = (int)jrogue_text_next(buf, (size_t)pos);
		memmove(&buf[pos], &buf[next], (size_t)(len - next + 1));
		len -= next - pos;
                changed = TRUE;
	    }
	    str_redraw_edit(win, oy, ox, buf, pos, &view_start);
	    continue;
	}
	if (c == killchar())	/* process kill character */
	{
            if (len > 0)
                changed = TRUE;
	    len = 0;
	    pos = 0;
	    buf[0] = '\0';
	    str_redraw_edit(win, oy, ox, buf, pos, &view_start);
	    continue;
	}
	if (len == 0 && pos == 0)
	{
	    if (c == '-' && win != stdscr)
		break;
	    else if (c == '~')
	    {
                /* Do not silently truncate a home directory path. */
                if (strlen(home) > (size_t)max_input ||
                    (cell_limit &&
                     jrogue_text_width(home) > cell_limit))
                {
                    putchar(CTRL('G'));
                    continue;
                }
		len = (int)jrogue_text_prefix(home, (size_t)max_input);
		memcpy(buf, home, (size_t)len);
		buf[len] = '\0';
		pos = len;
                changed = TRUE;
		str_redraw_edit(win, oy, ox, buf, pos, &view_start);
		continue;
	    }
	}
#ifdef _WIN32
        if (is_unicode)
            clen = jrogue_windows_encode_unicode((unsigned int)c, chbuf);
        else
#endif
        {
	    if (c < 0 || c > 255)
	    {
		putchar(CTRL('G'));
		continue;
	    }
	    clen = str_read_utf8_char(win, c, chbuf);
	}
	if (clen <= 0)
	{
	    putchar(CTRL('G'));
	    continue;
	}
	if (len + clen > max_input)
	    putchar(CTRL('G'));
	else
	{
            /* Try the insertion before accepting it: a Japanese character
             * or emoji can occupy two cells even when it is a single
             * editing unit.  Backspace/Delete remain available on old
             * overlong names so they can be reduced without data loss. */
	    memmove(&buf[pos + clen], &buf[pos], (size_t)(len - pos + 1));
	    memcpy(&buf[pos], chbuf, (size_t)clen);
            if (cell_limit && jrogue_text_width(buf) > cell_limit)
            {
                memmove(&buf[pos], &buf[pos + clen],
                        (size_t)(len - pos + 1));
                putchar(CTRL('G'));
                continue;
            }
	    len += clen;
            changed = TRUE;
	    /* Snap past a newly completed combined character. */
	    pos = (int)jrogue_text_next(buf, (size_t)(pos + clen - 1));
	    str_redraw_edit(win, oy, ox, buf, pos, &view_start);
	}
    }
    /* In the options menu, Enter with no edits keeps the old value.
     * In command-line entry (call/call_it), even an empty Enter must copy
     * the empty result: otherwise the caller sees stale data in prbuf. */
    if (c != ESCAPE && !(c == '-' && len == 0 && win != stdscr) &&
        (!get_str_use_initial || changed))
        jrogue_strlcpy(opt, buf, MAXSTR);

    /* Repaint only the visible slice: a full pathname must not wrap over
     * the next option (or the following dungeon status line). */
    str_redraw_edit(win, oy, ox, opt,
                    get_str_use_initial ? pos : (int)strlen(opt),
                    &view_start);
    jrogue_wset_normal_attr(win);
    /* In curses, writing '\n' clears from the current column to the end
     * of the line.  Returning to ox before writing it erased the string we
     * had just repainted (notably on macOS curses).  Move to the next row
     * without writing to, or clearing, the completed option value. */
    if (oy + 1 < getmaxy(win))
        wmove(win, oy + 1, 0);
    option_refresh(win);
    if (win == stdscr)
    {
        /* Long paths are horizontally scrolled, not printed across the
         * dungeon.  Keep the message cursor inside the 80-column row. */
        int occupied = jrogue_text_width(buf);
        mpos = occupied >= NUMCOLS - 1 - mpos ?
               NUMCOLS - 1 : mpos + occupied;
    }
    if (c == '-')
	return MINUS;
    else if (c == ESCAPE)
	return QUIT;
    else
	return NORM;
}


/* A specific caller opts into a larger input space.  The two named
 * entry points avoid changing get_str() for wizard or other prompts. */
int
get_str(void *vopt, WINDOW *win)
{
    return get_str_context(vopt, win, STRING_LEGACY);
}

int
get_str_called_name(void *vopt, WINDOW *win)
{
    return get_str_context(vopt, win, STRING_CALLED_NAME);
}

int
get_str_filename(void *vopt, WINDOW *win)
{
    return get_str_context(vopt, win, STRING_SAVE_FILENAME);
}

/*
 * get_inv_t
 *	Get an inventory type name
 */
int
get_inv_t(void *vp, WINDOW *win)
{
    int *ip = (int *) vp;
    int oy, ox;
    int op_bad;

    op_bad = TRUE;
    getyx(win, oy, ox);
    jrogue_wset_normal_attr(win);
    waddstr(win, inv_t_name[*ip]);
    while (op_bad)	
    {
	wmove(win, oy, ox);
	option_refresh(win);
	jrogue_show_cursor(TRUE);
	switch (wreadchar(win))
	{
	    case 'o':
	    case 'O':
		*ip = INV_OVER;
		op_bad = FALSE;
		break;
	    case 's':
	    case 'S':
		*ip = INV_SLOW;
		op_bad = FALSE;
		break;
	    case 'c':
	    case 'C':
		*ip = INV_CLEAR;
		op_bad = FALSE;
		break;
	    case '\n':
	    case '\r':
		op_bad = FALSE;
		break;
	    case ESCAPE:
		return QUIT;
	    case '-':
		return MINUS;
	    default:
		wmove(win, oy, ox + 15);
		waddstr(win, JSTR("［上書き(O)、低速(S)、クリア(C)］", "(O, S, or C)"));
	}
    }
    mvwprintw(win, oy, ox, "%s\n", inv_t_name[*ip]);
    return NORM;
}
	

#ifdef MASTER
/*
 * get_num:
 *	Get a numeric option
 */
int
get_num(void *vp, WINDOW *win)
{
    int *opt = (int *) vp;
    int i;
    static char buf[MAXSTR];

    if ((i = get_str(buf, win)) == NORM)
	*opt = atoi(buf);
    return i;
}
#endif

/*
 * parse_string_option:
 *      Set a string option, expanding a leading '~' only for actual
 *      string destinations.  All of the put_str destinations are MAXSTR
 *      byte arrays.  Compose the result before changing the option so
 *      an oversized home directory leaves the previous value intact.
 */
/* ROGUEOPTS fruit names are limited to 50 display cells, player names
 * to 32 cells, save paths to 1023 bytes; unrelated options stay unchanged. */
static void strucpy_fruit(char *dest, const char *src, size_t len);
static void strucpy_filename(char *dest, const char *src, size_t len);

static void
parse_string_option(const OPTION *op, const char *value, size_t len)
{
    char suffix[MAXSTR];
    char result[MAXSTR];
    const char *rest = value;
    size_t home_len, suffix_len;
    int had_slash = FALSE;
    int is_fruit = (op->o_opt == fruit);
    int is_file = (op->o_opt == file_name);

    /* Never save a silently truncated pathname.  Expansion of '~' is
     * separately checked against the same 1023-byte destination limit. */
    if (is_file && len > MAXSTR - 1)
        return;

    if (len == 0)
        return;

    if (*rest != '~')
    {
        if (is_fruit)
            strucpy_fruit((char *)op->o_opt, value, len);
        else if (op->o_opt == whoami)
            strucpy_name((char *)op->o_opt, value, len);
        else if (is_file)
            strucpy_filename((char *)op->o_opt, value, len);
        else
            strucpy2((char *)op->o_opt, value, len);
        return;
    }

    /* Skip the tilde and any following slashes, without passing the
     * end of the option or the NUL terminating the environment string. */
    rest++;
    len--;
    while (len > 0 && *rest == '/')
    {
        had_slash = TRUE;
        rest++;
        len--;
    }

    if (is_fruit)
        strucpy_fruit(suffix, rest, len);
    else if (op->o_opt == whoami)
        strucpy_name(suffix, rest, len);
    else if (is_file)
        strucpy_filename(suffix, rest, len);
    else
        strucpy2(suffix, rest, len);
    suffix_len = strlen(suffix);

    /* home is normally NUL-terminated by main.c; validate it here too.
     * Preserve one path separator for ~/... unless home already ends /.
     */
    {
        const char *end = memchr(home, '\0', MAXSTR);
        if (end == NULL)
            return;
        home_len = (size_t)(end - home);
    }
    if (home_len > MAXSTR - 1)
        return;
    if (had_slash && home_len != 0 && home[home_len - 1] != '/')
    {
        if (home_len >= MAXSTR - 1)
            return;
        memcpy(result, home, home_len);
        result[home_len++] = '/';
    }
    else
        memcpy(result, home, home_len);

    if (suffix_len > MAXSTR - 1 - home_len)
        return;
    memcpy(result + home_len, suffix, suffix_len + 1);
    memcpy(op->o_opt, result, home_len + suffix_len + 1);
    if (is_fruit)
        ((char *)op->o_opt)[jrogue_text_prefix_cells((const char *)op->o_opt,
                                                    JROGUE_FRUIT_MAX_CELLS)] = '\0';
    else if (op->o_opt == whoami)
        jrogue_player_name_limit((char *)op->o_opt);
}

/*
 * parse_opts:
 *      Parse the comma-separated ROGUEOPTS string.  A missing or empty
 *      value is ignored rather than reading beyond the NUL byte.
 *      Boolean and abbreviated option names retain their original syntax.
 */
void
parse_opts(char *str)
{
    const OPTION *op;
    const char *end, *name_end, *value;
    size_t name_len, value_len;

    while (*str != '\0')
    {
        char setting[MAXINP + 1];
        const char **i;

        while (*str == ',' || *str == ' ' || *str == '\t')
            str++;
        if (*str == '\0')
            break;

        end = str;
        while (*end != '\0' && *end != ',')
            end++;

        name_end = str;
        while (name_end < end && isalpha((unsigned char)*name_end))
            name_end++;
        name_len = (size_t)(name_end - str);

        if (name_len > 0)
        {
            for (op = optlist; op < &optlist[NUM_OPTS]; op++)
            {
                if (strncmp(str, op->o_name, name_len) == 0)
                {
                    if (op->o_putfunc == put_bool)
                        *(int *)op->o_opt = TRUE;
                    else if (name_end < end && *name_end == '=')
                    {
                        value = name_end + 1;
                        while (value < end && *value == '=')
                            value++;
                        value_len = (size_t)(end - value);

                        if (value_len != 0 && op->o_putfunc == put_str)
                            parse_string_option(op, value, value_len);
                        else if (value_len != 0 && value_len <= MAXINP &&
                                 op->o_putfunc == put_lang)
                        {
                            memcpy(setting, value, value_len);
                            setting[value_len] = '\0';
                            if (jrogue_set_language(setting))
                                jrogue_apply_language();
                        }
                        else if (value_len != 0 && value_len <= MAXINP &&
                                 op->o_putfunc == put_inv_t)
                        {
                            memcpy(setting, value, value_len);
                            setting[value_len] = '\0';
                            if (setting[0] >= 'a' && setting[0] <= 'z')
                                setting[0] = (char)(setting[0] - 'a' + 'A');
                            for (i = inv_t_name; i <= &inv_t_name[INV_CLEAR]; i++)
                                if (strncmp(setting, *i, value_len) == 0)
                                {
                                    inv_type = (int)(i - inv_t_name);
                                    break;
                                }
                        }
                    }
                    break;
                }
                if (op->o_putfunc == put_bool && name_len > 2 &&
                    strncmp(str, "no", 2) == 0 &&
                    strncmp(str + 2, op->o_name, name_len - 2) == 0)
                {
                    *(int *)op->o_opt = FALSE;
                    break;
                }
            }
        }

        str = (char *)end;
        if (*str == ',')
            str++;
    }
}

/*
 * strucpy:
 *	Copy string using unctrl for things
 */

/* Copy printable UTF-8 text without dropping its non-ASCII bytes.
 * Previously, isprint(*s2) received negative signed-char values for UTF-8,
 * causing Japanese characters in the Windows account name to disappear.
 * The normal limit remains MAXINP bytes; fruit and player names instead
 * use display-cell limits. Never copy a partial character.
 */
static void
strucpy_printable(char *s1, const char *s2, size_t len, size_t max_bytes)
{
    size_t i = 0, out = 0;

    if (len > max_bytes)
        len = max_bytes;
    /* A byte cap must not cut a combining or multi-code-point emoji. */
    len = jrogue_text_prefix(s2, len);
    while (i < len && s2[i] != '\0')
    {
        unsigned char c = (unsigned char) s2[i];
        size_t need, j;
        unsigned int cp;
        int valid = TRUE;

        if (c < 0x80)
        {
            if (c >= 0x20 && c != 0x7f)
                s1[out++] = (char)c;
            i++;
            continue;
        }
        need = (size_t)str_utf8_len(c);
        if (need == 1 || i + need > len)
            break;
        cp = (unsigned int)(c & (0x7fU >> need));
        for (j = 1; j < need; j++)
        {
            unsigned char d = (unsigned char)s2[i + j];
            if (d == 0 || (d & 0xc0) != 0x80)
            {
                valid = FALSE;
                break;
            }
            cp = (cp << 6) | (d & 0x3f);
        }
        if (valid && ((need == 2 && cp < 0x80) ||
                      (need == 3 && cp < 0x800) ||
                      (need == 4 && cp < 0x10000) ||
                      (cp >= 0xd800 && cp <= 0xdfff) ||
                      cp > 0x10ffff))
            valid = FALSE;
        if (valid)
        {
            memcpy(s1 + out, s2 + i, need);
            out += need;
            i += need;
        }
        else
            i++;
    }
    s1[out] = '\0';
}

void
strucpy(char *s1, const char *s2, size_t len)
{
    strucpy_printable(s1, s2, len, MAXINP);
}

void
strucpy_name(char *dest, const char *src, size_t len)
{
    /* This entry point also handles OS-provided usernames.  Restrict newly
     * assigned names but never rewrite names restored from save files. */
    strucpy_printable(dest, src, len, MAXSTR - 1);
    jrogue_player_name_limit(dest);
}

static void
strucpy_fruit(char *dest, const char *src, size_t len)
{
    strucpy_printable(dest, src, len, MAXSTR - 1);
    dest[jrogue_text_prefix_cells(dest, JROGUE_FRUIT_MAX_CELLS)] = '\0';
}

/* `file=` accepts a full MAXSTR-1 byte UTF-8 path, just like the
 * interactive options screen and the S command.  The caller validates
 * expansion capacity before changing the saved path. */
static void
strucpy_filename(char *dest, const char *src, size_t len)
{
    strucpy_printable(dest, src, len, MAXSTR - 1);
}

void
strucpy2(char *s1, const char *s2, size_t len)
{
    if (len > MAXINP)
	len = MAXINP;
    len = jrogue_text_prefix(s2, len);
    while (len--)
    {
	*s1++ = *s2;
	s2++;
    }
    *s1 = '\0';
}
