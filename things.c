/*
 * Contains functions for dealing with things like potions, scrolls,
 * and other items.
 *
 * @(#)things.c	4.53 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <curses.h>
#include <locale.h>
#include <string.h>
#include <ctype.h>
#include "rogue.h"
#include "jec/unicode_text.h"
#ifdef _WIN32
#include "windows/rogue_windows_console.h"
#endif

/* JEC: truncate fruit names only for display, never in saved data. */
const char *
jrogue_fruit_display(void)
{
    static char shortened[MAXSTR + 4];
    size_t bytes;

    if (jrogue_text_width(fruit) <= JROGUE_FRUIT_MAX_CELLS)
        return fruit;

    /* ASCII dots are one terminal cell each, even in Japanese locales.
     * Never cut between UTF-8 code points or inside a grapheme cluster. */
    bytes = jrogue_text_prefix_cells(fruit, JROGUE_FRUIT_MAX_CELLS - 3);
    memcpy(shortened, fruit, bytes);
    memcpy(shortened + bytes, "...", 4);
    return shortened;
}

/*
 * inv_name:
 *	Return the name of something as it would appear in an
 *	inventory.
 */
static char *inv_name_en(const THING *obj, int drop, int max_cells);
static char *inv_name_ja(const THING *obj, int drop, int max_cells);
static void nameit_en(const THING *obj, const char *type, const char *which, const struct obj_info *op, const char *(*prfunc)(const THING *), int max_cells);
static void nameit_ja(const THING *obj, const char *type, const char *which, const struct obj_info *op, const char *(*prfunc)(const THING *), int max_cells);

#define PRBUF_ROOM(p) \
    ((size_t)(((p) >= prbuf && (p) < prbuf + PRBUF_SIZE) ? \
    (prbuf + PRBUF_SIZE - (p)) : 0))

/* Allow an inventory letter and ") " plus one unused terminal cell.
 * Reserve the equipment marker BEFORE shortening a user-supplied name. */
#define INV_NAME_MAX_CELLS (NUMCOLS - 4)

static const char *
inv_marker(const THING *obj)
{
    if (!inv_describe)
        return "";
    if (jrogue_lang_is_japanese)
    {
        if (obj == cur_armor) return "（着ている）";
        if (obj == cur_weapon) return "（使っている）";
        if (obj == cur_ring[LEFT]) return "（左手）";
        if (obj == cur_ring[RIGHT]) return "（右手）";
    }
    else
    {
        if (obj == cur_armor) return " (being worn)";
        if (obj == cur_weapon) return " (weapon in hand)";
        if (obj == cur_ring[LEFT]) return " (on left hand)";
        if (obj == cur_ring[RIGHT]) return " (on right hand)";
    }
    return "";
}

static int
inv_label_cells(const THING *obj, int max_cells)
{
    return max_cells - jrogue_text_width(inv_marker(obj));
}

static void
inv_add_label(const THING *obj, const char *before, const char *label,
              const char *after, int max_cells)
{
    jrogue_text_append_labeled(prbuf, PRBUF_SIZE, inv_label_cells(obj, max_cells),
                                before, label, after);
}

static char *
inv_name_en(const THING *obj, int drop, int max_cells)
{
    char *pb;
    struct obj_info *op;
    const char *sp;
    int which;

    pb = prbuf;
    which = obj->o_which;
    switch (obj->o_type)
    {
	case POTION:
	    nameit_en(obj, "potion", p_colors[which], &pot_info[which], nullstr, max_cells);
	when RING:
	    nameit_en(obj, "ring", r_stones[which], &ring_info[which], ring_num, max_cells);
	when STICK:
	    nameit_en(obj, ws_type[which], ws_made[which], &ws_info[which], charge_str, max_cells);
	when SCROLL:
	    if (obj->o_count == 1)
	    {
		jrogue_strlcpy(pb, "A scroll ", PRBUF_ROOM(pb));
		pb = &prbuf[9];
	    }
	    else
	    {
		snprintf(pb, PRBUF_ROOM(pb), "%d scrolls ", obj->o_count);
		pb = &prbuf[strlen(prbuf)];
	    }
	    op = &scr_info[which];
	    if (op->oi_know)
		snprintf(pb, PRBUF_ROOM(pb), "of %s", op->oi_name);
	    else if (op->oi_guess)
                inv_add_label(obj, "called ", op->oi_guess, "", max_cells);
	    else
		snprintf(pb, PRBUF_ROOM(pb), "titled '%s'", s_names[which]);
	when FOOD:
	    if (which == 1)
		if (obj->o_count == 1)
		    snprintf(pb, PRBUF_ROOM(pb), "A%s %s", vowelstr(fruit), jrogue_fruit_display());
		else
		    snprintf(pb, PRBUF_ROOM(pb), "%d %ss", obj->o_count, jrogue_fruit_display());
	    else
		if (obj->o_count == 1)
		    jrogue_strlcpy(pb, "Some food", PRBUF_ROOM(pb));
		else
		    snprintf(pb, PRBUF_ROOM(pb), "%d rations of food", obj->o_count);
	when WEAPON:
	    sp = weap_info[which].oi_name;
	    if (obj->o_count > 1)
		snprintf(pb, PRBUF_ROOM(pb), "%d ", obj->o_count);
	    else
		snprintf(pb, PRBUF_ROOM(pb), "A%s ", vowelstr(sp));
	    pb = &prbuf[strlen(prbuf)];
	    if (obj->o_flags & ISKNOW)
		snprintf(pb, PRBUF_ROOM(pb), "%s %s", num(obj->o_hplus,obj->o_dplus,WEAPON), sp);
	    else
		snprintf(pb, PRBUF_ROOM(pb), "%s", sp);
	    if (obj->o_count > 1)
		jrogue_strlcat(pb, "s", PRBUF_ROOM(pb));
	    if (obj->o_label != NULL)
	    {
		pb = &prbuf[strlen(prbuf)];
                inv_add_label(obj, " called ", obj->o_label, "", max_cells);
	    }
	when ARMOR:
	    sp = arm_info[which].oi_name;
	    if (obj->o_flags & ISKNOW)
	    {
		snprintf(pb, PRBUF_ROOM(pb), "%s %s [",
		    num(a_class[which] - obj->o_arm, 0, ARMOR), sp);
		if (!terse)
		    jrogue_strlcat(pb, "protection ", PRBUF_ROOM(pb));
		pb = &prbuf[strlen(prbuf)];
		snprintf(pb, PRBUF_ROOM(pb), "%d]", 10 - obj->o_arm);
	    }
	    else
		snprintf(pb, PRBUF_ROOM(pb), "%s", sp);
	    if (obj->o_label != NULL)
	    {
		pb = &prbuf[strlen(prbuf)];
                inv_add_label(obj, " called ", obj->o_label, "", max_cells);
	    }
	when AMULET:
	    jrogue_strlcpy(pb, "The Amulet of Yendor", PRBUF_ROOM(pb));
	when GOLD:
	    snprintf(prbuf, PRBUF_SIZE, "%d Gold pieces", obj->o_goldval);
#ifdef MASTER
	otherwise:
	    snprintf(pb, PRBUF_ROOM(pb), "Something bizarre %s", unctrl(obj->o_type));
#endif
    }
    if (inv_describe)
    {
	if (obj == cur_armor)
	    jrogue_strlcat(prbuf, " (being worn)", PRBUF_SIZE);
	if (obj == cur_weapon)
	    jrogue_strlcat(prbuf, " (weapon in hand)", PRBUF_SIZE);
	if (obj == cur_ring[LEFT])
	    jrogue_strlcat(prbuf, " (on left hand)", PRBUF_SIZE);
	else if (obj == cur_ring[RIGHT])
	    jrogue_strlcat(prbuf, " (on right hand)", PRBUF_SIZE);
    }
    if (drop && isupper((int)prbuf[0]))
	prbuf[0] = (char) tolower(prbuf[0]);
    else if (!drop && islower((int)*prbuf))
	*prbuf = (char) toupper(*prbuf);
    prbuf[PRBUF_SIZE-1] = '\0';
    return prbuf;
}


static char *
inv_name_ja(const THING *obj, int drop, int max_cells)
{
    char *pb;
    struct obj_info *op;
    const char *sp;
    int which;

    pb = prbuf;
    which = obj->o_which;
    switch (obj->o_type)
    {
        case POTION:
	    nameit_ja(obj, "水薬", p_colors[which], &pot_info[which], nullstr, max_cells);
	when RING:
	    nameit_ja(obj, "指輪", r_stones[which], &ring_info[which], ring_num, max_cells);
	when STICK:
	    nameit_ja(obj, ws_type[which], ws_made[which], &ws_info[which], charge_str, max_cells);
	when SCROLL:
	    if (obj->o_count > 1)
	    {
		snprintf(pb, PRBUF_ROOM(pb), "%dつの", obj->o_count);
		pb = &prbuf[strlen(prbuf)];
	    }
            else
                pb[0] = '\0'; /* A guessed scroll name is appended below. */
	    op = &scr_info[which];
	    if (op->oi_know)
		snprintf(pb, PRBUF_ROOM(pb), "%s巻き物", op->oi_name);
	    else if (op->oi_guess)
                inv_add_label(obj, "『", op->oi_guess, "』と呼ぶ巻き物", max_cells);
	    else
		snprintf(pb, PRBUF_ROOM(pb), "「%s」という巻き物", s_names[which]);
	when FOOD:
	    if (which == 1)
		if (obj->o_count == 1)
		    snprintf(pb, PRBUF_ROOM(pb), "%s", jrogue_fruit_display());
		else
		    snprintf(pb, PRBUF_ROOM(pb), "%d個の%s", obj->o_count, jrogue_fruit_display());
	    else
		if (obj->o_count == 1)
		    jrogue_strlcpy(pb, "1袋の食糧", PRBUF_ROOM(pb));
		else
		    snprintf(pb, PRBUF_ROOM(pb), "%d袋の食糧", obj->o_count);
	when WEAPON:
	    sp = weap_info[which].oi_name;
	    if (obj->o_count > 1)
	    		snprintf(pb, PRBUF_ROOM(pb), "%d個の", obj->o_count);
	    else
	    {
		pb[0] = '\0';
	    }
            if (obj->o_label != NULL)
            {
                char tail[256];
                if (obj->o_flags & ISKNOW)
                    snprintf(tail, sizeof tail, "』と呼ぶ（%s）%s",
                             num(obj->o_hplus, obj->o_dplus, WEAPON), sp);
                else
                    snprintf(tail, sizeof tail, "』と呼ぶ%s", sp);
                inv_add_label(obj, "『", obj->o_label, tail, max_cells);
            }
            else
            {
                pb = &prbuf[strlen(prbuf)];
                if (obj->o_flags & ISKNOW)
                    snprintf(pb, PRBUF_ROOM(pb), "（%s）%s",
                             num(obj->o_hplus,obj->o_dplus,WEAPON), sp);
                else
                    snprintf(pb, PRBUF_ROOM(pb), "%s", sp);
            }
	    if (obj->o_count > 1)
		jrogue_strlcat(pb, "", PRBUF_ROOM(pb));
	when ARMOR:
	    sp = arm_info[which].oi_name;
	    pb[0] = '\0';
            if (obj->o_label != NULL)
            {
                char tail[256];
                if (obj->o_flags & ISKNOW)
                    snprintf(tail, sizeof tail, "』と呼ぶ（%s）%s［%s%d］",
                             num(a_class[which] - obj->o_arm, 0, ARMOR), sp,
                             terse ? "" : "守備 ", 10 - obj->o_arm);
                else
                    snprintf(tail, sizeof tail, "』と呼ぶ%s", sp);
                inv_add_label(obj, "『", obj->o_label, tail, max_cells);
            }
            else if (obj->o_flags & ISKNOW)
            {
                snprintf(pb, PRBUF_ROOM(pb), "（%s）%s［",
                         num(a_class[which] - obj->o_arm, 0, ARMOR), sp);
                if (!terse)
                    jrogue_strlcat(pb, "守備 ", PRBUF_ROOM(pb));
                pb = &prbuf[strlen(prbuf)];
                snprintf(pb, PRBUF_ROOM(pb), "%d］", 10 - obj->o_arm);
            }
            else
                snprintf(pb, PRBUF_ROOM(pb), "%s", sp);

	when AMULET:
	    jrogue_strlcpy(pb, "イェンダーの魔除け", PRBUF_ROOM(pb));
	when GOLD:
	    snprintf(prbuf, PRBUF_SIZE, "%d個の金塊", obj->o_goldval);
#ifdef MASTER
	otherwise:
	    snprintf(pb, PRBUF_ROOM(pb), "何か変なもの「%s」", unctrl(obj->o_type));
#endif
    }
    if (inv_describe)
    {
	if (obj == cur_armor)
	    jrogue_strlcat(prbuf, "（着ている）", PRBUF_SIZE);
	if (obj == cur_weapon)
	    jrogue_strlcat(prbuf, "（使っている）", PRBUF_SIZE);
	if (obj == cur_ring[LEFT])
	    jrogue_strlcat(prbuf, "（左手）", PRBUF_SIZE);
	else if (obj == cur_ring[RIGHT])
	    jrogue_strlcat(prbuf, "（右手）", PRBUF_SIZE);
    }
    if (drop && isupper((int)prbuf[0]))
	prbuf[0] = (char) tolower(prbuf[0]);
    else if (!drop && islower((int)*prbuf))
	*prbuf = (char) toupper(*prbuf);
    prbuf[PRBUF_SIZE-1] = '\0';
    return prbuf;
}


/* A narrow inventory context (notably the winner's sale list) can use
 * fewer cells than the regular inventory.  Rebuild the label at that width
 * rather than clipping a finished line and losing item details. */
char *
inv_name_limited(const THING *obj, int drop, int max_cells)
{
    char original[PRBUF_SIZE];
    char base[PRBUF_SIZE];
    const char *marker;
    size_t name_bytes, marker_bytes;

    if (max_cells < 0)
        max_cells = 0;
    if (max_cells > INV_NAME_MAX_CELLS)
        max_cells = INV_NAME_MAX_CELLS;
    if (jrogue_lang_is_japanese)
        inv_name_ja(obj, drop, max_cells);
    else
        inv_name_en(obj, drop, max_cells);

    /* The ordinary inventory retains its previous layout.  Only a narrower
     * caller requests the final safety clip for fixed (non-label) text. */
    if (max_cells == INV_NAME_MAX_CELLS ||
        jrogue_text_width(prbuf) <= max_cells)
        return prbuf;

    /* Fixed descriptions (for example a long fruit name) can still exceed
     * this narrower context.  Preserve the equipment annotation as a suffix
     * when doing the final display-only clipping. */
    jrogue_strlcpy(original, prbuf, sizeof original);
    marker = inv_marker(obj);
    name_bytes = strlen(original);
    marker_bytes = strlen(marker);
    if (marker_bytes > name_bytes ||
        strcmp(original + name_bytes - marker_bytes, marker) != 0)
        marker = "";
    marker_bytes = strlen(marker);
    memcpy(base, original, name_bytes - marker_bytes);
    base[name_bytes - marker_bytes] = '\0';
    prbuf[0] = '\0';
    jrogue_text_append_labeled(prbuf, PRBUF_SIZE, max_cells,
                               "", base, marker);
    return prbuf;
}

char *
inv_name(const THING *obj, int drop)
{
    return inv_name_limited(obj, drop, INV_NAME_MAX_CELLS);
}

/*
 * drop:
 *	Put something down
 */

void
drop(void)
{
    int ch;
    THING *obj;

    ch = chat(hero.y, hero.x);
    if (ch != FLOOR && ch != PASSAGE)
    {
	after = FALSE;
	msg(JSTR("そこには、すでに何かが置かれている。", "there is something there already"));
	return;
    }
    if ((obj = get_item(JSTR("持ちものを落とす", "drop"), 0)) == NULL)
	return;
    if (!dropcheck(obj))
	return;
    obj = leave_pack(obj, TRUE, !ISMULT(obj->o_type));
    if (obj == NULL)
    {
        after = FALSE;
        return;
    }
    /*
     * Link it into the level object list
     */
    attach(lvl_obj, obj);
    chat(hero.y, hero.x) = obj->o_type;
    flat(hero.y, hero.x) |= F_DROPPED;
    obj->o_pos = hero;
    if (obj->o_type == AMULET)
	amulet = FALSE;
    msg(JSTR("%sを落とした。", "dropped %s"), inv_name(obj, TRUE));
}

/*
 * dropcheck:
 *	Do special checks for dropping or unweilding|unwearing|unringing
 */
int
dropcheck(const THING *obj)
{
    if (obj == NULL)
	return TRUE;
    if (obj != cur_armor && obj != cur_weapon
	&& obj != cur_ring[LEFT] && obj != cur_ring[RIGHT])
	    return TRUE;
    if (obj->o_flags & ISCURSED)
    {
	msg(JSTR("それはできない。その持ちものは呪われているようだ。", "you can't.  It appears to be cursed"));
	return FALSE;
    }
    if (obj == cur_weapon)
	cur_weapon = NULL;
    else if (obj == cur_armor)
    {
	waste_time();
	cur_armor = NULL;
    }
    else
    {
	cur_ring[obj == cur_ring[LEFT] ? LEFT : RIGHT] = NULL;
	switch (obj->o_which)
	{
	    case R_ADDSTR:
		chg_str(-obj->o_arm);
		break;
	    case R_SEEINVIS:
		unsee();
		extinguish(unsee);
		break;
	}
    }
    return TRUE;
}

/*
 * new_thing:
 *	Return a new thing
 */
THING *
new_thing(void)
{
    THING *cur;
    int r;

    cur = new_item();
    cur->o_hplus = 0;
    cur->o_dplus = 0;
    jrogue_strlcpy(cur->o_damage, "0x0", sizeof cur->o_damage);
    jrogue_strlcpy(cur->o_hurldmg, "0x0", sizeof cur->o_hurldmg);
    cur->o_arm = 11;
    cur->o_count = 1;
    cur->o_group = 0;
    cur->o_flags = 0;
    /*
     * Decide what kind of object it will be
     * If we haven't had food for a while, let it be food.
     */
    switch (no_food > 3 ? 2 : pick_one(things, NUMTHINGS))
    {
	case 0:
	    cur->o_type = POTION;
	    cur->o_which = pick_one(pot_info, MAXPOTIONS);
	when 1:
	    cur->o_type = SCROLL;
	    cur->o_which = pick_one(scr_info, MAXSCROLLS);
	when 2:
	    cur->o_type = FOOD;
	    no_food = 0;
	    if (rnd(10) != 0)
		cur->o_which = 0;
	    else
		cur->o_which = 1;
	when 3:
	    init_weapon(cur, pick_one(weap_info, MAXWEAPONS));
	    if ((r = rnd(100)) < 10)
	    {
		cur->o_flags |= ISCURSED;
		cur->o_hplus -= rnd(3) + 1;
	    }
	    else if (r < 15)
		cur->o_hplus += rnd(3) + 1;
	when 4:
	    cur->o_type = ARMOR;
	    cur->o_which = pick_one(arm_info, MAXARMORS);
	    cur->o_arm = a_class[cur->o_which];
	    if ((r = rnd(100)) < 20)
	    {
		cur->o_flags |= ISCURSED;
		cur->o_arm += rnd(3) + 1;
	    }
	    else if (r < 28)
		cur->o_arm -= rnd(3) + 1;
	when 5:
	    cur->o_type = RING;
	    cur->o_which = pick_one(ring_info, MAXRINGS);
	    switch (cur->o_which)
	    {
		case R_ADDSTR:
		case R_PROTECT:
		case R_ADDHIT:
		case R_ADDDAM:
		    if ((cur->o_arm = rnd(3)) == 0)
		    {
			cur->o_arm = -1;
			cur->o_flags |= ISCURSED;
		    }
		when R_AGGR:
		case R_TELEPORT:
		    cur->o_flags |= ISCURSED;
	    }
	when 6:
	    cur->o_type = STICK;
	    cur->o_which = pick_one(ws_info, MAXSTICKS);
	    fix_stick(cur);
#ifdef MASTER
	otherwise:
	    debug("Picked a bad kind of object");
	    wait_for(stdscr, ' ');
#endif
    }
    return cur;
}

/*
 * pick_one:
 *	Pick an item out of a list of nitems possible objects
 */
int
pick_one(const struct obj_info *info, int nitems)
{
    const struct obj_info *end;
    const struct obj_info *start;
    int i;

    start = info;
    for (end = &info[nitems], i = rnd(100); info < end; info++)
	if (i < info->oi_prob)
	    break;
    if (info == end)
    {
#ifdef MASTER
	if (wizard)
	{
	    msg("bad pick_one: %d from %d items", i, nitems);
	    for (info = start; info < end; info++)
		msg("%s: %d%%", info->oi_name, info->oi_prob);
	}
#endif
	info = start;
    }
    return (int)(info - start);
}

/*
 * discovered:
 *	list what the player has discovered in this game of a certain type
 */
static int line_cnt = 0;

static int newpage = FALSE;

static const char *lastfmt, *lastarg;


void
discovered(void)
{
    int ch;
    int disc_list;

    do {
	disc_list = FALSE;
	if (jrogue_lang_is_japanese)
	{
	    addmsg("どの種類");
	    if (!terse)
		addmsg("を表示しますか");
	    msg("？ ('*'ですべて、ESCでキャンセル)");
	}
	else
	{
	    if (!terse)
		addmsg("for ");
	    addmsg("what type");
	    if (!terse)
		addmsg(" of object do you want a list");
	    msg("? (* for all)");
	}
	ch = readchar();
	switch (ch)
	{
	    case ESCAPE:
		msg("");
		return;
	    case POTION:
	    case SCROLL:
	    case RING:
	    case STICK:
	    case '*':
		disc_list = TRUE;
		break;
	    default:
		if (terse)
		    msg(JSTR("それは表示できない。", "Not a type"));
		else if (jrogue_lang_is_japanese)
		    msg("%c、%c、%c、%c から選んでください。", POTION, SCROLL, RING, STICK);
		else
		    msg("Please type one of %c%c%c%c (ESCAPE to quit)", POTION, SCROLL, RING, STICK);
	}
    } while (!disc_list);
    if (ch == '*')
    {
	print_disc(POTION);
	add_line("", NULL);
	print_disc(SCROLL);
	add_line("", NULL);
	print_disc(RING);
	add_line("", NULL);
	print_disc(STICK);
	end_line();
    }
    else
    {
	print_disc(ch);
	add_line("", NULL);
	end_line();
    }
    msg("");
}

/*
 * print_disc:
 *	Print what we've discovered of type 'type'
 */

#define MAX4(a,b,c,d)	(a > b ? (a > c ? (a > d ? a : d) : (c > d ? c : d)) : (b > c ? (b > d ? b : d) : (c > d ? c : d)))


void
print_disc(int type)
{
    struct obj_info *info = NULL;
    int i, maxnum = 0, num_found;
    THING obj;
    int order[MAX4(MAXSCROLLS, MAXPOTIONS, MAXRINGS, MAXSTICKS)];

    switch (type)
    {
	case SCROLL:
	    maxnum = MAXSCROLLS;
	    info = scr_info;
	    break;
	case POTION:
	    maxnum = MAXPOTIONS;
	    info = pot_info;
	    break;
	case RING:
	    maxnum = MAXRINGS;
	    info = ring_info;
	    break;
	case STICK:
	    maxnum = MAXSTICKS;
	    info = ws_info;
	    break;
    }
    set_order(order, maxnum);
    obj.o_count = 1;
    obj.o_flags = 0;
    num_found = 0;
    for (i = 0; i < maxnum; i++)
	if (info[order[i]].oi_know || info[order[i]].oi_guess)
	{
	    obj.o_type = type;
	    obj.o_which = order[i];
	    add_line("%s", inv_name(&obj, FALSE));
	    num_found++;
	}
    if (num_found == 0)
	add_line(nothing(type), NULL);
}

/*
 * set_order:
 *	Set up order for list
 */

void
set_order(int *order, int numthings)
{
    int i, r, t;

    for (i = 0; i< numthings; i++)
	order[i] = i;

    for (i = numthings; i > 0; i--)
    {
	r = rnd(i);
	t = order[i - 1];
	order[i - 1] = order[r];
	order[r] = t;
    }
}

/*
 * add_line:
 *	Add a line to the list of discoveries
 */
/* VARARGS1 */
int
add_line(const char *fmt, const char *arg)
{
    WINDOW *tw;
    int x;
#ifdef _WIN32
    int overlay_shown;
#else
    WINDOW *sw;
    int y;
    char mystr[240];
#endif
    const char *prompt = JSTR("--スペースを押してください--", "--Press space to continue--");
    static int maxlen = -1;

    if (line_cnt == 0)
    {
	    wclear(hw);
	    jrogue_wset_normal_attr(hw);
	    if (inv_type == INV_SLOW)
		mpos = 0;
    }
    if (inv_type == INV_SLOW)
    {
	if (*fmt != '\0')
	    if (msg(fmt, arg) == ESCAPE)
		return ESCAPE;
	line_cnt++;
    }
    else
    {
	if (maxlen < 0)
	    maxlen = (int) mbstrlen(prompt);
	if (line_cnt >= LINES - 1 || fmt == NULL)
	{
#ifdef _WIN32
	    /*
             * Copy rendered curses cells, not text read back with mvwinnstr().
             * A wide-character curses implementation need not return UTF-8
             * bytes from mvwinnstr(), so converting cells to char* and
             * printing them again can truncate Japanese inventory names.
             * The original parent/subwindow pair also freed the parent while
             * the child still existed, which is unsafe on PDCurses.
             */
            overlay_shown = FALSE;
            if (inv_type == INV_OVER && fmt == NULL && !newpage &&
                line_cnt > 0 && maxlen > 0 &&
                maxlen + 3 <= COLS && line_cnt < LINES)
            {
                tw = newwin(line_cnt + 1, maxlen + 2,
                    0, COLS - maxlen - 3);
                if (tw != NULL)
                {
                    jrogue_wset_normal_attr(tw);
                    if (copywin(hw, tw, 0, 0, 0, 1,
                        line_cnt - 1, maxlen, FALSE) != ERR)
                    {
                        msg("");
                        refresh();
                        wmove(tw, line_cnt, 1);
                        waddstr(tw, prompt);
                        if (LINES > NUMLINES)
                        {
                            if (NUMLINES + line_cnt > LINES)
                                mvwin(tw, LINES - (line_cnt + 1),
                                    COLS - maxlen - 3);
                            else
                                mvwin(tw, NUMLINES, 0);
                        }
                        touchwin(tw);
                        wrefresh(tw);
                        jrogue_show_cursor(TRUE);
                        wait_for(tw, ' ');
                        jrogue_restore_cursor();
                        if (md_hasclreol())
                        {
                            werase(tw);
                            leaveok(tw, TRUE);
                            wrefresh(tw);
                        }
                        overlay_shown = TRUE;
                        touchwin(stdscr);
                    }
                    delwin(tw);
                }
            }
            if (!overlay_shown)
            {
                wmove(hw, LINES - 1, 0);
                waddstr(hw, prompt);
                wrefresh(hw);
                jrogue_show_cursor(TRUE);
                wait_for(hw, ' ');
                jrogue_restore_cursor();
                clearok(curscr, TRUE);
                wclear(hw);
                touchwin(stdscr);
            }
#else
	    if (inv_type == INV_OVER && fmt == NULL && !newpage)
	    {
		msg("");
		refresh();
		tw = newwin(line_cnt + 1, maxlen + 2, 0, COLS - maxlen - 3);
		sw = subwin(tw, line_cnt + 1, maxlen + 1, 0, COLS - maxlen - 2);
		jrogue_wset_normal_attr(tw);
		jrogue_wset_normal_attr(sw);
                for (y = 0; y <= line_cnt; y++) 
                { 
                    mvwinnstr(hw, y, 0, mystr, 240);
					mvwprintw(sw, y, 0, "%s", mystr);
                } 
		wmove(tw, line_cnt, 1);
		waddstr(tw, prompt);
		/*
		 * if there are lines below, use 'em
		 */
		if (LINES > NUMLINES)
		{
		    if (NUMLINES + line_cnt > LINES)
			mvwin(tw, LINES - (line_cnt + 1), COLS - maxlen - 3);
		    else
			mvwin(tw, NUMLINES, 0);
		}
		touchwin(tw);
		wrefresh(tw);
	jrogue_show_cursor(TRUE);
		wait_for(tw, ' ');
	jrogue_restore_cursor();
                if (md_hasclreol())
		{
		    werase(tw);
		    leaveok(tw, TRUE);
		    wrefresh(tw);
		}
		delwin(tw);
		touchwin(stdscr);
	    }
	    else
	    {
		wmove(hw, LINES - 1, 0);
		waddstr(hw, prompt);
		wrefresh(hw);
	jrogue_show_cursor(TRUE);
		wait_for(hw, ' ');
	jrogue_restore_cursor();
		clearok(curscr, TRUE);
		wclear(hw);
		touchwin(stdscr);
	    }
#endif
	    newpage = TRUE;
	    line_cnt = 0;
	    maxlen = (int) mbstrlen(prompt);
	}
	if (fmt != NULL && !(line_cnt == 0 && *fmt == '\0'))
	{
#ifdef _WIN32
	    {
		char linebuf[PRBUF_SIZE];
		(void)snprintf(linebuf, sizeof linebuf, fmt, arg);
		wmove(hw, line_cnt++, 0);
		jrogue_waddstr_utf8(hw, linebuf);
	    }
	    x = getcurx(hw);
#else
	    mvwprintw(hw, line_cnt++, 0, fmt, arg);
	    getyx(hw, y, x);
#endif
	    if (maxlen < x)
		maxlen = x;
	    lastfmt = fmt;
	    lastarg = arg;
	}
    }
    return ~ESCAPE;
}

/*
 * end_line:
 *	End the list of lines
 */

void
end_line(void)
{
    if (inv_type != INV_SLOW)
    {
	if (line_cnt == 1 && !newpage)
	{
	    mpos = 0;
	    msg(lastfmt, lastarg);
	}
	else
	    add_line(NULL, NULL);
    }
    line_cnt = 0;
    newpage = FALSE;
}

/*
 * nothing:
 *	Set up prbuf so that message for "nothing found" is there
 */
const char *
nothing(int type)
{
    char *sp;
    const char *tystr = NULL;

    if (terse)
	snprintf(prbuf, PRBUF_SIZE, "%s", JSTR("（知っている", "Nothing"));
    else
	snprintf(prbuf, PRBUF_SIZE, "%s", JSTR("（これまでに明らかにされた", "Haven't discovered anything"));
    if (type != '*')
    {
	sp = &prbuf[strlen(prbuf)];
	switch (type)
	{
	    case POTION: tystr = JSTR("水薬", "potion");
	    when SCROLL: tystr = JSTR("巻き物", "scroll");
	    when RING: tystr = JSTR("指輪", "ring");
	    when STICK: tystr = JSTR("つえや魔法の棒", "stick");
	}
	snprintf(sp, PRBUF_ROOM(sp), JSTR("%sはない）", " about any %ss"), tystr);
    }
    return prbuf;
}

/*
 * nameit:
 *	Give the proper name to a potion, stick, or ring
 */

static void
nameit_en(const THING *obj, const char *type, const char *which, const struct obj_info *op,
    const char *(*prfunc)(const THING *), int max_cells)
{
    char *pb;

    if (op->oi_know || op->oi_guess)
    {
	if (obj->o_count == 1)
	    snprintf(prbuf, PRBUF_SIZE, "A %s ", type);
	else
	    snprintf(prbuf, PRBUF_SIZE, "%d %ss ", obj->o_count, type);
	pb = &prbuf[strlen(prbuf)];
	if (op->oi_know)
	    snprintf(pb, PRBUF_ROOM(pb), "of %s%s(%s)", op->oi_name, (*prfunc)(obj), which);
        else if (op->oi_guess)
        {
            char tail[256];
            snprintf(tail, sizeof tail, "%s(%s)", (*prfunc)(obj), which);
            inv_add_label(obj, "called ", op->oi_guess, tail, max_cells);
        }
    }
    else if (obj->o_count == 1)
	snprintf(prbuf, PRBUF_SIZE, "A%s %s %s", vowelstr(which), which, type);
    else
	snprintf(prbuf, PRBUF_SIZE, "%d %s %ss", obj->o_count, which, type);
}

static void
nameit_ja(const THING *obj, const char *type, const char *which, const struct obj_info *op,
    const char *(*prfunc)(const THING *), int max_cells)
{
    char *pb;

    if (op->oi_know || op->oi_guess)
    {
	if (obj->o_count == 1)
	    prbuf[0] = '\0';
	else
	    snprintf(prbuf, PRBUF_SIZE, "%d個の", obj->o_count);
	pb = &prbuf[strlen(prbuf)];
	if (op->oi_know)
	    snprintf(pb, PRBUF_ROOM(pb), "%s%s%s（%s）", op->oi_name, type, (*prfunc)(obj), which);
        else if (op->oi_guess)
        {
            char tail[256];
            snprintf(tail, sizeof tail, "』と呼ぶ%s%s（%s）",
                     type, (*prfunc)(obj), which);
            inv_add_label(obj, "『", op->oi_guess, tail, max_cells);
        }
    }
    else if (obj->o_count == 1)
	snprintf(prbuf, PRBUF_SIZE, "%sの%s", which, type);
    else
	snprintf(prbuf, PRBUF_SIZE, "%d個の%sの%s", obj->o_count, which, type);
}

void
nameit(const THING *obj, const char *type, const char *which, const struct obj_info *op,
    const char *(*prfunc)(const THING *))
{
    if (jrogue_lang_is_japanese)
	nameit_ja(obj, type, which, op, prfunc, INV_NAME_MAX_CELLS);
    else
	nameit_en(obj, type, which, op, prfunc, INV_NAME_MAX_CELLS);
}

/*
 * nullstr:
 *	Return a pointer to a null-length string
 */
const char *
nullstr(const THING *ignored)
{
    (void)ignored;
    return "";
}

# ifdef	MASTER
/*
 * pr_list:
 *	List possible potions, scrolls, etc. for wizard.
 */

void
pr_list(void)
{
    int ch;

//    if (!terse)
//	addmsg("for ");
    addmsg(JSTR("どのタイプ", "what type"));
    if (!terse)
	addmsg(JSTR("の物をリストにする", " of object do you want a list"));
    msg(JSTR("？", "? "));
    ch = readchar();
	msg("");
    switch (ch)
    {
	case POTION:
	    pr_spec(pot_info, MAXPOTIONS);
	when SCROLL:
	    pr_spec(scr_info, MAXSCROLLS);
	when RING:
	    pr_spec(ring_info, MAXRINGS);
	when STICK:
	    pr_spec(ws_info, MAXSTICKS);
	when ARMOR:
	    pr_spec(arm_info, MAXARMORS);
	when WEAPON:
	    pr_spec(weap_info, MAXWEAPONS);
	otherwise:
	    return;
    }
}

/*
 * pr_spec:
 *	Print specific list of possible items to choose from
 */

void
pr_spec(const struct obj_info *info, int nitems)
{
    const struct obj_info *endp;
    int i, lastprob;

    endp = &info[nitems];
    lastprob = 0;
    for (i = '0'; info < endp; i++)
    {
	if (i == '9' + 1)
	    i = 'a';
	snprintf(prbuf, PRBUF_SIZE, "%c: %%s (%d%%%%)", i, info->oi_prob - lastprob);
	lastprob = info->oi_prob;
	add_line(prbuf, info->oi_name);
	info++;
    }
    end_line();
}
# endif	/* MASTER */
