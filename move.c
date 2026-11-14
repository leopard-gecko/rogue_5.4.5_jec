/*
 * hero movement commands
 *
 * @(#)move.c	4.49 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <curses.h>
#include <ctype.h>
#include <string.h>
#include "rogue.h"

/*
 * Remember the direction at each square during one continuous run.
 * A passage loop can otherwise keep passgo running indefinitely.
 * One byte per map square covers the eight possible directions.
 */
static unsigned char run_visited[NUMLINES][NUMCOLS];

static unsigned char
run_direction_bit(int ch)
{
    switch (ch)
    {
        case 'h': return 1U << 0;
        case 'j': return 1U << 1;
        case 'k': return 1U << 2;
        case 'l': return 1U << 3;
        case 'y': return 1U << 4;
        case 'u': return 1U << 5;
        case 'b': return 1U << 6;
        case 'n': return 1U << 7;
    }
    return 0;
}

/*
 * do_run:
 *	Start the hero running
 */

void
do_run(int ch)
{
    /* A fresh run must not inherit the previous run's visited squares. */
    memset(run_visited, 0, sizeof run_visited);
    running = TRUE;
    after = FALSE;
    runch = ch;
}

/*
 * do_move:
 *	Check to see that a move is legal.  If it is handle the
 * consequences (fighting, picking up, etc.)
 */

void
do_move(int dy, int dx)
{
    int ch, fl;
    coord nh;

    firstmove = FALSE;
    if (no_move)
    {
	no_move--;
	msg(JSTR("とらばさみのわなでまだ身動きがとれない。", "you are still stuck in the bear trap"));
	return;
    }
    if (running)
    {
        unsigned char bit = run_direction_bit(runch);

        /* Stop before repeating a square and heading: no extra game turn. */
        if (bit != 0 && hero.y > 0 && hero.y < NUMLINES - 1
            && hero.x >= 0 && hero.x < NUMCOLS)
        {
            if (run_visited[hero.y][hero.x] & bit)
            {
                running = FALSE;
                after = FALSE;
                return;
            }
            run_visited[hero.y][hero.x] |= bit;
        }
    }
    /*
     * Do a confused move (maybe)
     */
    if (on(player, ISHUH) && rnd(5) != 0)
    {
	nh = rndmove(&player);
	if (ce(nh, hero))
	{
	    after = FALSE;
	    running = FALSE;
	    to_death = FALSE;
	    return;
	}
    }
    else
    {
over:
	nh.y = hero.y + dy;
	nh.x = hero.x + dx;
    }

    /*
     * Check if he tried to move off the screen or make an illegal
     * diagonal move, and stop him if he did.
     */
    if (nh.x < 0 || nh.x >= NUMCOLS || nh.y <= 0 || nh.y >= NUMLINES - 1)
	goto hit_bound;
    if (!diag_ok(&hero, &nh))
    {
	after = FALSE;
	running = FALSE;
	return;
    }
    if (running && ce(hero, nh))
	after = running = FALSE;
    fl = flat(nh.y, nh.x);
    ch = winat(nh.y, nh.x);
    if (!(fl & F_REAL) && ch == FLOOR)
    {
	if (!on(player, ISLEVIT))
	{
	    chat(nh.y, nh.x) = ch = TRAP;
	    flat(nh.y, nh.x) |= F_REAL;
	}
    }
    else if (on(player, ISHELD) && ch != 'F')
    {
	msg(JSTR("怪物につかまえられている。", "you are being held"));
	return;
    }
    switch (ch)
    {
	case ' ':
	case '|':
	case '-':
hit_bound:
	    if (passgo && running && (proom->r_flags & ISGONE)
		&& !on(player, ISBLIND))
	    {
		int	b1, b2;

		switch (runch)
		{
		    case 'h':
		    case 'l':
			b1 = (hero.y != 1 && turn_ok(hero.y - 1, hero.x));
			b2 = (hero.y != NUMLINES - 2 && turn_ok(hero.y + 1, hero.x));
			if (!(b1 ^ b2))
			    break;
			if (b1)
			{
			    runch = 'k';
			    dy = -1;
			}
			else
			{
			    runch = 'j';
			    dy = 1;
			}
			dx = 0;
			turnref();
			goto over;
		    case 'j':
		    case 'k':
			b1 = (hero.x != 0 && turn_ok(hero.y, hero.x - 1));
			b2 = (hero.x != NUMCOLS - 1 && turn_ok(hero.y, hero.x + 1));
			if (!(b1 ^ b2))
			    break;
			if (b1)
			{
			    runch = 'h';
			    dx = -1;
			}
			else
			{
			    runch = 'l';
			    dx = 1;
			}
			dy = 0;
			turnref();
			goto over;
		}
	    }
	    running = FALSE;
	    after = FALSE;
	    break;
	case DOOR:
	    running = FALSE;
	    if (flat(hero.y, hero.x) & F_PASS)
		enter_room(&nh);
	    goto move_stuff;
	case TRAP:
	    ch = be_trapped(&nh);
	    if (ch == T_DOOR || ch == T_TELEP)
		return;
	    goto move_stuff;
	case PASSAGE:
	    /*
	     * when you're in a corridor, you don't know if you're in
	     * a maze room or not, and there ain't no way to find out
	     * if you're leaving a maze room, so it is necessary to
	     * always recalculate proom.
	     */
	    proom = roomin(&hero);
	    goto move_stuff;
	case FLOOR:
	    if (!(fl & F_REAL))
		be_trapped(&hero);
	    goto move_stuff;
	case STAIRS:
	    seenstairs = TRUE;
	    /* FALLTHROUGH */
	default:
	    running = FALSE;
	    if (isupper(ch) || moat(nh.y, nh.x))
		fight(&nh, cur_weapon, FALSE);
	    else
	    {
		if (ch != STAIRS)
		    take = ch;
move_stuff:
		jrogue_mvaddch(hero.y, hero.x, floor_at());
		if ((fl & F_PASS) && chat(oldpos.y, oldpos.x) == DOOR)
		    leave_room(&nh);
		hero = nh;
	    }
    }
}

/*
 * turn_ok:
 *	Decide whether it is legal to turn onto the given space
 */
int
turn_ok(int y, int x)
{
    PLACE *pp;

    pp = INDEX(y, x);
    return (pp->p_ch == DOOR
	|| (pp->p_flags & (F_REAL|F_PASS)) == (F_REAL|F_PASS));
}

/*
 * turnref:
 *	Decide whether to refresh at a passage turning or not
 */

void
turnref(void)
{
    PLACE *pp;

    pp = INDEX(hero.y, hero.x);
    if (!(pp->p_flags & F_SEEN))
    {
	if (jump)
	{
	    leaveok(stdscr, TRUE);
	    jrogue_restore_cursor();
	    refresh();
	    leaveok(stdscr, FALSE);
	}
	pp->p_flags |= F_SEEN;
    }
}

/*
 * door_open:
 *	Called to illuminate a room.  If it is dark, remove anything
 *	that might move.
 */

void
door_open(const struct room *rp)
{
    int y, x;

    if (!(rp->r_flags & ISGONE))
	for (y = rp->r_pos.y; y < rp->r_pos.y + rp->r_max.y; y++)
	    for (x = rp->r_pos.x; x < rp->r_pos.x + rp->r_max.x; x++)
		if (isupper(winat(y, x)))
		    wake_monster(y, x);
}

/*
 * be_trapped:
 *	The guy stepped on a trap.... Make him pay.
 */
int
be_trapped(const coord *tc)
{
    PLACE *pp;
    THING *arrow;
    int tr;

    if (on(player, ISLEVIT))
	return T_RUST;	/* anything that's not a door or teleport */
    running = FALSE;
    count = FALSE;
    pp = INDEX(tc->y, tc->x);
    pp->p_ch = TRAP;
    tr = pp->p_flags & F_TMASK;
    pp->p_flags |= F_SEEN;
    switch (tr)
    {
	case T_DOOR:
	    if (level < JROGUE_MAX_DUNGEON_LEVEL)
	    {
		level++;
		new_level();
	    }
	    msg(JSTR("わなの中に落ちた！", "you fell into a trap!"));
	when T_BEAR:
	    no_move = jrogue_add_turns(no_move, BEARTIME);
	    msg(JSTR("とらばさみのわなにつかまった。", "you are caught in a bear trap"));
        when T_MYST:
            switch(rnd(11))
            {
                case 0: msg(JSTR("突然並行世界に移ったような気がする。", "you are suddenly in a parallel dimension"));
                when 1: msg(JSTR("ここの灯りが突然%sになったように見える。", "the light in here suddenly seems %s"), rainbow[rnd(cNCOLORS)]);
                when 2: msg(JSTR("首筋を刺されたように感じた。", "you feel a sting in the side of your neck"));
                when 3: msg(JSTR("様々な色の線が体のまわりにうずを巻き、消えていった。", "multi-colored lines swirl around you, then fade"));
                when 4: msg(JSTR("%sの閃光が見えた。", "a %s light flashes in your eyes"), rainbow[rnd(cNCOLORS)]);
                when 5: msg(JSTR("釘が撃ち出され、耳をかすめていった！", "a spike shoots past your ear!"));
                when 6: msg(JSTR("%sの火花がよろいのまわりを踊っている。", "%s sparks dance across your armor"), rainbow[rnd(cNCOLORS)]);
                when 7: msg(JSTR("急にのどがとても乾いた。", "you suddenly feel very thirsty"));
                when 8: msg(JSTR("突然、時の流れが早くなったように感じた。", "you feel time speed up suddenly"));
                when 9: msg(JSTR("時間の流れが遅くなったように感じる。", "time now seems to be going slower"));
                when 10: msg(JSTR("背負い袋が%sになった！", "you pack turns %s!"), rainbow[rnd(cNCOLORS)]);
            }
	when T_SLEEP:
	    no_command = jrogue_add_turns(no_command, SLEEPTIME);
	    player.t_flags &= ~ISRUN;
	    msg(JSTR("不思議な白いもやにつつまれ、眠り込んでしまった。", "a strange white mist envelops you and you fall asleep"));
	when T_ARROW:
	    if (swing(pstats.s_lvl - 1, pstats.s_arm, 1))
	    {
		pstats.s_hpt -= roll(1, 6);
		if (pstats.s_hpt <= 0)
		{
		    msg(JSTR("矢に撃たれて死んでしまった。", "an arrow killed you"));
		    death('a');
		}
		else
		    msg(JSTR("うわっ！矢に撃たれた。", "oh no! An arrow shot you"));
	    }
	    else
	    {
		arrow = new_item();
		if (arrow != NULL)
		{
		    init_weapon(arrow, ARROW);
		    arrow->o_count = 1;
		    arrow->o_pos = hero;
		    fall(arrow, FALSE);
		}
		msg(JSTR("矢が体をかすめていった。", "an arrow shoots past you"));
	    }
	when T_TELEP:
	    /*
	     * since the hero's leaving, look() won't put a TRAP
	     * down for us, so we have to do it ourself
	     */
	    teleport();
	    jrogue_mvaddch(tc->y, tc->x, TRAP);
	when T_DART:
	    if (!swing(pstats.s_lvl+1, pstats.s_arm, 1))
		msg(JSTR("小さな投げ矢が耳をかすめ、どこかに消えてしまった。", "a small dart whizzes by your ear and vanishes"));
	    else
	    {
		pstats.s_hpt -= roll(1, 4);
		if (pstats.s_hpt <= 0)
		{
		    msg(JSTR("毒の投げ矢が当たり死んでしまった。", "a poisoned dart killed you"));
		    death('d');
		}
		if (!ISWEARING(R_SUSTSTR) && !save(VS_POISON))
		    chg_str(-1);
		msg(JSTR("小さな投げ矢が肩に刺さった。", "a small dart just hit you in the shoulder"));
	    }
	when T_RUST:
	    msg(JSTR("ほとばしる水が頭から浴びせられた。", "a gush of water hits you on the head"));
	    rust_armor(cur_armor);
    }
    flush_type();
    return tr;
}

/*
 * rndmove:
 *	Move in a random direction if the monster/person is confused
 */
coord
rndmove(const THING *who)
{
    THING *obj;
    int x, y;
    int ch;
    coord ret;  /* what we will be returning */

    y = ret.y = who->t_pos.y + rnd(3) - 1;
    x = ret.x = who->t_pos.x + rnd(3) - 1;
    /*
     * Now check to see if that's a legal move.  If not, don't move.
     * (I.e., bump into the wall or whatever)
     */
    if (y == who->t_pos.y && x == who->t_pos.x)
	return ret;
    if (!diag_ok(&who->t_pos, &ret))
	goto bad;
    else
    {
	ch = winat(y, x);
	if (!step_ok(ch))
	    goto bad;
	if (ch == SCROLL)
	{
	    for (obj = lvl_obj; obj != NULL; obj = next(obj))
		if (y == obj->o_pos.y && x == obj->o_pos.x)
		    break;
	    if (obj != NULL && obj->o_which == S_SCARE)
		goto bad;
	}
    }
    return ret;

bad:
    ret = who->t_pos;
    return ret;
}

/*
 * rust_armor:
 *	Rust the given armor, if it is a legal kind to rust, and we
 *	aren't wearing a magic ring.
 */

void
rust_armor(THING *arm)
{
    if (arm == NULL || arm->o_type != ARMOR || arm->o_which == LEATHER ||
	arm->o_arm >= 9)
	    return;

    if ((arm->o_flags & ISPROT) || ISWEARING(R_SUSTARM))
    {
	if (!to_death)
	    msg(JSTR("さびはすぐに消え去った。", "the rust vanishes instantly"));
    }
    else
    {
	arm->o_arm++;
	if (!terse)
	    msg(JSTR("よろいがもろくなったようだ。なんてこった！", "your armor appears to be weaker now. Oh my!"));
	else
	    msg(JSTR("よろいがもろくなった。", "your armor weakens"));
    }
}
