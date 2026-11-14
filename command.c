/*
 * Read and execute the user commands
 *
 * @(#)command.c	4.73 (Berkeley) 08/06/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <limits.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "rogue.h"
#include "jec/unicode_text.h"

/* Coordinates used by commands that inspect adjacent map cells. */
static int
command_map_position_ok(int y, int x)
{
    return x >= 0 && x < NUMCOLS && y > 0 && y < NUMLINES - 1;
}

/* curses unctrl() and the C character functions accept only byte values. */
static int
command_input_is_byte(int ch)
{
    return ch >= 0 && ch <= UCHAR_MAX;
}

/*
 * command:
 *	Process the user commands
 */
void
command(void)
{
    int ch;
    int ntimes = 1;			/* Number of player moves */
    int *fp;
    THING *mp;
    static int countch, direction, newcount = FALSE;

    if (on(player, ISHASTE))
	ntimes++;
    /*
     * Let the daemons start up
     */
    do_daemons(BEFORE);
    do_fuses(BEFORE);
    while (ntimes--)
    {
	if (color_mode != color_mode_applied)
	    jrogue_refresh_display();
	again = FALSE;
	if (has_hit)
	{
	    endmsg();
	    has_hit = FALSE;
	}
	/*
	 * these are illegal things for the player to be, so if any are
	 * set, someone's been poking in memeory
	 */
	if (on(player, ISSLOW|ISGREED|ISINVIS|ISREGEN|ISTARGET))
	    exit(1);

	look(TRUE);
	if (!running)
	    door_stop = FALSE;
	if (running && !jump)
	    msleep(24);
	status();
	lastscore = purse;
	move(hero.y, hero.x);
	if (!((running || count) && jump))
	{
	    jrogue_restore_cursor();
	    refresh();			/* Draw screen */
	}
	take = 0;
	after = TRUE;
	/*
	 * Read command or continue run
	 */
#ifdef MASTER
	if (wizard)
	    noscore = TRUE;
#endif
	if (!no_command)
	{
	    if (running || to_death)
		ch = runch;
	    else if (count)
		ch = countch;
	    else
	    {
		ch = readchar();
		move_on = FALSE;
		if (mpos != 0)		/* Erase message if its there */
		{
		    if (ch != '.')
			msg("");
		}
	    }
	}
	else
	    ch = '.';
	if (no_command)
	{
	    if (--no_command == 0)
	    {
		player.t_flags |= ISRUN;
		msg(JSTR("また動けるようになった。", "you can move again"));
	    }
	}
	else
	{
	    /*
	     * check for prefixes
	     */
	    newcount = FALSE;
	    if (ch >= '0' && ch <= '9')
	    {
		count = 0;
		newcount = TRUE;
		while (ch >= '0' && ch <= '9')
		{
		    count = count * 10 + (ch - '0');
		    if (count > 255)
			count = 255;
		    ch = readchar();
		}
		countch = ch;
		/*
		 * turn off count for commands which don't make sense
		 * to repeat
		 */
		switch (ch)
		{
		    case CTRL('B'): case CTRL('H'): case CTRL('J'):
		    case CTRL('K'): case CTRL('L'): case CTRL('N'):
		    case CTRL('U'): case CTRL('Y'):
		    case '.': case 'a': case 'b': case 'h': case 'j':
		    case 'k': case 'l': case 'm': case 'n': case 'q':
		    case 'r': case 's': case 't': case 'u': case 'y':
		    case 'z': case 'B': case 'C': case 'H': case 'I':
		    case 'J': case 'K': case 'L': case 'N': case 'U':
		    case 'Y':
#ifdef MASTER
		    case CTRL('D'): case CTRL('A'):
#endif
			break;
		    default:
			count = 0;
		}
	    }
	    /*
	     * execute a command
	     */
	    if (count && !running)
		count--;
	    if (ch != 'a' && ch != ESCAPE && ch != INVALID_INPUT && !(running || count || to_death))
	    {
		l_last_comm = last_comm;
		l_last_dir = last_dir;
		l_last_pick = last_pick;
		last_comm = ch;
		last_dir = '\0';
		last_pick = NULL;
	    }
over:
	    switch (ch)
	    {
		case ',': {
		    THING *obj = NULL;
		    int found = 0;
		    for (obj = lvl_obj; obj != NULL; obj = next(obj))
    			{
			    if (obj->o_pos.y == hero.y && obj->o_pos.x == hero.x)
			    {
				found=1;
				break;
			    }
    			}

		    if (found) {
			if (levit_check())
			    ;
			else
			    pick_up(obj->o_type);
		    }
		    else {
		    if (jrogue_lang_is_japanese) {
			addmsg("ここには");
			if (!terse)
			    addmsg("拾えるものは");
			addmsg("何もない。");
		    }
		    else {
			/* RRP 5.4.5: terse uses only "nothing here". */
			if (!terse)
			    addmsg("there is ");
			addmsg("nothing here");
			if (!terse)
			    addmsg(" to pick up");
		    }
                        endmsg();
		    }
		}
		when '!': shell();
		when 'h': do_move(0, -1);
		when 'j': do_move(1, 0);
		when 'k': do_move(-1, 0);
		when 'l': do_move(0, 1);
		when 'y': do_move(-1, -1);
		when 'u': do_move(-1, 1);
		when 'b': do_move(1, -1);
		when 'n': do_move(1, 1);
		when 'H': do_run('h');
		when 'J': do_run('j');
		when 'K': do_run('k');
		when 'L': do_run('l');
		when 'Y': do_run('y');
		when 'U': do_run('u');
		when 'B': do_run('b');
		when 'N': do_run('n');
		when CTRL('H'): case CTRL('J'): case CTRL('K'): case CTRL('L'):
		case CTRL('Y'): case CTRL('U'): case CTRL('B'): case CTRL('N'):
		{
		    if (!on(player, ISBLIND))
		    {
			door_stop = TRUE;
			firstmove = TRUE;
		    }
		    if (count && !newcount)
			ch = direction;
		    else
		    {
			ch += ('A' - CTRL('A'));
			direction = ch;
		    }
		    goto over;
		}
		when 'F':
		    kamikaze = TRUE;
		    /* FALLTHROUGH */
		case 'f':
		    if (!get_dir())
		    {
			after = FALSE;
			break;
		    }
		    delta.y += hero.y;
		    delta.x += hero.x;
		    /* The direction is legal, but the target may be off-map. */
		    mp = command_map_position_ok(delta.y, delta.x)
			? moat(delta.y, delta.x) : NULL;
		    if (mp == NULL
			|| ((!see_monst(mp)) && !on(player, SEEMONST)))
		    {
			if (!terse)
			    addmsg(JSTR("その方向には", "I see "));
			msg(JSTR("怪物がいない。", "no monster there"));
			after = FALSE;
		    }
		    else if (diag_ok(&hero, &delta))
		    {
			to_death = TRUE;
			max_hit = 0;
			mp->t_flags |= ISTARGET;
			runch = ch = dir_ch;
			goto over;
		    }
		when 't':
		    if (!get_dir())
			after = FALSE;
		    else
			missile(delta.y, delta.x);
		when 'a':
		    if (last_comm == '\0')
		    {
			msg(JSTR("まだコマンドを入力していない。", "you haven't typed a command yet"));
			after = FALSE;
		    }
		    else
		    {
			ch = last_comm;
			again = TRUE;
			goto over;
		    }
		when 'q': quaff();
		when 'Q':
		    after = FALSE;
		    q_comm = TRUE;
		    quit(0);
		    q_comm = FALSE;
		when 'i': after = FALSE; inventory(pack, 0);
		when 'I': after = FALSE; picky_inven();
		when 'd': drop();
		when 'r': read_scroll();
		when 'e': eat();
		when 'w': wield();
		when 'W': wear();
		when 'T': take_off();
		when 'P': ring_on();
		when 'R': ring_off();
		when 'o':
		    option();
		    after = FALSE;
		when 'c':
		    call();
		    after = FALSE;
		when '>': after = FALSE; d_level();
		when '<': after = FALSE; u_level();
		when '?': after = FALSE; help();
		when '/': after = FALSE; identify();
		when 's': search();
		when 'z':
		    if (get_dir())
			do_zap();
		    else
			after = FALSE;
		when 'D': after = FALSE; discovered();
		when CTRL('P'): after = FALSE; msg("%s", jrogue_last_message());
		when CTRL('R'):
		    after = FALSE;
		    clearok(curscr,TRUE);
		    wrefresh(curscr);
		when 'v':
		    after = FALSE;
		    msg("version %s. (mctesq was here)", release);
		when 'S': 
		    after = FALSE;
		    save_game();
		when '.': ;			/* Rest command */
		when ' ': after = FALSE;	/* "Legal" illegal command */
		when '^':
		    after = FALSE;
		    if (get_dir()) {
			delta.y += hero.y;
			delta.x += hero.x;
                        if (!terse)
                            addmsg(JSTR("その方向には", "You have found "));
			if (!command_map_position_ok(delta.y, delta.x)
			    || chat(delta.y, delta.x) != TRAP)
			    msg(JSTR("わながない。", "no trap there"));
			else if (on(player, ISHALU))
			    msg("%s", tr_name[rnd(NTRAPS)]);
			else {
			    fp = &flat(delta.y, delta.x);
			    msg("%s", tr_name[*fp & F_TMASK]);
			    *fp |= F_SEEN;
			}
		    }
#ifdef MASTER
		when '+':
		    after = FALSE;
		    if (wizard)
		    {
			wizard = FALSE;
			turn_see(TRUE);
			msg(JSTR("もはや、魔法使いではない。", "not wizard any more"));
		    }
		    else
		    {
			wizard = passwd();
			if (wizard) 
			{
			    noscore = TRUE;
			    turn_see(FALSE);
			    msg(JSTR("あなたは洞窟（#%d）の中で、突然ケン・アーノルドのように賢くなった。", "you are suddenly as smart as Ken Arnold in dungeon #%d"), dnum);
			}
			else
			    msg(JSTR("残念でした。", "sorry"));
		    }
#endif
		when ESCAPE:	/* Escape */
		    door_stop = FALSE;
		    count = 0;
		    after = FALSE;
		    again = FALSE;
		when 'm':
		    move_on = TRUE;
		    if (!get_dir())
			after = FALSE;
		    else
		    {
			ch = dir_ch;
			countch = dir_ch;
			goto over;
		    }
		when ')': current(cur_weapon, JSTR("使っている", "wielding"), NULL);
		when ']': current(cur_armor, JSTR("着ている", "wearing"), NULL);
		when '=':
		    current(cur_ring[LEFT], JSTR("はめている", "wearing"),
					    terse ? JSTR("（左手）", "(L)") : JSTR("（左の手）", "on left hand"));
		    current(cur_ring[RIGHT], JSTR("はめている", "wearing"),
					    terse ? JSTR("（右手）", "(R)") : JSTR("（右の手）", "on right hand"));
		when '@':
		    stat_msg = TRUE;
		    status();
		    stat_msg = FALSE;
		    after = FALSE;
		otherwise:
		    after = FALSE;
#ifdef MASTER
		    if (wizard)
		    {
			switch (ch)
			{
			case '|': msg("@ %d,%d", hero.y, hero.x);
			when 'C': create_obj();
			when '$': msg(JSTR("荷物の数：%d", "inpack = %d"), inpack);
			when CTRL('G'): inventory(lvl_obj, 0);
			when CTRL('W'): whatis(FALSE, 0);
			when CTRL('D'): if (level < JROGUE_MAX_DUNGEON_LEVEL) { level++; new_level(); }
			when CTRL('A'): if (level > 1) { level--; new_level(); }
			when CTRL('F'): show_map();
			when CTRL('T'): teleport();
			when CTRL('E'): msg(JSTR("食べものの残り：%d", "food left: %d"), food_left);
			when CTRL('Q'): add_pass();
			when CTRL('X'): turn_see(on(player, SEEMONST));
			when '~':
			{
			    THING *item;

			    if ((item = get_item(JSTR("つえをチャージする", "charge"), STICK)) != NULL)
				item->o_charges = 10000;
			}
			when CTRL('I'):
			{
			    int i;
			    THING *obj;

			    for (i = 0; i < 9; i++)
				raise_level();
			    /*
			     * Give him a sword (+1,+1)
			     */
			    obj = new_item();
			    init_weapon(obj, TWOSWORD);
			    obj->o_hplus = 1;
			    obj->o_dplus = 1;
			    add_pack(obj, TRUE);
			    cur_weapon = obj;
			    /*
			     * And his suit of armor
			     */
			    obj = new_item();
			    obj->o_type = ARMOR;
			    obj->o_which = PLATE_MAIL;
			    obj->o_arm = -5;
			    obj->o_flags |= ISKNOW;
			    obj->o_count = 1;
			    obj->o_group = 0;
			    cur_armor = obj;
			    add_pack(obj, TRUE);
			}
			when '*' :
			    pr_list();
			otherwise:
			    illcom(ch);
			}
			jrogue_restore_cursor();
		    }
		    else
#endif
			illcom(ch);
	    }
	    /*
	     * turn off flags if no longer needed
	     */
	    if (!running)
		door_stop = FALSE;
	}
	/*
	 * If he ran into something to take, let him pick it up.
	 */
	if (take != 0)
	    pick_up(take);
	if (!running)
	    door_stop = FALSE;
	if (!after)
	    ntimes++;
    }
    do_daemons(AFTER);
    do_fuses(AFTER);
    if (ISRING(LEFT, R_SEARCH))
	search();
    else if (ISRING(LEFT, R_TELEPORT) && rnd(50) == 0)
	teleport();
    if (ISRING(RIGHT, R_SEARCH))
	search();
    else if (ISRING(RIGHT, R_TELEPORT) && rnd(50) == 0)
	teleport();
}

/*
 * illcom:
 *	What to do with an illegal command
 */
void
illcom(int ch)
{
    save_msg = FALSE;
    count = 0;
    if (!command_input_is_byte(ch))
	msg(JSTR("入力エラーです。", "input error"));
    else
	msg(JSTR("'%s' コマンドが違います。", "illegal command '%s'"), unctrl(ch));
    save_msg = TRUE;
}

/*
 * search:
 *	player gropes about him to find hidden things.
 */
void
search(void)
{
    int y, x;
    int *fp;
    int ey, ex;
    int probinc;
    int found;

    ey = hero.y + 1;
    ex = hero.x + 1;
    probinc = (on(player, ISHALU) ? 3 : 0);
    probinc += (on(player, ISBLIND) ? 2 : 0);
    found = FALSE;
    for (y = hero.y - 1; y <= ey; y++) 
	for (x = hero.x - 1; x <= ex; x++)
	{
	    if ((y == hero.y && x == hero.x)
		|| !command_map_position_ok(y, x))
		continue;
	    fp = &flat(y, x);
	    if (!(*fp & F_REAL))
		switch (chat(y, x))
		{
		    case '|':
		    case '-':
			if (rnd(5 + probinc) != 0)
			    break;
			chat(y, x) = DOOR;
                        msg(JSTR("隠しとびらを見つけた。", "a secret door"));
foundone:
			found = TRUE;
			*fp |= F_REAL;
			count = FALSE;
			running = FALSE;
			break;
		    case FLOOR:
			if (rnd(2 + probinc) != 0)
			    break;
			chat(y, x) = TRAP;
			/* RRP 5.4.5 puts "you found " before the trap name.
			 * Japanese needs the suffix after the trap name instead. */
			if (!jrogue_lang_is_japanese && !terse)
			    addmsg("you found ");
			if (on(player, ISHALU))
			    addmsg("%s", tr_name[rnd(NTRAPS)]);
			else {
			    addmsg("%s", tr_name[*fp & F_TMASK]);
			    *fp |= F_SEEN;
			}
			if (jrogue_lang_is_japanese && !terse)
			    addmsg("を見つけた。");
			endmsg();
			goto foundone;
			break;
		    case ' ':
			if (rnd(3 + probinc) != 0)
			    break;
			chat(y, x) = PASSAGE;
			goto foundone;
		}
	}
    if (found)
	look(FALSE);
}

/*
 * help:
 *	Give single character help, or the whole mess if he wants it
 */
void
help(void)
{
    const struct h_list *strp;
    int helpch;
    int numprint, cnt;
    msg(JSTR("どのコマンドのヘルプを表示しますか？ ('*'で全て表示): ", "character you want help for (* for all): "));
    helpch = readchar();
    mpos = 0;
    if (!command_input_is_byte(helpch))
    {
	msg(JSTR("入力エラーです。", "input error"));
	return;
    }
    /*
     * If its not a *, print the right help string
     * or an error if he typed a funny character.
     */
    if (helpch != '*')
    {
	move(0, 0);
	for (strp = helpstr; strp->h_desc != NULL; strp++)
	    if (strp->h_ch == helpch)
	    {
		lower_msg = TRUE;
		msg("%s%s", unctrl(strp->h_ch), strp->h_desc);
		lower_msg = FALSE;
		return;
	    }
	msg(JSTR("'%s' コマンドが違います。", "unknown character '%s'"), unctrl(helpch));
	return;
    }
    /*
     * Here we print help for everything.
     * Then wait before we return to command mode
     */
    numprint = 0;
    for (strp = helpstr; strp->h_desc != NULL; strp++)
	if (strp->h_print)
	    numprint++;
    if (numprint & 01)		/* round odd numbers up */
	numprint++;
    numprint /= 2;
    if (numprint > _LINES - 1)
	numprint = _LINES - 1;

    wclear(hw);
    jrogue_wset_normal_attr(hw);
    cnt = 0;
    for (strp = helpstr; strp->h_desc != NULL; strp++)
	if (strp->h_print)
	{
	    wmove(hw, cnt % numprint, cnt >= numprint ? _COLS / 2 : 0);
	    if (strp->h_ch)
		waddstr(hw, unctrl(strp->h_ch));
	    waddstr(hw, strp->h_desc);
	    if (++cnt >= numprint * 2)
		break;
	}
    wmove(hw, _LINES - 1, 0);
    waddstr(hw, JSTR("--スペースを押してください--", "--Press space to continue--"));
    wrefresh(hw);
    jrogue_show_cursor(TRUE);
    wait_for(hw, ' ');
    jrogue_restore_cursor();
    clearok(stdscr, TRUE);
/*
    refresh();
*/
    msg("");
    touchwin(stdscr);
    wrefresh(stdscr);
}

/*
 * identify:
 *	Tell the player what a certain thing is.
 */
void
identify(void)
{
    int ch;
    const struct h_list *hp;
    const char *str;
    const struct h_list ident_list[] = {
	{'|',		JSTR("部屋の壁", "wall of a room"),		FALSE},
	{'-',		JSTR("部屋の壁", "wall of a room"),		FALSE},
	{GOLD,		JSTR("金塊", "gold"),				FALSE},
	{STAIRS,	JSTR("隣の階への階段", "a staircase"),			FALSE},
	{DOOR,		JSTR("部屋のドア", "door"),				FALSE},
	{FLOOR,		JSTR("部屋の床", "room floor"),			FALSE},
	{PLAYER,	JSTR("プレイヤー", "you"),				FALSE},
	{PASSAGE,	JSTR("部屋と部屋をむすぶ通路", "passage"),			FALSE},
	{TRAP,		JSTR("隠されたわな", "trap"),				FALSE},
	{POTION,	JSTR("水薬", "potion"),			FALSE},
	{SCROLL,	JSTR("巻き物", "scroll"),			FALSE},
	{FOOD,		JSTR("食糧", "food"),				FALSE},
	{WEAPON,	JSTR("武器", "weapon"),			FALSE},
	{' ',		JSTR("硬い岩", "solid rock"),			FALSE},
	{ARMOR,		JSTR("よろい", "armor"),			FALSE},
	{AMULET,	JSTR("イェンダーの魔除け", "the Amulet of Yendor"),		FALSE},
	{RING,		JSTR("魔法の指輪", "ring"),				FALSE},
	{STICK,		JSTR("魔法の杖か棒", "wand or staff"),		FALSE},
	{'\0'}
    };

    msg(JSTR("どの文字？", "what do you want identified? "));
    ch = readchar();
    mpos = 0;
    if (ch == ESCAPE)
    {
	msg("");
	return;
    }
    if (!command_input_is_byte(ch))
    {
	msg(JSTR("入力エラーです。", "input error"));
	return;
    }
    if (ch >= 'A' && ch <= 'Z')
	str = monsters[ch-'A'].m_name;
    else
    {
	str = JSTR("その文字は使われていない", "unknown character");
	for (hp = ident_list; hp->h_ch != '\0'; hp++)
	    if (hp->h_ch == ch)
	    {
		str = hp->h_desc;
		break;
	    }
    }
    msg("'%s': %s", unctrl(ch), str);
}

/*
 * d_level:
 *	He wants to go down a level
 */
void
d_level(void)
{
    if (levit_check())
	return;
    if (chat(hero.y, hero.x) != STAIRS)
	msg(JSTR("降りる階段がない。", "I see no way down"));
    else
    {
	if (level >= JROGUE_MAX_DUNGEON_LEVEL)
	{
	    msg(JSTR("これ以上深くは進めない。", "you cannot descend any deeper"));
	    return;
	}
	level++;
	seenstairs = FALSE;
	new_level();
    }
}

/*
 * u_level:
 *	He wants to go up a level
 */
void
u_level(void)
{
    if (levit_check())
	return;
    if (chat(hero.y, hero.x) == STAIRS)
	if (amulet)
	{
	    level--;
	    if (level == 0)
		total_winner();
	    new_level();
	    msg(JSTR("はらわたをねじ切られるような感じだ。", "you feel a wrenching sensation in your gut"));
	}
	else
	    msg(JSTR("階段は不思議な力でふさがれている。", "your way is magically blocked"));
    else
	msg(JSTR("登る階段がない。", "I see no way up"));
}

/*
 * levit_check:
 *	Check to see if she's levitating, and if she is, print an
 *	appropriate message.
 */
int
levit_check(void)
{
    if (!on(player, ISLEVIT))
	return FALSE;
    msg(JSTR("それはできない。宙に浮いている！", "You can't.  You're floating off the ground!"));
    return TRUE;
}

/*
 * call:
 *	Allow a user to call a potion, scroll, or ring something
 */
void
call(void)
{
    THING *obj;
    struct obj_info *op = NULL;
    char **guess;
    const char *elsewise = NULL;
    int *know;

    obj = get_item(JSTR("持ちものに呼び名をつける", "call"), CALLABLE);
    /*
     * Make certain that it is somethings that we want to wear
     */
    if (obj == NULL)
	return;
    switch (obj->o_type)
    {
	case RING:
	    op = &ring_info[obj->o_which];
	    elsewise = r_stones[obj->o_which];
	    goto norm;
	when POTION:
	    op = &pot_info[obj->o_which];
	    elsewise = p_colors[obj->o_which];
	    goto norm;
	when SCROLL:
	    op = &scr_info[obj->o_which];
	    elsewise = s_names[obj->o_which];
	    goto norm;
	when STICK:
	    op = &ws_info[obj->o_which];
	    elsewise = ws_made[obj->o_which];
norm:
	    know = &op->oi_know;
	    guess = &op->oi_guess;
	    if (*guess != NULL)
		elsewise = *guess;
	when FOOD:
	    msg(JSTR("それには呼び名をつけることができない。", "you can't call that anything"));
	    return;
	otherwise:
	    guess = &obj->o_label;
	    know = NULL;
	    elsewise = obj->o_label;
    }
    if (know != NULL && *know)
    {
	msg(JSTR("それはすでに鑑別されている。", "that has already been identified"));
	return;
    }
    if (elsewise != NULL && elsewise == *guess)
    {
	if (!jrogue_lang_is_japanese && !terse)
	    addmsg("Was ");
        char preview[MAXSTR] = "";
        /* The old saved name may be longer than one message line. */
        jrogue_text_append_labeled(preview, sizeof preview,
                                   NUMCOLS - 1 -
                                   jrogue_text_width(JSTR("\"\"という呼び名が付いている",
                                                                 "Was called \"\"")),
                                   "", elsewise, "");
        msg(JSTR("\"%s\"という呼び名が付いている", "called \"%s\""), preview);
    }
    if (terse)
	msg(JSTR("呼び名は？", "call it: "));
    else
	msg(JSTR("どんな呼び名をつける？", "what do you want to call it? "));
    jrogue_show_cursor(TRUE);
    /* This is a fresh answer, not an option value being edited.
     * Empty Enter means "leave the old call unchanged". */
    prbuf[0] = '\0';
    jrogue_set_normal_attr();
    if (get_str_called_name(prbuf, stdscr) == NORM && prbuf[0] != '\0')
    {
        size_t bytes = strlen(prbuf) + 1;
        char *replacement = malloc(bytes);
        if (replacement != NULL)
        {
            memcpy(replacement, prbuf, bytes);
            free(*guess);
            *guess = replacement;
        }
    }
    msg("");
    jrogue_restore_cursor();
}

/*
 * current:
 *	Print the current weapon/armor
 */
void
current(const THING *cur, const char *how, const char *where)
{
    after = FALSE;
    if (cur != NULL)
    {
	if (jrogue_lang_is_japanese)
	{
	    inv_describe = FALSE;
	    addmsg("%c) %s", cur->o_packch, inv_name(cur, TRUE));
	    if (!terse)
		addmsg("（%s）", how);
	    inv_describe = TRUE;
	}
	else
	{
	    if (!terse)
		addmsg("you are %s (", how);
	    inv_describe = FALSE;
	    addmsg("%c) %s", cur->o_packch, inv_name(cur, TRUE));
	    inv_describe = TRUE;
	}
	if (where)
	    addmsg(" %s", where);
	endmsg();
    }
    else
    {
	if (jrogue_lang_is_japanese)
	{
	    if (strcmp(how, "使っている") == 0)
		addmsg("武器を手にしていない。");
	    else if (strcmp(how, "着ている") == 0)
		addmsg("よろいを身につけていない。");
	    else if (where != NULL && strstr(where, "左") != NULL)
		addmsg("左手に指輪をはめていない。");
	    else if (where != NULL && strstr(where, "右") != NULL)
		addmsg("右手に指輪をはめていない。");
	    else
		addmsg("指輪をはめていない。");
	}
	else
	{
	    if (!terse)
		addmsg("you are ");
	    addmsg("%s nothing", how);
	    if (where)
		addmsg(" %s", where);
	}
	endmsg();
    }
}
