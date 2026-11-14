/*
 * Routines to deal with the pack
 *
 * @(#)pack.c	4.40 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <string.h>
#include <stdio.h>
#include <curses.h>
#include <ctype.h>
#include "rogue.h"

/*
 * update_mdest:
 *      Called after picking up an object, before discarding it.
 *      If this was the object of something's desire, that monster will
 *      get mad and run at the hero
 */
void
update_mdest(THING *obj)
{
    register THING *mp;

    for (mp = mlist; mp != NULL; mp = next(mp))
        if (mp->t_dest == &obj->o_pos)
     mp->t_dest = &hero;
}

/*
 * add_pack:
 *	Pick up an object and add it to the pack.  If the argument is
 *	non-null use it as the linked_list pointer instead of gettting
 *	it off the ground.
 */

void
add_pack(THING *obj, int silent)
{
    THING *op, *lp;
    int from_floor;
    int discarded = 0;

    from_floor = FALSE;
    if (obj == NULL)
    {
	if ((obj = find_obj(hero.y, hero.x)) == NULL)
	    return;
	from_floor = TRUE;
    }

    /*
     * Check for and deal with scare monster scrolls
     */
    if (obj->o_type == SCROLL && obj->o_which == S_SCARE)
	if (obj->o_flags & ISFOUND)
	{
	    detach(lvl_obj, obj);
	    jrogue_mvaddch(hero.y, hero.x, floor_ch());
	    chat(hero.y, hero.x) = (proom->r_flags & ISGONE) ? PASSAGE : FLOOR;
            update_mdest(obj);
            discarded = 1;
	    discard_object(obj);
	    msg(JSTR("拾いあげたとたん、巻き物はちりになってしまった。", "the scroll turns to dust as you pick it up"));
	    return;
	}

    /*
     * JEC safety fix relative to RRP Rogue 5.4.5: when the pack was
     * empty, the original code made a floor object the first pack node
     * without detaching it from lvl_obj.  That could leave the same node
     * in both linked lists.  Run pack_room() first to detach floor items
     * and check the pack limit, then clear the old links before insertion.
     */
    if (pack == NULL)
    {
	if (!pack_room(from_floor, obj))
	    return;
	pack = obj;
	prev(obj) = NULL;
	next(obj) = NULL;
	obj->o_packch = pack_char();
    }
    else
    {
	lp = NULL;
	for (op = pack; op != NULL; op = next(op))
	{
	    if (op->o_type != obj->o_type)
		lp = op;
	    else
	    {
		while (op->o_type == obj->o_type && op->o_which != obj->o_which)
		{
		    lp = op;
		    if (next(op) == NULL)
			break;
		    else
			op = next(op);
		}
		if (op->o_type == obj->o_type && op->o_which == obj->o_which)
		{
		    if (ISMULT(op->o_type))
		    {
			if (!pack_room(from_floor, obj))
			    return;
			op->o_count++;
dump_it:
			update_mdest(obj);
			discard_object(obj);
			obj = op;
                        discarded = 1;
			lp = NULL;
			goto out;
		    }
		    else if (obj->o_group)
		    {
			lp = op;
			while (op->o_type == obj->o_type
			    && op->o_which == obj->o_which
			    && op->o_group != obj->o_group)
			{
			    lp = op;
			    if (next(op) == NULL)
				break;
			    else
				op = next(op);
			}
			if (op->o_type == obj->o_type
			    && op->o_which == obj->o_which
			    && op->o_group == obj->o_group)
			{
				op->o_count += obj->o_count;
				inpack--;
				if (!pack_room(from_floor, obj))
				    return;
				goto dump_it;
			}
		    }
		    else
			lp = op;
		}
out:
		break;
	    }
	}

	if (lp != NULL)
	{
	    if (!pack_room(from_floor, obj))
		return;
	    else
	    {
		obj->o_packch = pack_char();
		next(obj) = next(lp);
		prev(obj) = lp;
		if (next(lp) != NULL)
		    prev(next(lp)) = obj;
		next(lp) = obj;
	    }
	}
    }

    obj->o_flags |= ISFOUND;

    /*
     * If this was the object of something's desire, that monster will
     * get mad and run at the hero.
     */
    if (!discarded)
        update_mdest(obj);

    if (obj->o_type == AMULET)
	amulet = TRUE;
    /*
     * Notify the user
     */
    if (!silent)
    {
if (jrogue_lang_is_japanese)
	{
	    if (!terse)
		addmsg("あなたは");
	    msg("%s (%c)を手に入れた。", inv_name(obj, !terse), obj->o_packch);
	}
	else
	{
	    if (!terse)
		addmsg("you now have ");
	    msg("%s (%c)", inv_name(obj, !terse), obj->o_packch);
	}
    }
}

/*
 * pack_room:
 *	See if there's room in the pack.  If not, print out an
 *	appropriate message
 */
int
pack_room(int from_floor, THING *obj)
{
    if (++inpack > MAXPACK)
    {
if (jrogue_lang_is_japanese)
	{
	    if (!terse)
		addmsg("もう");
	    addmsg("これ以上、物は持てない。");
	}
	else
	{
	    if (!terse)
		addmsg("there's ");
	    addmsg("no room");
	    if (!terse)
		addmsg(" in your pack");
	}
	endmsg();
	if (from_floor)
	    move_msg(obj);
	inpack = MAXPACK;
	return FALSE;
    }

    if (from_floor)
    {
	detach(lvl_obj, obj);
	jrogue_mvaddch(hero.y, hero.x, floor_ch());
	chat(hero.y, hero.x) = (proom->r_flags & ISGONE) ? PASSAGE : FLOOR;
    }

    return TRUE;
}

/*
 * leave_pack:
 *	take an item out of the pack
 */
THING *
leave_pack(THING *obj, int newobj, int all)
{
    THING *nobj;

    nobj = obj;
    if (obj->o_count > 1 && !all && newobj)
    {
        /* Do not change the pack until both the split node and its
         * independent call name have been allocated successfully. */
        nobj = copy_object(obj);
        if (nobj == NULL)
        {
            msg(JSTR("メモリが不足して持ちものを分けられない。",
                     "not enough memory to split the item"));
            return NULL;
        }
        nobj->o_count = 1;
    }

    inpack--;
    if (obj->o_count > 1 && !all)
    {
        last_pick = obj;
        obj->o_count--;
        if (obj->o_group)
            inpack++;
    }
    else
    {
        last_pick = NULL;
        pack_used[obj->o_packch - 'a'] = FALSE;
        detach(pack, obj);
    }
    return nobj;
}

/*
 * pack_char:
 *	Return the next unused pack character.
 */
int
pack_char(void)
{
    int *bp;

    for (bp = pack_used; *bp; bp++)
	continue;
    *bp = TRUE;
    return ((int)(bp - pack_used) + 'a');
}

/*
 * inventory:
 *	List what is in the pack.  Return TRUE if there is something of
 *	the given type.
 */
int
inventory(const THING *list, int type)
{
    static char inv_temp[MAXSTR];

    n_objs = 0;
    for (; list != NULL; list = next(list))
    {
	if (type && type != list->o_type && !(type == CALLABLE &&
	    list->o_type != FOOD && list->o_type != AMULET) &&
	    !(type == R_OR_S && (list->o_type == RING || list->o_type == STICK)))
		continue;
	n_objs++;
#ifdef MASTER
	if (!list->o_packch)
	    jrogue_strlcpy(inv_temp, "%s", sizeof inv_temp);
	else
#endif
	    snprintf(inv_temp, sizeof inv_temp, "%c) %%s", list->o_packch);
	msg_esc = TRUE;
	if (add_line(inv_temp, inv_name(list, FALSE)) == ESCAPE)
	{
	    msg_esc = FALSE;
	    msg("");
	    return TRUE;
	}
	msg_esc = FALSE;
    }
    if (n_objs == 0)
    {
	if (terse)
	    msg(type == 0 ? JSTR("何も持っていない。", "empty handed") :
			    JSTR("それは１つも持っていない。", "nothing appropriate"));
	else
	    msg(type == 0 ? JSTR("何も持っていない。", "you are empty handed") :
			    JSTR("それは１つも持っていない。", "you don't have anything appropriate"));
	return FALSE;
    }
    end_line();
    return TRUE;
}

/*
 * pick_up:
 *	Add something to characters pack.
 */

void
pick_up(int ch)
{
    THING *obj;

    if (on(player, ISLEVIT))
	return;

    obj = find_obj(hero.y, hero.x);
    if (move_on)
	move_msg(obj);
    else
	switch (ch)
	{
	    case GOLD:
		if (obj == NULL)
		    return;
		money(obj->o_goldval);
		detach(lvl_obj, obj);
		update_mdest(obj);
		discard_object(obj);
		proom->r_goldval = 0;
		break;
	    default:
#ifdef MASTER
		debug(JSTR("どこで'%s'なんかを拾ったの？？？", "Where did you pick a '%s' up???"), unctrl(ch));
#endif
		/* Unknown types still use the normal item pickup path. */
		add_pack(NULL, FALSE);
		break;
	    case ARMOR:
	    case POTION:
	    case FOOD:
	    case WEAPON:
	    case SCROLL:	
	    case AMULET:
	    case RING:
	    case STICK:
		add_pack(NULL, FALSE);
		break;
	}
}

/*
 * move_msg:
 *	Print out the message if you are just moving onto an object
 */

void
move_msg(const THING *obj)
{
    if (!terse)
	addmsg(JSTR("あなたは", "you "));
    msg(JSTR("%sの上にいる。", "moved onto %s"), inv_name(obj, TRUE));
}

/*
 * picky_inven:
 *	Allow player to inventory a single item
 */

void
picky_inven(void)
{
    THING *obj;
    int mch;

    if (pack == NULL)
	msg(JSTR("何も持っていない。", "you aren't carrying anything"));
    else if (next(pack) == NULL)
	msg("a) %s", inv_name(pack, FALSE));
    else
    {
	msg(terse ? JSTR("調べる持ちものは？", "item: ") : JSTR("調べる持ちものの種類は？", "which item do you wish to inventory: "));
	mpos = 0;
	if ((mch = readchar()) == ESCAPE)
	{
	    msg("");
	    return;
	}
	for (obj = pack; obj != NULL; obj = next(obj))
	    if (mch == obj->o_packch)
	    {
		msg("%c) %s", mch, inv_name(obj, FALSE));
		return;
	    }
	msg(JSTR("'%s'は持っていない。", "'%s' not in pack"), unctrl(mch));
    }
}

/*
 * get_item:
 *	Pick something out of a pack for a purpose
 */
THING *
get_item(const char *purpose, int type)
{
    THING *obj;
    int ch;

    if (pack == NULL)
	msg(JSTR("何も持っていない。", "you aren't carrying anything"));
    else if (again)
	if (last_pick)
	    return last_pick;
	else
	    msg(JSTR("使い果たした。", "you ran out"));
    else
    {
	for (;;)
	{
	    if (jrogue_lang_is_japanese)
	    {
		addmsg("どの");
		addmsg("%s", purpose);
		if (!terse)
		    msg("？ ('*'でリスト表示、ESCでキャンセル) ");
		else
		    msg("？ ('*'でリスト表示)");
	    }
	    else
	    {
		if (!terse)
		    addmsg("which object do you want to ");
		addmsg("%s", purpose);
		if (terse)
		    addmsg(" what");
		msg("? (* for list): ");
	    }
	jrogue_show_cursor(TRUE);
	    ch = readchar();
	    mpos = 0;
	    /*
	     * Give the poor player a chance to abort the command
	     */
	    if (ch == ESCAPE)
	    {
		reset_last();
		after = FALSE;
		msg("");
	jrogue_restore_cursor();
		return NULL;
	    }
	    n_objs = 1;		/* normal case: person types one char */
	    if (ch == '*')
	    {
		mpos = 0;
		if (inventory(pack, type) == 0)
		{
		    after = FALSE;
	jrogue_restore_cursor();
		    return NULL;
		}
		continue;
	    }
	    for (obj = pack; obj != NULL; obj = next(obj))
		if (obj->o_packch == ch)
		    break;
	    if (obj == NULL)
	    {
		msg(JSTR("'%s' それはできない。", "'%s' is not a valid item"),unctrl(ch));
	jrogue_restore_cursor();
		continue;
	    }
	    else {
		msg("");
	jrogue_restore_cursor();
		return obj;
	    }
	}
    }
	jrogue_restore_cursor();
    return NULL;
}

/*
 * money:
 *	Add or subtract gold from the pack
 */

void
money(int value)
{
    purse = jrogue_sat_money_add(purse, value);
    jrogue_mvaddch(hero.y, hero.x, floor_ch());
    chat(hero.y, hero.x) = (proom->r_flags & ISGONE) ? PASSAGE : FLOOR;
    if (value > 0)
    {
    if (!terse)
	    addmsg(JSTR("あなたは", "you found "));
	msg(JSTR("%d個の金塊を見つけた。", "%d gold pieces"), value);
    }
}

/*
 * floor_ch:
 *	Return the appropriate floor character for her room
 */
int
floor_ch(void)
{
    if (proom->r_flags & ISGONE)
	return PASSAGE;
    return (show_floor() ? FLOOR : ' ');
}

/*
 * floor_at:
 *	Return the character at hero's position, taking see_floor
 *	into account
 */
int
floor_at(void)
{
    int ch;

    ch = chat(hero.y, hero.x);
    if (ch == FLOOR)
	ch = floor_ch();
    return ch;
}

/*
 * reset_last:
 *	Reset the last command when the current one is aborted
 */

void
reset_last(void)
{
    last_comm = l_last_comm;
    last_dir = l_last_dir;
    last_pick = l_last_pick;
}
