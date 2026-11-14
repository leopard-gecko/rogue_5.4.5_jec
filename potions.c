/*
 * Function(s) for dealing with potions
 *
 * @(#)potions.c	4.46 (Berkeley) 06/07/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <curses.h>
#include <limits.h>
#include <stdio.h>
#include <ctype.h>
#include "rogue.h"

typedef struct PACT
{
    const int pa_flags;
    void (*pa_daemon)(void);
    const int pa_time;
    const char *pa_high, *pa_straight;
} PACT;

static PACT p_actions[] =
{
	{ ISHUH,	unconfuse,	HUHDURATION,	/* P_CONFUSE */
		JSTR_STATIC("トリップしている気分だ！", "what a trippy feeling!"),
		JSTR_STATIC("ちょっと待った、何が起こったんだ？え？なに？だれ？", "wait, what's going on here. Huh? What? Who?") },
	{ ISHALU,	come_down,	SEEDURATION,	/* P_LSD */
		JSTR_STATIC("わお！ 何もかもが宇宙っぽく見える！", "Oh, wow!  Everything seems so cosmic!"),
		JSTR_STATIC("わお！ 何もかもが宇宙っぽく見える！", "Oh, wow!  Everything seems so cosmic!") },
	{ 0,		NULL,	0, NULL, NULL },			/* P_POISON */
	{ 0,		NULL,	0, NULL, NULL },			/* P_STRENGTH */
	{ CANSEE,	unsee,	SEEDURATION,		/* P_SEEINVIS */
		prbuf,
		prbuf },
	{ 0,		NULL,	0, NULL, NULL },			/* P_HEALING */
	{ 0,		NULL,	0, NULL, NULL },			/* P_MFIND */
	{ 0,		NULL,	0, NULL, NULL },			/* P_TFIND  */
	{ 0,		NULL,	0, NULL, NULL },			/* P_RAISE */
	{ 0,		NULL,	0, NULL, NULL },			/* P_XHEAL */
	{ 0,		NULL,	0, NULL, NULL },			/* P_HASTE */
	{ 0,		NULL,	0, NULL, NULL },			/* P_RESTORE */
	{ ISBLIND,	sight,	SEEDURATION,		/* P_BLIND */
		JSTR_STATIC("なんてこった！全部真っ暗だ！助けて！", "oh, bummer!  Everything is dark!  Help!"),
		JSTR_STATIC("深い暗闇のとばりがあたりをおおってゆく。", "a cloak of darkness falls around you") },
	{ ISLEVIT,	land,	HEALTIME,		/* P_LEVIT */
		JSTR_STATIC("わお！ 体が宙に浮いている！", "oh, wow!  You're floating in the air!"),
		JSTR_STATIC("体が宙に浮き始めた。", "you start to float in the air") }
};
void
jrogue_apply_potions_language(void)
{
    p_actions[0].pa_high = JSTR("トリップしている気分だ！", "what a trippy feeling!");
    p_actions[0].pa_straight = JSTR("ちょっと待った、何が起こったんだ？え？なに？だれ？", "wait, what's going on here. Huh? What? Who?");
    p_actions[1].pa_high = JSTR("わお！ 何もかもが宇宙っぽく見える！", "Oh, wow!  Everything seems so cosmic!");
    p_actions[1].pa_straight = JSTR("わお！ 何もかもが宇宙っぽく見える！", "Oh, wow!  Everything seems so cosmic!");
    p_actions[12].pa_high = JSTR("なんてこった！全部真っ暗だ！助けて！", "oh, bummer!  Everything is dark!  Help!");
    p_actions[12].pa_straight = JSTR("深い暗闇のとばりがあたりをおおってゆく。", "a cloak of darkness falls around you");
    p_actions[13].pa_high = JSTR("わお！ 体が宙に浮いている！", "oh, wow!  You're floating in the air!");
    p_actions[13].pa_straight = JSTR("体が宙に浮き始めた。", "you start to float in the air");
}


/*
 * quaff:
 *	Quaff a potion from the pack
 */

void
quaff(void)
{
    THING *obj, *tp, *mp;
    int discardit = FALSE;
    int show, trip;

    obj = get_item(JSTR("水薬を飲む", "quaff"), POTION);
    /*
     * Make certain that it is somethings that we want to drink
     */
    if (obj == NULL)
	return;
    if (obj->o_type != POTION)
    {
	if (!terse)
	    msg(JSTR("おえっ。なんでそんなものを飲もうとするの？", "yuk! Why would you want to drink that?"));
	else
	    msg(JSTR("それは飲めない。", "that's undrinkable"));
	return;
    }
    if (obj == cur_weapon)
	cur_weapon = NULL;

    /*
     * Calculate the effect it has on the poor guy.
     */
    trip = on(player, ISHALU);
    discardit = (obj->o_count == 1);
    leave_pack(obj, FALSE, FALSE);
    switch (obj->o_which)
    {
	case P_CONFUSE:
	    do_pot(P_CONFUSE, !trip);
	when P_POISON:
	    pot_info[P_POISON].oi_know = TRUE;
	    if (ISWEARING(R_SUSTSTR))
		msg(JSTR("具合が悪くなったが、すぐに治った。", "you feel momentarily sick"));
	    else
	    {
		chg_str(-(rnd(3) + 1));
		msg(JSTR("すごく具合が悪くなった。", "you feel very sick now"));
		come_down();
	    }
	when P_HEALING:
	    pot_info[P_HEALING].oi_know = TRUE;
	    pstats.s_hpt = jrogue_sat_add(pstats.s_hpt, roll(pstats.s_lvl, 4));
	    if (pstats.s_hpt > max_hp)
	    {
		if (max_hp < INT_MAX)
		    ++max_hp;
		pstats.s_hpt = max_hp;
	    }
	    sight();
	    msg(JSTR("気分がよくなってきた。", "you begin to feel better"));
	when P_STRENGTH:
	    pot_info[P_STRENGTH].oi_know = TRUE;
	    chg_str(1);
	    msg(JSTR("強くなった気がするぞ。このはちきれそうな筋肉！", "you feel stronger, now.  What bulging muscles!"));
	when P_MFIND:
	    player.t_flags |= SEEMONST;
	    /* Repeated doses extend the same effect, as do other timed
	     * potions.  Multiple expiration fuses would end it too early. */
	    if (find_slot(turn_see_off) != NULL)
		lengthen(turn_see_off, HUHDURATION);
	    else
		fuse(turn_see_off, 0, HUHDURATION, AFTER);
	    if (!turn_see(FALSE))
		msg(JSTR("%s気分におそわれたが、すぐに消えていった。", "you have a %s feeling for a moment, then it passes"),
		    choose_str(JSTR("まともな", "normal"), JSTR("奇妙な", "strange")));
	when P_TFIND:
	    /*
	     * Potion of magic detection.  Show the potions and scrolls
	     */
	    show = FALSE;
	    if (lvl_obj != NULL)
	    {
		wclear(hw);
		jrogue_wset_normal_attr(hw);
		for (tp = lvl_obj; tp != NULL; tp = next(tp))
		{
		    if (is_magic(tp))
		    {
			show = TRUE;
			wmove(hw, tp->o_pos.y, tp->o_pos.x);
			jrogue_waddch(hw, MAGIC);
			jrogue_wset_normal_attr(hw);
			pot_info[P_TFIND].oi_know = TRUE;
		    }
		}
		for (mp = mlist; mp != NULL; mp = next(mp))
		{
		    for (tp = mp->t_pack; tp != NULL; tp = next(tp))
		    {
			if (is_magic(tp))
			{
			    show = TRUE;
			    wmove(hw, mp->t_pos.y, mp->t_pos.x);
			    jrogue_waddch(hw, MAGIC);
			    jrogue_wset_normal_attr(hw);
			}
		    }
		}
	    }
	    if (show)
	    {
		pot_info[P_TFIND].oi_know = TRUE;
		show_win(JSTR("この階で魔法の存在を感じた。--続く--", "You sense the presence of magic on this level.--More--"));
	    }
	    else
		msg(JSTR("%s気分におそわれたが、すぐに消えていった。", "you have a %s feeling for a moment, then it passes"),
		    choose_str(JSTR("まともな", "normal"), JSTR("奇妙な", "strange")));
	when P_LSD:
	    if (!trip)
	    {
		if (on(player, SEEMONST))
		    turn_see(FALSE);
		start_daemon(visuals, 0, BEFORE);
		seenstairs = seen_stairs();
	    }
	    do_pot(P_LSD, TRUE);
	when P_SEEINVIS:
	    snprintf(prbuf, PRBUF_SIZE, JSTR("この水薬は、%sジュースのような味がする。", "this potion tastes like %s juice"), jrogue_fruit_display());
	    show = on(player, CANSEE);
	    do_pot(P_SEEINVIS, FALSE);
	    if (!show)
		invis_on();
	    sight();
	when P_RAISE:
	    pot_info[P_RAISE].oi_know = TRUE;
	    msg(JSTR("突然に腕前がすごく上がったようだ。", "you suddenly feel much more skillful"));
	    raise_level();
	when P_XHEAL:
	    pot_info[P_XHEAL].oi_know = TRUE;
	    pstats.s_hpt = jrogue_sat_add(pstats.s_hpt, roll(pstats.s_lvl, 8));
	    if (pstats.s_hpt > max_hp)
	    {
		if (pstats.s_hpt > jrogue_sat_add(max_hp, pstats.s_lvl + 1) &&
		    max_hp < INT_MAX)
		    ++max_hp;
		if (max_hp < INT_MAX)
		    ++max_hp;
		pstats.s_hpt = max_hp;
	    }
	    sight();
	    come_down();
	    msg(JSTR("とても気分がよくなってきた。", "you begin to feel much better"));
	when P_HASTE:
	    pot_info[P_HASTE].oi_know = TRUE;
	    after = FALSE;
	    if (add_haste(TRUE))
		msg(JSTR("動きが素早くなったようだ。", "you feel yourself moving much faster"));
	when P_RESTORE:
	    if (ISRING(LEFT, R_ADDSTR))
		add_str(&pstats.s_str, -cur_ring[LEFT]->o_arm);
	    if (ISRING(RIGHT, R_ADDSTR))
		add_str(&pstats.s_str, -cur_ring[RIGHT]->o_arm);
	    if (pstats.s_str < max_stats.s_str)
		pstats.s_str = max_stats.s_str;
	    if (ISRING(LEFT, R_ADDSTR))
		add_str(&pstats.s_str, cur_ring[LEFT]->o_arm);
	    if (ISRING(RIGHT, R_ADDSTR))
		add_str(&pstats.s_str, cur_ring[RIGHT]->o_arm);
	    msg(JSTR("ああ、これはうまい。身体中があたたまる。", "hey, this tastes great.  It makes you feel warm all over"));
	when P_BLIND:
	    do_pot(P_BLIND, TRUE);
	when P_LEVIT:
	    do_pot(P_LEVIT, TRUE);
#ifdef MASTER
	otherwise:
	    msg(JSTR("なんてひどい味の水薬だ！", "what an odd tasting potion!"));
	    return;
#endif
    }
    status();
    /*
     * Throw the item away
     */

    call_it(&pot_info[obj->o_which]);

    if (discardit)
	discard_object(obj);
    return;
}

/*
 * is_magic:
 *	Returns true if an object radiates magic
 */
int
is_magic(const THING *obj)
{
    switch (obj->o_type)
    {
	case ARMOR:
	    return ((obj->o_flags&ISPROT) || obj->o_arm != a_class[obj->o_which]);
	case WEAPON:
	    return (obj->o_hplus != 0 || obj->o_dplus != 0);
	case POTION:
	case SCROLL:
	case STICK:
	case RING:
	case AMULET:
	    return TRUE;
    }
    return FALSE;
}

/*
 * invis_on:
 *	Turn on the ability to see invisible
 */

void
invis_on(void)
{
    THING *mp;

    player.t_flags |= CANSEE;
    for (mp = mlist; mp != NULL; mp = next(mp))
	if (on(*mp, ISINVIS) && see_monst(mp) && !on(player, ISHALU))
	    jrogue_mvaddch(mp->t_pos.y, mp->t_pos.x, mp->t_disguise);
}

/*
 * turn_see:
 *	Put on or off seeing monsters on this level
 */
int
turn_see(int turn_off)
{
    THING *mp;
    int can_see, add_new;

    add_new = FALSE;
    for (mp = mlist; mp != NULL; mp = next(mp))
    {
	move(mp->t_pos.y, mp->t_pos.x);
	can_see = see_monst(mp);
	if (turn_off)
	{
	    if (!can_see)
		jrogue_addch(mp->t_oldch);
	}
	else
	{
	    if (!can_see)
		jrogue_standout();
	    if (!on(player, ISHALU))
		jrogue_addch(mp->t_type);
	    else
		jrogue_addch(rnd(26) + 'A');
	    if (!can_see)
	    {
		jrogue_standend();
		add_new++;
	    }
	}
    }
    if (turn_off)
	player.t_flags &= ~SEEMONST;
    else
	player.t_flags |= SEEMONST;
    return add_new;
}

/*
 * turn_see_off:
 *	Callback with no arguments for the monster-detection potion's fuse.
 *	turn_see() itself takes an argument and returns a status value.
 */
void
turn_see_off(void)
{
    (void)turn_see(TRUE);
}

/*
 * seen_stairs:
 *	Return TRUE if the player has seen the stairs
 */
int
seen_stairs(void)
{
    THING	*tp;

    move(stairs.y, stairs.x);
    if (CCHAR( inch() ) == STAIRS)			/* it's on the map */
	return TRUE;
    if (ce(hero, stairs))			/* It's under him */
	return TRUE;

    /*
     * if a monster is on the stairs, this gets hairy
     */
    if ((tp = moat(stairs.y, stairs.x)) != NULL)
    {
	if (see_monst(tp) && on(*tp, ISRUN))	/* if it's visible and awake */
	    return TRUE;			/* it must have moved there */

	if (on(player, SEEMONST)		/* if she can detect monster */
	    && tp->t_oldch == STAIRS)		/* and there once were stairs */
		return TRUE;			/* it must have moved there */
    }
    return FALSE;
}

/*
 * raise_level:
 *	The guy just magically went up a level.
 */

void
raise_level(void)
{
    /* e_levels[20] is a terminator, NOT the threshold for level 22.
     * A raise-level potion at level 21 must not reset experience to 1. */
    if (pstats.s_lvl >= JROGUE_MAX_EXP_LEVEL)
        return;
    pstats.s_exp = e_levels[pstats.s_lvl-1] + 1;
    check_level();
}

/*
 * do_pot:
 *	Do a potion with standard setup.  This means it uses a fuse and
 *	turns on a flag
 */

void
do_pot(int type, int knowit)
{
    const PACT *pp;
    int t;

    pp = &p_actions[type];
    if (!pot_info[type].oi_know)
	pot_info[type].oi_know = knowit;
    t = spread(pp->pa_time);
    if (!on(player, pp->pa_flags))
    {
	player.t_flags |= pp->pa_flags;
	fuse(pp->pa_daemon, 0, t, AFTER);
	look(FALSE);
    }
    else
	lengthen(pp->pa_daemon, t);
    msg("%s", choose_str(pp->pa_high, pp->pa_straight));
}
