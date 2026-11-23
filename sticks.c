/*
 * Functions to implement the various sticks one might find
 * while wandering around the dungeon.
 *
 * @(#)sticks.c	4.39 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "rogue.h"

/*
 * fix_stick:
 *	Set up a new stick
 */

void
fix_stick(THING *cur)
{
    if (strcmp(ws_type[cur->o_which], JSTR("つえ", "staff")) == 0)
	jrogue_strlcpy(cur->o_damage, "2x3", sizeof cur->o_damage);
    else
	jrogue_strlcpy(cur->o_damage, "1x1", sizeof cur->o_damage);
    jrogue_strlcpy(cur->o_hurldmg, "1x1", sizeof cur->o_hurldmg);

    switch (cur->o_which)
    {
	case WS_LIGHT:
	    cur->o_charges = rnd(10) + 10;
	otherwise:
	    cur->o_charges = rnd(5) + 3;
    }
}

/*
 * do_zap:
 *	Perform a zap with a wand
 */

void
do_zap(void)
{
    THING *obj, *tp;
    int y, x;
    const char *name;
    int monster, oldch;
    THING bolt = {0};

    if ((obj = get_item(JSTR("つえを振りかざす", "zap with"), STICK)) == NULL)
	return;
    if (obj->o_type != STICK)
    {
	after = FALSE;
	msg(JSTR("それを振りかざすことはできない。", "you can't zap with that!"));
	return;
    }
    if (obj->o_charges == 0)
    {
	msg(JSTR("何も起こらなかった。", "nothing happens"));
	return;
    }
    switch (obj->o_which)
    {
	case WS_LIGHT:
	    /*
	     * Reddy Kilowat wand.  Light up the room
	     */
	    ws_info[WS_LIGHT].oi_know = TRUE;
	    if (proom->r_flags & ISGONE)
		msg(JSTR("通路が輝き、しばらくして光は消えていった。", "the corridor glows and then fades"));
	    else
	    {
		proom->r_flags &= ~ISDARK;
		/*
		 * Light the room and put the player back up
		 */
		enter_room(&hero);
		if (!terse)
		    addmsg(JSTR("%sに輝く", "the room is lit by a shimmering %s light"),
			pick_color(JSTR("青色", "blue")));
		if (terse || jrogue_lang_is_japanese)
		    addmsg(JSTR("光が部屋を照らしている。", "the room is lit"));
		endmsg();
	    }
	when WS_DRAIN:
	    /*
	     * take away 1/2 of hero's hit points, then take it away
	     * evenly from the monsters in the room (or next to hero
	     * if he is in a passage)
	     */
	    if (pstats.s_hpt < 2)
	    {
		msg(JSTR("それを使うための体力がない。", "you are too weak to use it"));
		return;
	    }
	    else
		drain();
	when WS_INVIS:
	case WS_POLYMORPH:
	case WS_TELAWAY:
	case WS_TELTO:
	case WS_CANCEL:
	    y = hero.y;
	    x = hero.x;
	    while (step_ok(winat(y, x)))
	    {
		y += delta.y;
		x += delta.x;
	    }
	    if ((tp = moat(y, x)) != NULL)
	    {
		monster = tp->t_type;
		if (monster == 'F')
		    player.t_flags &= ~ISHELD;
		switch (obj->o_which) {
		    case WS_INVIS:
			tp->t_flags |= ISINVIS;
			if (cansee(y, x))
			    jrogue_mvaddch(y, x, tp->t_oldch);
			break;
		    case WS_POLYMORPH:
		    {
			THING *pp;

			pp = tp->t_pack;
			detach(mlist, tp);
			if (see_monst(tp))
			    jrogue_mvaddch(y, x, chat(y, x));
			oldch = tp->t_oldch;
			delta.y = y;
			delta.x = x;
			new_monster(tp, monster = rnd(26) + 'A', &delta);
			if (see_monst(tp))
			    jrogue_mvaddch(y, x, monster);
			tp->t_oldch = oldch;
			tp->t_pack = pp;
			ws_info[WS_POLYMORPH].oi_know |= see_monst(tp);
			break;
		    }
		    case WS_CANCEL:
			tp->t_flags |= ISCANC;
			tp->t_flags &= ~(ISINVIS|CANHUH);
			tp->t_disguise = tp->t_type;
			if (see_monst(tp))
			    jrogue_mvaddch(y, x, tp->t_disguise);
			break;
		    case WS_TELAWAY:
		    case WS_TELTO:
                    {
			coord new_pos;

			if (obj->o_which == WS_TELAWAY)
			{
			    do
			    {
				find_floor(NULL, &new_pos, FALSE, TRUE);
			    } while (ce(new_pos, hero));
			}
			else
			{
			    new_pos.y = hero.y + delta.y;
			    new_pos.x = hero.x + delta.x;
			}
			tp->t_dest = &hero;
			tp->t_flags |= ISRUN;
			relocate(tp, &new_pos);
		    }
		}
	    }
	when WS_MISSILE:
	    ws_info[WS_MISSILE].oi_know = TRUE;
	    bolt.o_type = '*';
	    jrogue_strlcpy(bolt.o_hurldmg, "1x4", sizeof bolt.o_hurldmg);
	    bolt.o_hplus = 100;
	    bolt.o_dplus = 1;
	    bolt.o_flags = ISMISL;
	    /* A missile without a launcher uses o_hurldmg.  The original
	     * left o_launch (and o_damage) uninitialized when unarmed. */
	    bolt.o_launch = -1;
	    if (cur_weapon != NULL)
		bolt.o_launch = cur_weapon->o_which;
	    do_motion(&bolt, delta.y, delta.x);
	    if ((tp = moat(bolt.o_pos.y, bolt.o_pos.x)) != NULL
		&& !save_throw(VS_MAGIC, tp))
		    hit_monster(unc(bolt.o_pos), &bolt);
	    else if (terse)
		msg(JSTR("ミサイルは消えた。", "missile vanishes"));
	    else
		msg(JSTR("ミサイルは煙とともに消えてしまった。", "the missile vanishes with a puff of smoke"));
	when WS_HASTE_M:
	case WS_SLOW_M:
	    y = hero.y;
	    x = hero.x;
	    while (step_ok(winat(y, x)))
	    {
		y += delta.y;
		x += delta.x;
	    }
	    if ((tp = moat(y, x)) != NULL)
	    {
		if (obj->o_which == WS_HASTE_M)
		{
		    if (on(*tp, ISSLOW))
			tp->t_flags &= ~ISSLOW;
		    else
			tp->t_flags |= ISHASTE;
		}
		else
		{
		    if (on(*tp, ISHASTE))
			tp->t_flags &= ~ISHASTE;
		    else
			tp->t_flags |= ISSLOW;
		    tp->t_turn = TRUE;
		}
		delta.y = y;
		delta.x = x;
		runto(&delta);
	    }
	when WS_ELECT:
	case WS_FIRE:
	case WS_COLD:
	    if (obj->o_which == WS_ELECT)
		name = JSTR("いなずま", "bolt");
	    else if (obj->o_which == WS_FIRE)
		name = JSTR("炎", "flame");
	    else
		name = JSTR("冷気", "ice");
	    fire_bolt(&hero, &delta, name);
	    ws_info[obj->o_which].oi_know = TRUE;
	when WS_NOP:
	    break;
#ifdef MASTER
	otherwise:
	    msg(JSTR("なんておかしな杖なんだ！", "what a bizarre schtick!"));
#endif
    }
    obj->o_charges--;
}

/*
 * drain:
 *	Do drain hit points from player shtick
 */

void
drain(void)
{
    THING *mp;
    struct room *corp;
    THING **dp, **drainee;
    int cnt, ntargets;
    int inpass;

    /*
     * Count the targets first.  The original fixed 40-pointer array
     * overflowed with 40 or more targets (including its NULL terminator).
     * Keep a snapshot of all targets before killed() changes mlist.
     */
    cnt = 0;
    if (chat(hero.y, hero.x) == DOOR)
	corp = &passages[flat(hero.y, hero.x) & F_PNUM];
    else
	corp = NULL;
    inpass = (proom->r_flags & ISGONE);
    for (mp = mlist; mp != NULL; mp = next(mp))
	if (mp->t_room == proom || mp->t_room == corp ||
	    (inpass && chat(mp->t_pos.y, mp->t_pos.x) == DOOR &&
	    &passages[flat(mp->t_pos.y, mp->t_pos.x) & F_PNUM] == proom))
		cnt++;
    if (cnt == 0)
    {
	msg(JSTR("チクチクする感覚におそわれた。", "you have a tingling feeling"));
	return;
    }
    ntargets = cnt;
    drainee = malloc((size_t)ntargets * sizeof *drainee);
    if (drainee == NULL)
    {
	msg(JSTR("記憶領域が足りない。", "not enough memory"));
	return;
    }
    dp = drainee;
    for (mp = mlist; mp != NULL; mp = next(mp))
	if (mp->t_room == proom || mp->t_room == corp ||
	    (inpass && chat(mp->t_pos.y, mp->t_pos.x) == DOOR &&
	    &passages[flat(mp->t_pos.y, mp->t_pos.x) & F_PNUM] == proom))
		*dp++ = mp;

    pstats.s_hpt /= 2;
    cnt = pstats.s_hpt / cnt;
    /*
     * Divide the same amount of damage among the selected monsters,
     * in their original list order.  No sentinel is needed.
     */
    for (dp = drainee; dp < drainee + ntargets; dp++)
    {
	mp = *dp;
	if ((mp->t_stats.s_hpt -= cnt) <= 0)
	    killed(mp, see_monst(mp));
	else
	    runto(&mp->t_pos);
    }
    free(drainee);
}

/*
 * fire_bolt:
 *	Fire a bolt in a given direction from a specific starting place
 */

void
fire_bolt(const coord *start, coord *dir, const char *name)
{
    int step, saved_count, i;
    THING *tp;
    int dirch = 0, ch;
    int hit_hero, used, changed;
    int beam_pair;
    coord pos;
    coord spotpos[BOLT_LENGTH];
    chtype oldch[BOLT_LENGTH];
    THING bolt = {0};

    bolt.o_type = WEAPON;
    bolt.o_launch = -1;  /* select o_hurldmg for reflected flame */
    bolt.o_which = FLAME;
    jrogue_strlcpy(bolt.o_hurldmg, "6x6", sizeof bolt.o_hurldmg);
    bolt.o_hplus = 100;
    bolt.o_dplus = 0;
    weap_info[FLAME].oi_name = name;
    if (strcmp(name, JSTR("炎", "flame")) == 0)
        beam_pair = JROGUE_PAIR_PROJECTILE_RED;
    else if (strcmp(name, JSTR("いなずま", "bolt")) == 0)
        beam_pair = JROGUE_PAIR_PROJECTILE_YELLOW;
    else
        beam_pair = JROGUE_PAIR_PROJECTILE_CYAN;
    switch (dir->y + dir->x)
    {
	case 0: dirch = '/';
	when 1: case -1: dirch = (dir->y == 0 ? '-' : '|');
	when 2: case -2: dirch = '\\';
    }
    pos = *start;
    hit_hero = (start != &hero);
    used = FALSE;
    changed = FALSE;
    saved_count = 0;
    /* Count non-wall beam steps, as in the original.  Using an integer
     * permits a bounce on the very first step without making a pointer
     * one element before spotpos[]. */
    for (step = 0; step < BOLT_LENGTH && !used; step++)
    {
	pos.y += dir->y;
	pos.x += dir->x;
	ch = winat(pos.y, pos.x);
	switch (ch)
	{
	    case DOOR:
		/*
		 * this code is necessary if the hero is on a door
		 * and he fires at the wall the door is in, it would
		 * otherwise loop infinitely
		 */
		if (ce(hero, pos))
		    goto def;
		/* FALLTHROUGH */
	    case '|':
	    case '-':
	    case ' ':
		if (!changed)
		    hit_hero = !hit_hero;
		changed = FALSE;
		dir->y = -dir->y;
		dir->x = -dir->x;
		step--;	/* retry this beam step after the bounce */
		msg(JSTR("%sは、はねかえった。", "the %s bounces"), name);
		break;
	    default:
def:
		if (!hit_hero && (tp = moat(pos.y, pos.x)) != NULL)
		{
		    hit_hero = TRUE;
		    changed = !changed;
		    tp->t_oldch = chat(pos.y, pos.x);
		    if (!save_throw(VS_MAGIC, tp))
		    {
			bolt.o_pos = pos;
			used = TRUE;
			if (tp->t_type == 'D' && strcmp(name, JSTR("炎", "flame")) == 0)
			{
			    if (jrogue_lang_is_japanese)
				addmsg(terse ? "炎ははねかえった。" :
				    "炎はドラゴンにはねかえった。");
			    else
			    {
				addmsg("the flame bounces");
				if (!terse)
				    addmsg(" off the dragon");
			    }
			    endmsg();
			}
			else
			    hit_monster(unc(pos), &bolt);
		    }
		    else if (ch != 'M' || tp->t_disguise == 'M')
		    {
			if (start == &hero)
			    runto(&pos);
			if (terse)
			    msg(JSTR("%sは外れた。", "%s misses"), name);
			else
			    msg(JSTR("%sは、%sをかすめて通り過ぎた。", "the %s whizzes past %s"), name, set_mname(tp));
		    }
		}
		else if (hit_hero && ce(pos, hero))
		{
		    hit_hero = FALSE;
		    changed = !changed;
		    if (!save(VS_MAGIC))
		    {
			if ((pstats.s_hpt -= roll(6, 6)) <= 0)
			{
			    if (start == &hero)
				death('b');
			    else
				death(moat(start->y, start->x)->t_type);
			}
			used = TRUE;
			if (terse)
			    msg(JSTR("%sが当たった。", "the %s hits"), name);
			else
			    msg(JSTR("%sによる攻撃が命中した。", "you are hit by the %s"), name);
		    }
		    else
			msg(JSTR("%sがかすめて通り過ぎた。", "the %s whizzes by you"), name);
		}
		/* A reflected ray may visit the same cell more than once.
		 * Save the original display only on its first visit; otherwise
		 * a previously drawn ray glyph would be restored as a ghost. */
		for (i = 0; i < saved_count; i++)
		    if (ce(spotpos[i], pos))
			break;
		if (i == saved_count)
		{
		    spotpos[saved_count] = pos;
		    oldch[saved_count] = mvinch(pos.y, pos.x);
		    saved_count++;
		}
		mvaddch_projectile(pos.y, pos.x, dirch, beam_pair);
		msleep(50);
		jrogue_restore_cursor();
		jrogue_refresh_window(stdscr);
	}
    }
    for (i = 0; i < saved_count; i++)
	mvaddch(spotpos[i].y, spotpos[i].x, oldch[i]);
    jrogue_set_normal_attr();
}

/*
 * charge_str:
 *	Return an appropriate string for a wand charge
 */
const char *
charge_str(const THING *obj)
{
    static char buf[64];

    if (!(obj->o_flags & ISKNOW))
	buf[0] = '\0';
    else if (terse)
	snprintf(buf, sizeof buf, JSTR("［%d］", " [%d]"), obj->o_charges);
    else
	snprintf(buf, sizeof buf, JSTR("［残り %d］", " [%d charges]"), obj->o_charges);
    return buf;
}
