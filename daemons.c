/*
 * All the daemon and fuse functions are in here
 *
 * @(#)daemons.c	4.24 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <curses.h>
#include "rogue.h"

/* Recovery has no further benefit once this threshold is reached. */
#define JROGUE_QUIET_SATURATION 21

/*
 * doctor:
 *	A healing daemon that restors hit points after rest
 */
void
doctor(void)
{
    int lv, ohp;

    lv = pstats.s_lvl;
    ohp = pstats.s_hpt;
    /* When the healing threshold is passed, a larger quiet count gives
     * no extra benefit.  Saturation also handles extreme saved values. */
    if (quiet < JROGUE_QUIET_SATURATION)
        quiet++;
    if (lv < 8)
    {
	if (quiet > 20 - (lv << 1))
	    pstats.s_hpt = jrogue_sat_add(pstats.s_hpt, 1);
    }
    else
	if (quiet >= 3)
	    pstats.s_hpt = jrogue_sat_add(pstats.s_hpt, rnd(lv - 7) + 1);
    if (ISRING(LEFT, R_REGEN))
	pstats.s_hpt = jrogue_sat_add(pstats.s_hpt, 1);
    if (ISRING(RIGHT, R_REGEN))
	pstats.s_hpt = jrogue_sat_add(pstats.s_hpt, 1);
    if (ohp != pstats.s_hpt)
    {
	if (pstats.s_hpt > max_hp)
	    pstats.s_hpt = max_hp;
	quiet = 0;
    }
}

/*
 * Swander:
 *	Called when it is time to start rolling for wandering monsters
 */
void
swander(void)
{
    start_daemon(rollwand, 0, BEFORE);
}

/*
 * rollwand:
 *	Called to roll to see if a wandering monster starts up
 */

void
rollwand(void)
{

    /* Do not increment past a corrupt or extreme counter value. */
    if (between >= 3)
    {
	if (roll(1, 6) == 4)
	{
	    wanderer();
	    kill_daemon(rollwand);
	    fuse(swander, 0, WANDERTIME, BEFORE);
	}
	between = 0;
    }
    else
        between++;
}

/*
 * unconfuse:
 *	Release the poor player from his confusion
 */
void
unconfuse(void)
{
    player.t_flags &= ~ISHUH;
    msg(JSTR("ようやく%sがおさまってきた。", "you feel less %s now"), choose_str(JSTR("トリップ感覚", "trippy"), JSTR("混乱", "confused")));
}

/*
 * unsee:
 *	Turn off the ability to see invisible
 */
void
unsee(void)
{
    THING *th;

    for (th = mlist; th != NULL; th = next(th))
	if (on(*th, ISINVIS) && see_monst(th))
	    jrogue_mvaddch(th->t_pos.y, th->t_pos.x, th->t_oldch);
    player.t_flags &= ~CANSEE;
}

/*
 * sight:
 *	He gets his sight back
 */
void
sight(void)
{
    if (on(player, ISBLIND))
    {
	extinguish(sight);
	player.t_flags &= ~ISBLIND;
	if (!(proom->r_flags & ISGONE))
	    enter_room(&hero);
	msg(choose_str(JSTR("いいぞ！すべてがまた宇宙っぽく見えてきた。", "far out!  Everything is all cosmic again"),
		       JSTR("暗闇のとばりが上がってゆく。", "the veil of darkness lifts")));
    }
}

/*
 * nohaste:
 *	End the hasting
 */
void
nohaste(void)
{
    player.t_flags &= ~ISHASTE;
    msg(JSTR("素早くなる薬の効き目がなくなった。", "you feel yourself slowing down"));
}

/*
 * stomach:
 *	Digest the hero's food
 */
void
stomach(void)
{
    int oldfood;
    int orig_hungry = hungry_state;

    if (food_left <= 0)
    {
	if (food_left-- < -STARVETIME)
	    death('s');
	/*
	 * the hero is fainting
	 */
	if (no_command || rnd(5) != 0)
	    return;
	no_command = jrogue_add_turns(no_command, rnd(8) + 4);
	hungry_state = 3;
	if (!terse)
	    addmsg("%s", choose_str(JSTR("腹がへって運動能力が圧倒されているぞ。", "the munchies overpower your motor capabilities.  "),
			      JSTR("食べていないので体が弱りきっている。", "you feel too weak from lack of food.  ")));
	msg("%s", choose_str(JSTR("何が何だかわからなくなった。", "You freak out"), JSTR("気を失った。", "You faint")));
    }
    else
    {
	oldfood = food_left;
	food_left -= ring_eat(LEFT) + ring_eat(RIGHT) + 1 - amulet;

	if (food_left < MORETIME && oldfood >= MORETIME)
	{
	    hungry_state = 2;
	    msg("%s", choose_str(JSTR("腹がへって運動能力が妨害されてきたぞ。", "the munchies are interfering with your motor capabilities"),
			   JSTR("そろそろ力がなくなってきた。", "you are starting to feel weak")));
	}
	else if (food_left < 2 * MORETIME && oldfood >= 2 * MORETIME)
	{
	    hungry_state = 1;
	    if (terse)
		msg("%s", choose_str(JSTR("腹がへったなあ。", "getting the munchies"), JSTR("空腹になってきた。", "getting hungry")));
	    else
		msg("%s", choose_str(JSTR("そろそろ腹がへったなあ。", "you are getting the munchies"),
			       JSTR("そろそろ空腹になってきた。", "you are starting to get hungry")));
	}
    }
    if (hungry_state != orig_hungry) { 
        player.t_flags &= ~ISRUN; 
        running = FALSE; 
        to_death = FALSE; 
        count = 0; 
    } 
}

/*
 * come_down:
 *	Take the hero down off her acid trip.
 */
void
come_down(void)
{
    THING *tp;
    int seemonst;

    if (!on(player, ISHALU))
	return;

    kill_daemon(visuals);
    player.t_flags &= ~ISHALU;

    if (on(player, ISBLIND))
	return;

    /*
     * undo the things
     */
    for (tp = lvl_obj; tp != NULL; tp = next(tp))
	if (cansee(tp->o_pos.y, tp->o_pos.x))
	    jrogue_mvaddch(tp->o_pos.y, tp->o_pos.x, tp->o_type);

    /*
     * undo the monsters
     */
    seemonst = on(player, SEEMONST);
    for (tp = mlist; tp != NULL; tp = next(tp))
    {
	move(tp->t_pos.y, tp->t_pos.x);
	if (cansee(tp->t_pos.y, tp->t_pos.x))
	    if (!on(*tp, ISINVIS) || on(player, CANSEE))
		jrogue_addch(tp->t_disguise);
	    else
		jrogue_addch(chat(tp->t_pos.y, tp->t_pos.x));
	else if (seemonst)
	{
	    jrogue_standout();
	    jrogue_addch(tp->t_type);
	    jrogue_standend();
	}
    }
    msg(JSTR("すべてがとてもつまらなく見える。", "Everything looks SO boring now."));
}

/*
 * visuals:
 *	change the characters for the player
 */
void
visuals(void)
{
    THING *tp;
    int seemonst;

    if (!after || (running && jump))
	return;
    /*
     * change the things
     */
    for (tp = lvl_obj; tp != NULL; tp = next(tp))
	if (cansee(tp->o_pos.y, tp->o_pos.x))
	    jrogue_mvaddch(tp->o_pos.y, tp->o_pos.x, rnd_thing());

    /*
     * change the stairs
     */
    if (!seenstairs && cansee(stairs.y, stairs.x))
	jrogue_mvaddch(stairs.y, stairs.x, rnd_thing());

    /*
     * change the monsters
     */
    seemonst = on(player, SEEMONST);
    for (tp = mlist; tp != NULL; tp = next(tp))
    {
	move(tp->t_pos.y, tp->t_pos.x);
	if (see_monst(tp))
	{
	    if (tp->t_type == 'X' && tp->t_disguise != 'X')
		jrogue_addch(rnd_thing());
	    else
		jrogue_addch(rnd(26) + 'A');
	}
	else if (seemonst)
	{
	    jrogue_standout();
	    jrogue_addch(rnd(26) + 'A');
	    jrogue_standend();
	}
    }
}

/*
 * land:
 *	Land from a levitation potion
 */
void
land(void)
{
    player.t_flags &= ~ISLEVIT;
    msg(choose_str(JSTR("うわっ！地面に落っこちた。", "bummer!  You've hit the ground"),
		   JSTR("ゆっくりと地面に降り立った。", "you float gently to the ground")));
}
