/*
 * File for the fun ends
 * Death or a total win
 *
 * @(#)rip.c	4.57 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <curses.h>
#include <locale.h>
#include "rogue.h"
#include "score.h"
#include "jec/unicode_text.h"

int jrogue_score_waited_for_exit = FALSE;

static const char *rip[] = {
"                       __________  \n",
"                      /          \\\n",
"                     /            \\\n",
JSTR_STATIC("                    /   安らかに   \\\n", "                    /              \\\n"),
JSTR_STATIC("                   /      眠れ      \\\n", "                   /                \\\n"),
"                  /                  \\\n",
"                  |                  |\n",
"                  |                  |\n",
"                  |                  |\n",
JSTR_STATIC("                  |    により死す    |\n", "                  |                  |\n"),
"                  |       1980       |\n",
"                 *|     *  *  *      | *\n",
"         ________)/\\\\_//(\\/(/\\)/\\//\\/|_)_______\n",
    0
};
void
jrogue_apply_rip_language(void)
{
    rip[3] = JSTR("                    /   安らかに   \\\n", "                    /              \\\n");
    rip[4] = JSTR("                   /      眠れ      \\\n", "                   /                \\\n");
    rip[9] = JSTR("                  |    により死す    |\n", "                  |                  |\n");
}




static void
score_reset_attrs(int use_color)
{
#ifdef _WIN32
    /* Raw score output uses VT RGB, or Win32 attributes when stdout
     * cannot enable VT; this is independent of the curses backend. */
    md_raw_score_normal(use_color);
#else
    if (use_color)
        printf("\033[0m\033[37m%s", jrogue_black_background_ansi());
    else
        printf("\033[0m");
#endif
}



static void
score_print_list(SCORE *top_ten, SCORE *endp, SCORE *highlight,
    int flags, int prflags, const char **reason, int use_color,
    SCORE *deleted, int *deleted_count)
{
    SCORE *scp, *sc2;
    int row_deleted;
    char buf[2 * MAXSTR];
    char score_name[MAXSTR];
    char score_prefix[64];
    char score_suffix[MAXSTR];
    const char *killer;
#ifdef MASTER
    int i;
#endif

#ifndef MASTER
    (void) prflags;
#endif

    sc2 = highlight;
#ifndef MASTER
    (void) deleted;
    (void) deleted_count;
#endif
    if (flags != -1)
	putchar('\n');
    score_reset_attrs(use_color);
    if (use_color)
    {
#ifdef _WIN32
        md_raw_score_color(221);
#else
        printf("\x1b[38;5;221m");
#endif
    }
    printf(JSTR("トップ%s %s:\n", "Top %s %s:\n"),
        JSTR("テン", "Ten"),
        allscore ? JSTR("スコア", "Scores") : JSTR("冒険者", "Rogueists"));
    if (use_color)
    {
#ifdef _WIN32
        md_raw_score_color(114);
#else
        printf("\x1b[38;5;114m");
#endif
    }
    printf("%s", JSTR("    得点 名前\n", "   Score Name\n"));
    score_reset_attrs(use_color);
    for (scp = top_ten; scp < endp; )
    {
        row_deleted = 0;
	if (scp->sc_score) {
            /* Construct the fixed score information separately and reserve
             * its cells before adding the name.  This prevents old, long
             * UTF-8 names from wrapping or losing the cause of death. */
            snprintf(score_prefix, sizeof score_prefix, "%2d %5d ",
                     (int)(scp - top_ten + 1), scp->sc_score);
            jrogue_player_name_display(score_name, sizeof score_name,
                                       scp->sc_name,
                                       JROGUE_PLAYER_NAME_MAX_CELLS);
            if (jrogue_lang_is_japanese)
            {
                if (scp->sc_death_id == SC_DEATH_KILLED ||
                    scp->sc_death_id == SC_DEATH_AMULET)
                {
                    killer = killname(scp->sc_killer_id, FALSE);
                    snprintf(score_suffix, sizeof score_suffix,
                             ": 地下%d階にて%s%s%s", scp->sc_level,
                             scp->sc_death_id == SC_DEATH_AMULET ? "魔除けを手に、" : "",
                             killer,
                             isupper(scp->sc_killer_id) ? "と戦いて死す" :
                             reason[scp->sc_death_id]);
                }
                else
                    snprintf(score_suffix, sizeof score_suffix,
                             ": 地下%d階にて%s", scp->sc_level,
                             reason[scp->sc_death_id]);
            }
            else if (scp->sc_death_id == SC_DEATH_KILLED ||
                     scp->sc_death_id == SC_DEATH_AMULET)
                snprintf(score_suffix, sizeof score_suffix,
                         ": %s on level %d by %s", reason[scp->sc_death_id],
                         scp->sc_level, killname(scp->sc_killer_id, TRUE));
            else
                snprintf(score_suffix, sizeof score_suffix,
                         ": %s on level %d", reason[scp->sc_death_id],
                         scp->sc_level);
            buf[0] = '\0';
            /* Final punctuation is printed below (two cells in Japanese).
             * Leave the terminal's last column untouched to avoid a wrap. */
            jrogue_text_append_labeled(buf, sizeof buf,
                                       jrogue_lang_is_japanese ? 77 : 78,
                                       score_prefix, score_name, score_suffix);
	    if (sc2 == scp)
	        md_raw_standout(use_color);

            printf("%s", buf);

#ifdef MASTER
	    if (prflags == 1)
	    {
                snprintf(buf, sizeof buf, " (%s)", md_getrealname(scp->sc_uid));
	        printf("%s", buf);
	    }
	    else if (prflags == 2)
	    {
		fflush(stdout);
		(void) fgets(prbuf,10,stdin);
		if (prbuf[0] == 'd')
		{
                    /* Remember the exact row, then reapply this deletion
                     * to a fresh scoreboard under a new lock.  Do not hold
                     * a file lock while waiting for wizard key input. */
                    if (deleted != NULL && deleted_count != NULL &&
                        *deleted_count < numscores)
                        deleted[(*deleted_count)++] = *scp;
		    for (sc2 = scp; sc2 < endp - 1; sc2++)
			*sc2 = *(sc2 + 1);
		    sc2 = endp - 1;
		    sc2->sc_score = 0;
		    for (i = 0; i < MAXSTR; i++)
			sc2->sc_name[i] = (char) rnd(255);
		    sc2->sc_death_id = RN;
		    sc2->sc_level = RN;
		    sc2->sc_killer_id = RN;
                    row_deleted = 1;
		}
	    }
	    else
#endif /* MASTER */
if (jrogue_lang_is_japanese)
            {
                printf("。");
            }
            else
            {
                printf(".");
            }
	    if (highlight == scp)
            {
		md_raw_standend(use_color);
            }
            putchar('\n');
            if (!row_deleted)
                ++scp;
	}
	else
	    break;
    }
    score_reset_attrs(use_color);
}

/*
 * A score is ranked and recorded only once in score().  Both display
 * modes use the same formatter; only terminal preparation and the optional
 * alternate-screen pass differ.
 * On Windows, endwin() may leave the grave/winner image on the regular
 * console screen; clear it before any raw score output there.
 */
static void
score_enter_alternate_screen(void)
{
#ifdef _WIN32
    /* Windows VT output uses the RGB black background of the score view. */
    fputs("\033[?1049h\033[48;2;0;0;0m\033[H\033[2J", stdout);
#else
    printf("\033[?1049h%s\033[H\033[2J", jrogue_black_background_ansi());
#endif
}

static void
score_leave_alternate_screen(void)
{
    fputs("\033[?1049l", stdout);
    fflush(stdout);
}

static void
score_prepare_screen(int had_game_screen, int use_alternate)
{
    if (use_alternate)
    {
        score_enter_alternate_screen();
        return;
    }
#ifdef _WIN32
    if (had_game_screen)
        md_raw_score_clear_screen();
#else
    (void)had_game_screen;
#endif
}

static void
score_wait_for_exit(void)
{
    printf("\n%s", JSTR("［リターンを押してください］",
                       "[Press return to exit]\n"));
    fflush(stdout);
    (void) fgets(prbuf, 80, stdin);
    jrogue_score_waited_for_exit = TRUE;
}

static void
score_show_colored_then_record(SCORE *top_ten, SCORE *endp, SCORE *highlight,
    int flags, int prflags, const char **reason)
{
    int use_alternate = TRUE;

#ifdef _WIN32
    /* Win32 console attributes are used without an alternate screen when
     * stdout does not support VT.  Do not emit literal VT escapes there. */
    use_alternate = md_raw_score_uses_vt();
#endif
    score_prepare_screen(flags != -1, use_alternate);
    score_print_list(top_ten, endp, highlight, flags, prflags, reason,
                     TRUE, NULL, NULL);
    score_wait_for_exit();

    if (use_alternate)
    {
        /* Restore normal terminal content, then replace the old game image
         * with a plain score copy that remains in terminal scrollback. */
        score_reset_attrs(TRUE);
        score_leave_alternate_screen();
        score_prepare_screen(flags != -1, FALSE);
        score_print_list(top_ten, endp, highlight, flags, prflags, reason,
                         FALSE, NULL, NULL);
    }
    else
    {
        /* Non-VT Windows stays on its regular console screen. */
        score_reset_attrs(FALSE);
    }
}

#ifdef MASTER
/* Apply wizard deletions against the most recent score contents, not an
 * outdated list displayed before another process updated the scoreboard. */
static int
score_same_entry(const SCORE *a, const SCORE *b)
{
    return a->sc_uid == b->sc_uid && a->sc_score == b->sc_score &&
           a->sc_death_id == b->sc_death_id &&
           a->sc_killer_id == b->sc_killer_id &&
           a->sc_level == b->sc_level && a->sc_time == b->sc_time &&
           strncmp(a->sc_name, b->sc_name, MAXSTR) == 0;
}

static int
score_apply_deletions(SCORE *list, const SCORE *deleted, int count)
{
    int i, j, updated = 0;

    for (i = 0; i < count; ++i)
        for (j = 0; j < numscores; ++j)
            if (list[j].sc_score != 0 &&
                score_same_entry(&list[j], &deleted[i]))
            {
                if (j + 1 < numscores)
                    memmove(&list[j], &list[j + 1],
                            (size_t)(numscores - j - 1) * sizeof *list);
                memset(&list[numscores - 1], 0, sizeof *list);
                ++updated;
                break;
            }
    return updated;
}
#endif

/*
 * score:
 *	Figure score and post it.
 */
/* VARARGS2 */

void
score(int amount, int flags, int monst)
{
    SCORE *scp;
    int i;
    SCORE *sc2;
    SCORE *top_ten, *endp;
    int prflags = 0;
    int score_locked = FALSE;
#ifdef MASTER
    SCORE *wizard_deleted = NULL;
    int wizard_deleted_count = 0;
#endif
    jrogue_uid_t uid;
    const char *reason[] = {
	JSTR("により死す", "killed"),
	JSTR("逃亡す", "quit"),
	JSTR("完全勝利者", "A total winner"),
	JSTR("により死す", "killed with Amulet")  // 魔除けを手に、
    };
    jrogue_score_waited_for_exit = FALSE;
    start_score();

 if (flags >= 0
#ifdef MASTER
            || wizard
#endif
        )
    {
	mvaddstr(_LINES - 1, 0 , JSTR("［リターンを押してください］", "[Press return to continue]"));
        jrogue_prepare_input_attr(stdscr);
        refresh();
        wgetnstr(stdscr,prbuf,80);
 	endwin();
        resetltchars();
	/*
	 * free up space to "guarantee" there is space for the top_ten
	 */
	delwin(stdscr);
	delwin(curscr);
	if (hw != NULL)
	    delwin(hw);
    }

#ifdef _WIN32
    /* Initialize Windows raw score output after curses has ended. */
    md_raw_score_prepare();
#endif

    top_ten = malloc(numscores * sizeof (SCORE));

	if (top_ten == NULL)
		return;

    endp = &top_ten[numscores];
    for (scp = top_ten; scp < endp; scp++)
    {
	scp->sc_score = 0;
	for (i = 0; i < MAXSTR; i++)
	    scp->sc_name[i] = (char) rnd(255);
	scp->sc_death_id = RN;
	scp->sc_level = RN;
	scp->sc_killer_id = RN;
	scp->sc_uid = RN;
    }

    signal(SIGINT, SIG_DFL);

#ifdef MASTER
    if (wizard)
    {
	if (strcmp(prbuf, "names") == 0)
	    prflags = 1;
	else if (strcmp(prbuf, "edit") == 0)
	    prflags = 2;
    }
#endif
    /* Lock before reading: another process may have just posted a score.
     * Keep the lock through ranking and writing, then release it before
     * the potentially long score display and all user input. */
    if (scoreboard != NULL)
        score_locked = lock_sc();
    if (score_locked)
        rd_score(top_ten);
    /*
     * Insert her in list if need be
     */
    sc2 = NULL;
    if (!noscore)
    {
	uid = md_getuid();
	for (scp = top_ten; scp < endp; scp++)
	    if (amount > scp->sc_score)
		break;
	    else if (!allscore &&	/* only one score per nowin uid */
		flags != SC_DEATH_WINNER && scp->sc_uid == uid && scp->sc_death_id != SC_DEATH_WINNER)
		    scp = endp;
	if (scp < endp)
	{
	    if (flags != SC_DEATH_WINNER && !allscore)
	    {
		for (sc2 = scp; sc2 < endp; sc2++)
		{
		    if (sc2->sc_uid == uid && sc2->sc_death_id != SC_DEATH_WINNER)
			break;
		}
		if (sc2 >= endp)
		    sc2 = endp - 1;
	    }
	    else
		sc2 = endp - 1;
	    while (sc2 > scp)
	    {
		*sc2 = sc2[-1];
		sc2--;
	    }
	    scp->sc_score = amount;
	    jrogue_strlcpy(scp->sc_name, whoami, sizeof scp->sc_name);
	    scp->sc_death_id = flags;
	    if (flags == 2)
		scp->sc_level = max_level;
	    else
		scp->sc_level = level;
	    scp->sc_killer_id = monst;
	    scp->sc_uid = uid;
	    sc2 = scp;
	}
    }
    /* A game-over score is committed before displaying the list.  Never
     * keep a lock during the Return-key or color-screen wait. */
    if (score_locked)
    {
        if (sc2 != NULL && !wr_score(top_ten))
            fprintf(stderr, "%s\n",
                    JSTR("スコアを書き込めませんでした。",
                         "Could not save the score."));
        unlock_sc();
        score_locked = FALSE;
    }
#ifdef MASTER
    if (prflags == 2)
        wizard_deleted = calloc((size_t)numscores, sizeof *wizard_deleted);
#endif
    /*
     * Use the same formatter for every display path.  Color during normal
     * gameplay adds only a separate viewing pass; -s and wizard editing
     * stay on the ordinary screen, with no duplicate display or edits.
     */
#ifdef MASTER
    if (color_mode == TRUE && flags != -1 && prflags != 2)
#else
    if (color_mode == TRUE && flags != -1)
#endif
        score_show_colored_then_record(top_ten, endp, sc2, flags, prflags, reason);
    else
    {
        /* Do not clear for -s: it did not draw a curses game screen. */
        score_prepare_screen(flags != -1, FALSE);
        score_print_list(top_ten, endp, sc2, flags, prflags, reason,
                         FALSE,
#ifdef MASTER
                         wizard_deleted, &wizard_deleted_count
#else
                         NULL, NULL
#endif
                         );
    }
#ifdef MASTER
    if (wizard_deleted_count > 0 && wizard_deleted != NULL &&
        scoreboard != NULL && lock_sc())
    {
        SCORE *latest = calloc((size_t)numscores, sizeof *latest);
        if (latest != NULL)
        {
            /* A score may have changed while wizard edited the display.
             * Only remove rows that still match the originally shown row. */
            rd_score(latest);
            if (score_apply_deletions(latest, wizard_deleted,
                                      wizard_deleted_count) > 0 &&
                !wr_score(latest))
                fprintf(stderr, "%s\n",
                        JSTR("スコアの編集内容を保存できませんでした。",
                             "Could not save wizard score edits."));
            free(latest);
        }
        unlock_sc();
    }
    free(wizard_deleted);
#endif
    free(top_ten);
}


static void
grave_bounds(int row, int *left, int *right)
{
    /* Screen rows 9..13 are the sloping top of the tombstone.
     * The usable text area changes on each line, so using the same
     * column for every line makes English text look shifted left.
     */
    switch (row) {
    case 9:  *left = 23; *right = 32; break;
    case 10: *left = 22; *right = 33; break;
    case 11: *left = 21; *right = 34; break;
    case 12: *left = 20; *right = 35; break;
    case 13: *left = 19; *right = 36; break;
    default: *left = 19; *right = 36; break;
    }
}

static void
grave_text(int row, const char *text)
{
    int left, right, width, len, col, i;
    char shown[MAXSTR + 4];

    grave_bounds(row, &left, &right);
    width = right - left + 1;
    /* The stone has only 18 text cells: even a normal 32-cell name must
     * be abbreviated here.  The same protection covers long killer names. */
    shown[0] = '\0';
    jrogue_text_append_labeled(shown, sizeof shown, width, "", text, "");
    len = jrogue_text_width(shown);
    col = left + (width - len) / 2;
    if (col < left)
        col = left;

    move(row, left);
    for (i = 0; i < width; i++)
        addch(' ');
    mvaddstr(row, col, shown);
}

/*
 * death:
 *	Do something really fun when he dies
 */

void
death(int monst)
{
    const char **dp;
    const char *killer;
    struct tm *lt;
    time_t date;

    signal(SIGINT, SIG_IGN);
    purse -= purse / 10;
    signal(SIGINT, leave);
    clear();
    killer = killname(monst, FALSE);
    if (!tombstone)
    {
	if (jrogue_lang_is_japanese)
	{
	    /* Match the original death message position (second-last row). */
	    move(_LINES - 2, 0);
	    printw("%s", killer);
	    if (isupper(monst))
		printw("と戦いて");
	    else
		printw("により");
	    printw("死す。 ＄%d", purse);
	}
	else
	{
	    mvprintw(_LINES - 2, 0, "Killed by ");
	    if (monst != 's' && monst != 'h')
		printw("a%s ", vowelstr(killer));
	    printw("%s with %d gold", killer, purse);
	}
    }
    else
    {
	time(&date);
	lt = localtime(&date);
	move(8, 0);
	dp = rip;
	if (color_mode == TRUE)
	    attrset(COLOR_PAIR(43) | A_NORMAL);
	while (*dp)
	    addstr(*dp++);	
	if (color_mode == TRUE){
	    jrogue_set_normal_attr();
if (jrogue_lang_is_japanese)
        {
            grave_text(11, "安らかに");
            grave_text(12, "眠れ");
            grave_text(17, "により死す");
        }
        else
        {
            grave_text(10, "REST");
            grave_text(11, "IN");
            grave_text(12, "PEACE");
        }
        attrset(COLOR_PAIR(3) | A_NORMAL);
        mvaddstr(19, 17, "*");
        mvaddstr(19, 24, "*");
        mvaddstr(19, 27, "*");
        mvaddstr(19, 30, "*");
        mvaddstr(19, 39, "*");
        attrset(COLOR_PAIR(2) | A_NORMAL);
        mvaddstr(20, 9, "________)/\\\\_//(\\/(/\\)/\\//\\/");
        mvaddstr(20, 38, "_)_______");
        jrogue_set_normal_attr();
        }
    if (color_mode == TRUE)
	    jrogue_set_normal_attr();
if (jrogue_lang_is_japanese)
        grave_text(16, killer);
    else
        {
            /* Use the original vowelstr() rule for English articles.
             * Monsters and missiles need a/an; starvation, hypothermia,
             * and the proper name Wally the Wonder Badger do not. */
            const int has_article = isupper((unsigned char)monst) ||
                                    monst == 'a' || monst == 'b' || monst == 'd';
            const char *deathline = !has_article ? "killed by" :
                                    *vowelstr(killer) ? "killed by an" :
                                                        "killed by a";
            grave_text(16, deathline);
            grave_text(17, killer);
        }
//	if (monst == 's' || monst == 'h')
//	    jrogue_mvaddch(16, 32, ' ');
//	else
//	    mvaddstr(16, 33, vowelstr(killer));
	grave_text(14, whoami);
	snprintf(prbuf, PRBUF_SIZE, JSTR("＄%d", "%d Au"), purse);
	grave_text(15, prbuf);
	snprintf(prbuf, PRBUF_SIZE, "%4d", 1900+lt->tm_year);
	grave_text(18, prbuf);
if (jrogue_lang_is_japanese && isupper(monst))
	    grave_text(17, "と戦いて死す");
    }
    move(_LINES - 1, 0);
    refresh();
    score(purse, amulet ? SC_DEATH_AMULET : SC_DEATH_KILLED, monst);
    if (!jrogue_score_waited_for_exit)
    {
        printf("%s", JSTR("［リターンを押してください］", "[Press return to exit]\n"));
        fflush(stdout);
        (void) fgets(prbuf,10,stdin);
    }
    my_exit(0);
}

/*
 * center:
 *	Return the index to center the given string
 */
int
center(const char *str)
{
    return 28 - (((int)mbstrlen(str) + 1) / 2);
}

/*
 * total_winner:
 *	Code for a winner
 */

void
total_winner(void)
{
    THING *obj;
    struct obj_info *op;
    int worth = 0;
    int oldpurse;

    clear();
    jrogue_standout();
    if (color_mode == TRUE)
	    attrset(COLOR_PAIR(JROGUE_PAIR_WIN_GREEN) | A_NORMAL);
    addstr("                                                               \n");
    addstr("  @   @               @   @           @          @@@  @     @  \n");
    addstr("  @   @               @@ @@           @           @   @     @  \n");
    addstr("  @   @  @@@  @   @   @ @ @  @@@   @@@@  @@@      @  @@@    @  \n");
    addstr("   @@@@ @   @ @   @   @   @     @ @   @ @   @     @   @     @  \n");
    addstr("      @ @   @ @   @   @   @  @@@@ @   @ @@@@@     @   @     @  \n");
    addstr("  @   @ @   @ @  @@   @   @ @   @ @   @ @         @   @  @     \n");
    addstr("   @@@   @@@   @@ @   @   @  @@@@  @@@@  @@@     @@@   @@   @  \n");
    addstr("                                                               \n");
    if (color_mode == TRUE)
	    attrset(COLOR_PAIR(JROGUE_PAIR_WIN_YELLOW) | A_NORMAL);
    addstr(JSTR("     おめでとう！  君はようやく太陽の光のもとにたどりついた。  \n", "     Congratulations, you have made it to the light of day!    \n"));
    jrogue_standend();
    if (color_mode == TRUE)
	    attrset(COLOR_PAIR(JROGUE_PAIR_WIN_YELLOW) | A_NORMAL);
    addstr(JSTR("\n     そして、運命の洞窟から生きて戻った精鋭の一人となった。    \n", "\nYou have joined the elite ranks of those who have escaped the\n"));
    addstr(JSTR("     君は故郷へ帰り、すべての戦利品を売って大金を手に入れ、    \n", "Dungeons of Doom alive.  You journey home and sell all your loot at\n"));
    addstr(JSTR("     戦士ギルドに迎え入れられたということである・・・。        \n", "a great profit and are admitted to the Fighters' Guild.\n"));
    jrogue_set_normal_attr();
    mvaddstr(_LINES - 1, 0, JSTR("--スペースを押してください--", "--Press space to continue--"));
    refresh();
    jrogue_show_cursor(TRUE);
    wait_for(stdscr, ' ');
    jrogue_restore_cursor();
    clear();
    mvaddstr(0, 0, JSTR("    価値  持ちもの\n", "   Worth  Item\n"));
    oldpurse = purse;
    for (obj = pack; obj != NULL; obj = next(obj))
    {
	switch (obj->o_type)
	{
	    case FOOD:
		worth = jrogue_sat_mul(2, obj->o_count);
	    when WEAPON:
		worth = weap_info[obj->o_which].oi_worth;
		worth = jrogue_sat_mul(worth,
            jrogue_sat_add(jrogue_sat_mul(3,
                jrogue_sat_add(obj->o_hplus, obj->o_dplus)), obj->o_count));
		obj->o_flags |= ISKNOW;
	    when ARMOR:
		worth = arm_info[obj->o_which].oi_worth;
		worth = jrogue_sat_add(worth,
            jrogue_sat_mul(jrogue_sat_sub(9, obj->o_arm), 100));
		worth = jrogue_sat_add(worth,
            jrogue_sat_mul(10, jrogue_sat_sub(a_class[obj->o_which], obj->o_arm)));
		obj->o_flags |= ISKNOW;
	    when SCROLL:
		worth = scr_info[obj->o_which].oi_worth;
		worth = jrogue_sat_mul(worth, obj->o_count);
		op = &scr_info[obj->o_which];
		if (!op->oi_know)
		    worth /= 2;
		op->oi_know = TRUE;
	    when POTION:
		worth = pot_info[obj->o_which].oi_worth;
		worth = jrogue_sat_mul(worth, obj->o_count);
		op = &pot_info[obj->o_which];
		if (!op->oi_know)
		    worth /= 2;
		op->oi_know = TRUE;
	    when RING:
		op = &ring_info[obj->o_which];
		worth = op->oi_worth;
		if (obj->o_which == R_ADDSTR || obj->o_which == R_ADDDAM ||
		    obj->o_which == R_PROTECT || obj->o_which == R_ADDHIT)
		{
			if (obj->o_arm > 0)
			    worth = jrogue_sat_add(worth, jrogue_sat_mul(obj->o_arm, 100));
			else
			    worth = 10;
		}
		if (!(obj->o_flags & ISKNOW))
		    worth /= 2;
		obj->o_flags |= ISKNOW;
		op->oi_know = TRUE;
	    when STICK:
		op = &ws_info[obj->o_which];
		worth = op->oi_worth;
		worth = jrogue_sat_add(worth, jrogue_sat_mul(20, obj->o_charges));
		if (!(obj->o_flags & ISKNOW))
		    worth /= 2;
		obj->o_flags |= ISKNOW;
		op->oi_know = TRUE;
	    when AMULET:
		worth = 1000;
	}
	if (worth < 0)
	    worth = 0;
        {
            char prefix[48];
            int name_cells;

            /* Reserve the inventory letter, the FULL value (which can
             * exceed five digits), and the two spaces before the item.
             * Leave the final column empty to avoid terminal auto-wrap. */
            snprintf(prefix, sizeof prefix, "%c) %5d  ", obj->o_packch, worth);
            name_cells = NUMCOLS - 1 - jrogue_text_width(prefix);
            printw("%s%s\n", prefix, inv_name_limited(obj, FALSE, name_cells));
        }
	purse = jrogue_sat_money_add(purse, worth);
    }
    printw(JSTR("   %5d  金塊          ", "   %5d  Gold Pieces          "), oldpurse);
    refresh();
    score(purse, SC_DEATH_WINNER, ' ');
    if (!jrogue_score_waited_for_exit)
    {
        printf("%s", JSTR("［リターンを押してください］", "[Press return to exit]\n"));
        fflush(stdout);
        (void) fgets(prbuf,10,stdin);
    }
    my_exit(0);
}

/*
 * killname:
 *	Convert a code to a monster name
 */
const char *
killname(int monst, int doart)
{
    struct h_list *hp;
    const char *sp;
    int article;
    struct h_list nlist[] = {
	{'a',	JSTR("矢", "arrow"),		TRUE},
	{'b',	JSTR("ボルト", "bolt"),			TRUE},
	{'d',	JSTR("毒投げ矢", "dart"),			TRUE},
	{'h',	JSTR("寒さ", "hypothermia"),		FALSE},
	{'s',	JSTR("飢え", "starvation"),		FALSE},
	{'\0'}
    };

    if (isupper(monst))
    {
	sp = monsters[monst-'A'].m_name;
	article = TRUE;
    }
    else
    {
	sp = JSTR("驚くべきアナグマのウォリー", "Wally the Wonder Badger");
	article = FALSE;
	for (hp = nlist; hp->h_ch; hp++)
	    if (hp->h_ch == monst)
	    {
		sp = hp->h_desc;
		article = hp->h_print;
		break;
	    }
    }
    if (doart && article)
	snprintf(prbuf, PRBUF_SIZE, "a%s ", vowelstr(sp));
    else
	prbuf[0] = '\0';
    jrogue_strlcat(prbuf, sp, PRBUF_SIZE);
    return prbuf;
}

/*
 * death_monst:
 *	Return a monster appropriate for a random death.
 */
int
death_monst(void)
{
    int poss[] =
    {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
	'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'h', 'd', 's',
	' '	/* This is provided to generate the "Wally the Wonder Badger"
		   message for killer */
    };

    return poss[rnd(sizeof poss / sizeof (int))];
}

