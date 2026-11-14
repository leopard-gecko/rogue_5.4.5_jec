/*
 * All the fighting gets done here
 *
 * @(#)fight.c	4.67 (Berkeley) 09/06/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <curses.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "rogue.h"

#define	EQSTR(a, b)	(strcmp(a, b) == 0)

static const char *h_names[] = {		/* strings for hitting */
	JSTR_STATIC("激しい一撃を食らわせた。", " scored an excellent hit on "),
	JSTR_STATIC("攻撃し命中した。", " hit "),
	JSTR_STATIC("傷を負わせた。", " have injured "),
	JSTR_STATIC("殴りかかり命中した。", " swing and hit "),
	JSTR_STATIC("激しい一撃を食らわせた。", " scored an excellent hit on "),
	JSTR_STATIC("攻撃し命中した。", " hit "),
	JSTR_STATIC("傷を負わせた。", " has injured "),
	JSTR_STATIC("殴りかかり命中した。", " swings and hits ")
};

static const char *m_names[] = {		/* strings for missing */
	JSTR_STATIC("攻撃したが外れた。", " miss"),
	JSTR_STATIC("殴りかかったが外れた。", " swing and miss"),
	JSTR_STATIC("攻撃したがぎりぎりでそれた。", " barely miss"),
	JSTR_STATIC("攻撃したが当たらなかった。", " don't hit"),
	JSTR_STATIC("攻撃したが外れた。", " misses"),
	JSTR_STATIC("殴りかかったが外れた。", " swings and misses"),
	JSTR_STATIC("攻撃したがぎりぎりでそれた。", " barely misses"),
	JSTR_STATIC("攻撃したが当たらなかった。", " doesn't hit"),
};
void
jrogue_apply_fight_language(void)
{
    h_names[0] = JSTR("激しい一撃を食らわせた。", " scored an excellent hit on ");
    h_names[1] = JSTR("攻撃し命中した。", " hit ");
    h_names[2] = JSTR("傷を負わせた。", " have injured ");
    h_names[3] = JSTR("殴りかかり命中した。", " swing and hit ");
    h_names[4] = JSTR("激しい一撃を食らわせた。", " scored an excellent hit on ");
    h_names[5] = JSTR("攻撃し命中した。", " hit ");
    h_names[6] = JSTR("傷を負わせた。", " has injured ");
    h_names[7] = JSTR("殴りかかり命中した。", " swings and hits ");
    m_names[0] = JSTR("攻撃したが外れた。", " miss");
    m_names[1] = JSTR("殴りかかったが外れた。", " swing and miss");
    m_names[2] = JSTR("攻撃したがぎりぎりでそれた。", " barely miss");
    m_names[3] = JSTR("攻撃したが当たらなかった。", " don't hit");
    m_names[4] = JSTR("攻撃したが外れた。", " misses");
    m_names[5] = JSTR("殴りかかったが外れた。", " swings and misses");
    m_names[6] = JSTR("攻撃したがぎりぎりでそれた。", " barely misses");
    m_names[7] = JSTR("攻撃したが当たらなかった。", " doesn't hit");
}


/*
 * adjustments to hit probabilities due to strength
 */
static int str_plus[] = {
    -7, -6, -5, -4, -3, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3,
};

/*
 * adjustments to damage done due to strength
 */
static int add_dam[] = {
    -7, -6, -5, -4, -3, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3,
    3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6
};

/*
 * fight:
 *	The player attacks the monster.
 */
int
fight(const coord *mp, const THING *weap, int thrown)
{
    THING *tp;
    int did_hit = TRUE;
    const char *mname;
	int ch;

    /*
     * Find the monster we want to fight
     */
    if ((tp = moat(mp->y, mp->x)) == NULL)
	{
#ifdef MASTER
		debug("Fight what @ %d,%d", mp->y, mp->x);
#endif
		return FALSE;
	}
    /*
     * Since we are fighting, things are not quiet so no healing takes
     * place.
     */
    count = 0;
    quiet = 0;
    runto(mp);
    /*
     * Let him know it was really a xeroc (if it was one).
     */
    ch = '\0';
    if (tp->t_type == 'X' && tp->t_disguise != 'X' && !on(player, ISBLIND))
    {
	tp->t_disguise = 'X';
	if (on(player, ISHALU)) {
	    ch = rnd(26) + 'A';
	    jrogue_mvaddch(tp->t_pos.y, tp->t_pos.x, ch);
	}
	msg(choose_str(JSTR("しまった！そいつは汚い生き物だ！", "heavy!  That's a nasty critter!"),
		       JSTR("待て！そいつはゼロックだ！", "wait!  That's a xeroc!")));
	if (!thrown)
	    return FALSE;
    }
    mname = set_mname(tp);
    did_hit = FALSE;
    has_hit = (terse && !to_death);
    if (roll_em(&player, tp, weap, thrown))
    {
	did_hit = FALSE;
	if (thrown)
	    thunk(weap, mname, terse);
	else
	    hit(NULL, mname, terse);
	if (on(player, CANHUH))
	{
	    did_hit = TRUE;
	    tp->t_flags |= ISHUH;
	    player.t_flags &= ~CANHUH;
	    endmsg();
	    has_hit = FALSE;
	    msg(JSTR("手の%sの輝きが消えた。", "your hands stop glowing %s"), pick_color(JSTR("赤色", "red")));
	}
	if (tp->t_stats.s_hpt <= 0)
	    killed(tp, TRUE);
	else if (did_hit && !on(player, ISBLIND))
	    msg(JSTR("%sは頭が混乱した。", "%s appears confused"), mname);
	did_hit = TRUE;
    }
    else
	if (thrown)
	    bounce(weap, mname, terse);
	else
	    miss(NULL, mname, terse);
    return did_hit;
}

/*
 * attack:
 *	The monster attacks the player
 */
int
attack(THING *mp)
{
    const char *mname;
    int oldhp;

    /*
     * Since this is an attack, stop running and any healing that was
     * going on at the time.
     */
    running = FALSE;
    count = 0;
    quiet = 0;
    if (to_death && !on(*mp, ISTARGET))
    {
	to_death = FALSE;
	kamikaze = FALSE;
    }
    if (mp->t_type == 'X' && mp->t_disguise != 'X' && !on(player, ISBLIND))
    {
	mp->t_disguise = 'X';
	if (on(player, ISHALU))
	    jrogue_mvaddch(mp->t_pos.y, mp->t_pos.x, rnd(26) + 'A');
    }
    mname = set_mname(mp);
    oldhp = pstats.s_hpt;
    if (roll_em(mp, &player, NULL, FALSE))
    {
	if (mp->t_type != 'I')
	{
	    if (has_hit)
		addmsg(JSTR("", ".  "));
	    hit(mname, NULL, FALSE);
	}
	else
	    if (has_hit)
		endmsg();
	has_hit = FALSE;
	if (pstats.s_hpt <= 0)
	    death(mp->t_type);	/* Bye bye life ... */
	else if (!kamikaze)
	{
	    oldhp = jrogue_sat_sub(oldhp, pstats.s_hpt);
	    if (oldhp > max_hit)
		max_hit = oldhp;
	    if (pstats.s_hpt <= max_hit)
		to_death = FALSE;
	}
	if (!on(*mp, ISCANC))
	    switch (mp->t_type)
	    {
		case 'A':
		    /*
		     * If an aquator hits, you can lose armor class.
		     */
		    rust_armor(cur_armor);
		when 'I':
		    /*
		     * The ice monster freezes you
		     */
		    player.t_flags &= ~ISRUN;
		    if (!no_command)
		    {
if (jrogue_lang_is_japanese)
		    {
			if (!terse)
			    addmsg("%sの攻撃で", mname);
			addmsg("体が凍りついた。");
		    }
		    else
		    {
			addmsg("you are frozen");
			if (!terse)
			    addmsg(" by %s", mname);
		    }
			endmsg();
		    }
		    no_command = jrogue_add_turns(no_command, rnd(2) + 2);
		    if (no_command > BORE_LEVEL)
			death('h');
		when 'R':
		    /*
		     * Rattlesnakes have poisonous bites
		     */
		    if (!save(VS_POISON))
		    {
			if (!ISWEARING(R_SUSTSTR))
			{
			    chg_str(-1);
			    if (!terse)
				msg(JSTR("足を咬まれて、力が弱くなったようだ。", "you feel a bite in your leg and now feel weaker"));
			    else
				msg(JSTR("咬まれて、力が弱くなった。", "a bite has weakened you"));
			}
			else if (!to_death)
			{
			    if (!terse)
				msg(JSTR("咬まれて、ほんの一瞬だけ力が弱くなった。", "a bite momentarily weakens you"));
			    else
				msg(JSTR("咬まれたが、何ともない。", "bite has no effect"));
			}
		    }
		when 'W':
		case 'V':
		    /*
		     * Wraiths might drain energy levels, and Vampires
		     * can steal max_hp
		     */
		    if (rnd(100) < (mp->t_type == 'W' ? 15 : 30))
		    {
			int fewer;

			if (mp->t_type == 'W')
			{
			    if (pstats.s_exp == 0)
				death('W');		/* All levels gone */
			    if (--pstats.s_lvl == 0)
			    {
				pstats.s_exp = 0;
				pstats.s_lvl = 1;
			    }
			    else
				pstats.s_exp = e_levels[pstats.s_lvl-1]+1;
			    fewer = roll(1, 10);
			}
			else
			    fewer = roll(1, 3);
			pstats.s_hpt -= fewer;
			max_hp -= fewer;
			if (pstats.s_hpt <= 0)
			    pstats.s_hpt = 1;
			if (max_hp <= 0)
			    death(mp->t_type);
			msg(JSTR("急に弱くなったようだ。", "you suddenly feel weaker"));
		    }
		when 'F':
		    /*
		     * Venus Flytrap stops the poor guy from moving
		     */
		    player.t_flags |= ISHELD;
		    /* Keep the flytrap damage count representable in a bounded
		     * dice string even during extraordinary wizard sessions. */
		    if (vf_hit < JROGUE_MAX_FLYTRAP_HITS)
			vf_hit++;
		    snprintf(monsters['F'-'A'].m_stats.s_dmg, sizeof monsters['F'-'A'].m_stats.s_dmg, "%ux1", (unsigned int)vf_hit);
		    if (--pstats.s_hpt <= 0)
			death('F');
		when 'L':
		{
		    /*
		     * Leperachaun steals some gold
		     */
		    int lastpurse;

		    lastpurse = purse;
		    purse = jrogue_sat_money_sub(purse, GOLDCALC);
		    if (!save(VS_MAGIC))
		    {
			int k;
			/* Four extra gold rolls, in precisely the original order. */
			for (k = 0; k < 4; k++)
			    purse = jrogue_sat_money_sub(purse, GOLDCALC);
		    }
		    remove_mon(&mp->t_pos, mp, FALSE);
                    mp=NULL;
		    if (purse != lastpurse)
			msg(JSTR("財布が軽くなった気がするぞ。", "your purse feels lighter"));
		}
		when 'N':
		{
		    THING *obj, *steal;
		    int nobj;

		    /*
		     * Nymph's steal a magic item, look through the pack
		     * and pick out one we like.
		     */
		    steal = NULL;
		    for (nobj = 0, obj = pack; obj != NULL; obj = next(obj))
			if (obj != cur_armor && obj != cur_weapon
			    && obj != cur_ring[LEFT] && obj != cur_ring[RIGHT]
			    && is_magic(obj) && rnd(++nobj) == 0)
				steal = obj;
		    if (steal != NULL)
		    {
                steal = leave_pack(steal, TRUE, FALSE);
                if (steal != NULL)
                {
                    remove_mon(&mp->t_pos, moat(mp->t_pos.y, mp->t_pos.x), FALSE);
                    mp = NULL;
                    msg(JSTR("彼女は%sを盗んでいった！", "she stole %s!"), inv_name(steal, TRUE));
                    discard_object(steal);
                }
		    }
		}
		otherwise:
		    break;
	    }
    }
    else if (mp->t_type != 'I')
    {
	if (has_hit)
	{
	    addmsg(JSTR("", ".  "));
	    has_hit = FALSE;
	}
	if (mp->t_type == 'F')
	{
	    /* Compare before subtracting: an extreme flytrap count could
	     * otherwise overflow the signed HP value. */
	    if (vf_hit >= pstats.s_hpt)
		death(mp->t_type);
	    else
		pstats.s_hpt -= vf_hit;	/* Bye bye life ... */
	}
	miss(mname, NULL, FALSE);
    }
    if (fight_flush && !to_death)
	flush_type();
    count = 0;
    status();
    if (mp == NULL)
        return(-1);
    else
        return(0);
}

/*
 * set_mname:
 *	return the monster name for the given monster
 */
const char *
set_mname(const THING *tp)
{
    int ch;
    const char *mname;
    static char tbuf[MAXSTR] = { 't', 'h', 'e', ' ' };

    if (!see_monst(tp) && !on(player, SEEMONST))
	return (terse ? JSTR("何か", "it") : JSTR("何者か", "something"));
    else if (on(player, ISHALU))
    {
	move(tp->t_pos.y, tp->t_pos.x);
	ch = toascii(CCHAR(inch()));
	if (!isupper(ch))
	    ch = rnd(26);
	else
	    ch -= 'A';
	mname = monsters[ch].m_name;
    }
    else
	mname = monsters[tp->t_type - 'A'].m_name;
    /* RRP 5.4.5 prefixes visible monster names with "the ".
     * Retain Japanese names without the English article, and bound the copy. */
    if (jrogue_lang_is_japanese)
	jrogue_strlcpy(tbuf, mname, sizeof tbuf);
    else {
	jrogue_strlcpy(tbuf, "the ", sizeof tbuf);
	jrogue_strlcat(tbuf, mname, sizeof tbuf);
    }
    return tbuf;
}

/*
 * swing:
 *	Returns true if the swing hits
 */
int
swing(int at_lvl, int op_arm, int wplus)
{
    int res = rnd(20);
    /* Both armor and bonuses can be extreme in restored/wizard games.
     * Evaluate the comparison in a wider type without changing the roll. */
    long long need = (20LL - at_lvl) - op_arm;

    return ((long long)res + wplus >= need);
}

/*
 * roll_em:
 *	Roll several attacks
 */
int
roll_em(const THING *thatt, THING *thdef, const THING *weap, int hurl)
{
    const struct stats *att;
    struct stats *def;
    const char *cp;
    int ndice, nsides, def_arm;
    int did_hit = FALSE;
    int hplus;
    int dplus;
    int damage;

    att = &thatt->t_stats;
    def = &thdef->t_stats;
    if (weap == NULL)
    {
	cp = att->s_dmg;
	dplus = 0;
	hplus = 0;
    }
    else
    {
	hplus = (weap == NULL ? 0 : weap->o_hplus);
	dplus = (weap == NULL ? 0 : weap->o_dplus);
	if (weap == cur_weapon)
	{
	    if (ISRING(LEFT, R_ADDDAM))
		dplus = jrogue_sat_add(dplus, cur_ring[LEFT]->o_arm);
	    else if (ISRING(LEFT, R_ADDHIT))
		hplus = jrogue_sat_add(hplus, cur_ring[LEFT]->o_arm);
	    if (ISRING(RIGHT, R_ADDDAM))
		dplus = jrogue_sat_add(dplus, cur_ring[RIGHT]->o_arm);
	    else if (ISRING(RIGHT, R_ADDHIT))
		hplus = jrogue_sat_add(hplus, cur_ring[RIGHT]->o_arm);
	}
	cp = weap->o_damage;
	if (hurl)
	{
	    if ((weap->o_flags&ISMISL) && cur_weapon != NULL &&
	      cur_weapon->o_which == weap->o_launch)
	    {
		cp = weap->o_hurldmg;
		hplus = jrogue_sat_add(hplus, cur_weapon->o_hplus);
		dplus = jrogue_sat_add(dplus, cur_weapon->o_dplus);
	    }
	    else if (weap->o_launch < 0)
		cp = weap->o_hurldmg;
	}
    }
    /*
     * If the creature being attacked is not running (alseep or held)
     * then the attacker gets a plus four bonus to hit.
     */
    if (!on(*thdef, ISRUN))
	hplus = jrogue_sat_add(hplus, 4);
    def_arm = def->s_arm;
    if (def == &pstats)
    {
	if (cur_armor != NULL)
	    def_arm = cur_armor->o_arm;
	if (ISRING(LEFT, R_PROTECT))
	    def_arm = jrogue_sat_sub(def_arm, cur_ring[LEFT]->o_arm);
	if (ISRING(RIGHT, R_PROTECT))
	    def_arm = jrogue_sat_sub(def_arm, cur_ring[RIGHT]->o_arm);
    }
    while(cp != NULL && *cp != '\0')
    {
	ndice = atoi(cp);
	if ((cp = strchr(cp, 'x')) == NULL)
	    break;
	nsides = atoi(++cp);
	if (swing(att->s_lvl, def_arm, jrogue_sat_add(hplus, str_plus[att->s_str])))
	{
	    int proll;

	    proll = roll(ndice, nsides);
#ifdef MASTER
	    if (ndice + nsides > 0 && proll <= 0)
		debug("Damage for %dx%d came out %d, dplus = %d, add_dam = %d, def_arm = %d", ndice, nsides, proll, dplus, add_dam[att->s_str], def_arm);
#endif
	    damage = jrogue_sat_add3(dplus, proll, add_dam[att->s_str]);
	    def->s_hpt = jrogue_sat_sub(def->s_hpt, max(0, damage));
	    did_hit = TRUE;
	}
	if ((cp = strchr(cp, '/')) == NULL)
	    break;
	cp++;
    }
    return did_hit;
}

/*
 * prname:
 *	The print name of a combatant
 */

char *
prname(const char *mname, int upper)
{
    static char tbuf[MAXSTR];

    *tbuf = '\0';
    if (jrogue_lang_is_japanese)
    {
	if (mname == 0)
	    jrogue_strlcpy(tbuf, "あなた", sizeof tbuf); 
	else
	    jrogue_strlcpy(tbuf, mname, sizeof tbuf);
    }
    else
    {
	if (mname == 0)
	    jrogue_strlcpy(tbuf, "you", sizeof tbuf);
	else
	    jrogue_strlcpy(tbuf, mname, sizeof tbuf);
	if (upper)
	    *tbuf = (char) toupper(*tbuf);
    }
    return tbuf;
}

/*
 * thunk:
 *	A missile hits a monster
 */
void
thunk(const THING *weap, const char *mname, int noend)
{
    if (to_death)
	return;

    if (jrogue_lang_is_japanese)
    {
	if (weap->o_type == WEAPON)
	    addmsg("%sは", weap_info[weap->o_which].oi_name);
	addmsg("%s", mname);
	addmsg("に命中した。");
    }
    else
    {
	if (weap->o_type == WEAPON)
	    addmsg("the %s hits ", weap_info[weap->o_which].oi_name);
	else
	    addmsg("you hit ");
	addmsg("%s", mname);
    }
    if (!noend)
	endmsg();
}

/*
 * hit:
 *	Print a message to indicate a successful hit
 */
void
hit(const char *er, const char *ee, int noend)
{
    int i;
    const char *s;

    if (to_death)
	return;

    if (jrogue_lang_is_japanese)
    {
	addmsg("%s", prname(er, TRUE));
	if (terse)
	    s = "の攻撃は命中した。";
	else
	{
	    addmsg("は%sに", prname(ee, FALSE));
	    i = rnd(4);
	    if (er != NULL)
		i += 4;
	    s = h_names[i];
	}
	addmsg("%s", s);
    }
    else
    {
	addmsg("%s", prname(er, TRUE));
	if (terse)
	    s = " hit";
	else
	{
	    i = rnd(4);
	    if (er != NULL)
		i += 4;
	    s = h_names[i];
	}
	addmsg("%s", s);
	if (!terse)
	    addmsg("%s", prname(ee, FALSE));
    }
    if (!noend)
	endmsg();
}

/*
 * miss:
 *	Print a message to indicate a poor swing
 */
void
miss(const char *er, const char *ee, int noend)
{
    int i;

    if (to_death)
	return;

    if (jrogue_lang_is_japanese)
    {
	addmsg("%sは", prname(er, TRUE));
	if (!terse)
	    addmsg("%sに", prname(ee, FALSE));
	if (terse)
	    i = 0;
	else
	    i = rnd(4);
	if (er != NULL)
	    i += 4;
	addmsg("%s", m_names[i]);
    }
    else
    {
	addmsg("%s", prname(er, TRUE));
	if (terse)
	    i = 0;
	else
	    i = rnd(4);
	if (er != NULL)
	    i += 4;
	addmsg("%s", m_names[i]);
	if (!terse)
	    addmsg(" %s", prname(ee, FALSE));
    }
    if (!noend)
	endmsg();
}

/*
 * bounce:
 *	A missile misses a monster
 */
void
bounce(const THING *weap, const char *mname, int noend)
{
    if (to_death)
	return;

    if (jrogue_lang_is_japanese)
    {
	if (weap->o_type == WEAPON)
	    addmsg("%sはそれて、", weap_info[weap->o_which].oi_name);
	addmsg("%s", mname);
	addmsg("には当たらなかった。");
    }
    else
    {
	if (weap->o_type == WEAPON)
	    addmsg("the %s misses ", weap_info[weap->o_which].oi_name);
	else
	    addmsg("you missed ");
	addmsg("%s", mname);
    }
    if (!noend)
	endmsg();
}


/*
 * remove_mon:
 *	Remove a monster from the screen
 */
void
remove_mon(const coord *mp, THING *tp, int waskill)
{
    THING *obj, *nexti;

    for (obj = tp->t_pack; obj != NULL; obj = nexti)
    {
	nexti = next(obj);
	obj->o_pos = tp->t_pos;
	detach(tp->t_pack, obj);
	if (waskill)
	    fall(obj, FALSE);
	else
	    discard_object(obj);
    }
    moat(mp->y, mp->x) = NULL;
    jrogue_mvaddch(mp->y, mp->x, tp->t_oldch);
    detach(mlist, tp);
    if (on(*tp, ISTARGET))
    {
	kamikaze = FALSE;
	to_death = FALSE;
	if (fight_flush)
	    flush_type();
    }
    discard(tp);
}

/*
 * killed:
 *	Called to put a monster to death
 */
void
killed(THING *tp, int pr)
{
    const char *mname;

    pstats.s_exp = jrogue_sat_add(pstats.s_exp, tp->t_stats.s_exp);

    /*
     * If the monster was a venus flytrap, un-hold him
     */
    switch (tp->t_type)
    {
	case 'F':
	    player.t_flags &= ~ISHELD;
	    vf_hit = 0;
	    jrogue_strlcpy(monsters['F'-'A'].m_stats.s_dmg, "000x0", sizeof monsters['F'-'A'].m_stats.s_dmg);
	when 'L':
	{
	    THING *gold;

	    if (fallpos(&tp->t_pos, &tp->t_room->r_gold) && level >= max_level)
	    {
		gold = new_item();
		gold->o_type = GOLD;
		gold->o_goldval = GOLDCALC;
		if (save(VS_MAGIC))
		{
		    int k;
		    for (k = 0; k < 4; k++)
			gold->o_goldval = jrogue_sat_add(gold->o_goldval, GOLDCALC);
		}
		attach(tp->t_pack, gold);
	    }
	}
    }
    /*
     * Get rid of the monster.
     */
    mname = set_mname(tp);
    remove_mon(&tp->t_pos, tp, TRUE);
    if (pr)
    {
if (jrogue_lang_is_japanese)
	{
	    if (has_hit)
	    {
		addmsg("");
		has_hit = FALSE;
	    }
	    else
	    {
		if (!terse)
		    addmsg("あなたは");
	    }
	    msg("%sを倒した。", mname);
	}
	else
	{
	    if (has_hit)
	    {
		addmsg(".  Defeated ");
		has_hit = FALSE;
	    }
	    else
	    {
		if (!terse)
		    addmsg("you have ");
		addmsg("defeated ");
	    }
	    msg("%s", mname);
	}
    }
    /*
     * Do adjustments if he went up a level
     */
    check_level();
    if (fight_flush)
	flush_type();
}
