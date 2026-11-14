/*
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 *
 * @(#)main.c	4.22 (Berkeley) 02/05/99
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#ifdef _WIN32
/* MinGW conio.h declares getch(void), while PDCurses defines getch()
 * as a macro.  Include conio.h before curses.h to avoid a collision. */
#include <conio.h>
#endif
#include <curses.h>
#include <locale.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "rogue.h"
#include "jec/unicode_text.h"
#ifdef _WIN32
#include "windows/rogue_windows.h"
#include "windows/rogue_windows_console.h"
#endif

/*
 * main:
 *	The main program, of course
 */
int
main(int argc, char **argv)
{
    char *env;
    int had_rogueopts;
    char display_name[MAXSTR];
    char welcome_suffix[256];
    const char *welcome_prefix;
    time_t lowtime;
#ifdef _WIN32
    char *windows_save_path = NULL;
    int windows_argument_index = 1;
#endif

    jrogue_init_locale();
    jrogue_detect_language();
    jrogue_apply_language();
#ifdef _WIN32
    jrogue_enable_vt_truecolor();
#endif
#ifdef __CYGWIN__
    setenv("TERMINFO", "terminfo", 1);
#endif

    md_init();
#ifdef _WIN32
    jrogue_windows_resize_direct_launch();
#endif

#ifdef MASTER
    /*
     * Check to see if he is a wizard
     */
    if (argc >= 2 && argv[1][0] == '\0')
	if (strcmp(PASSWD, md_crypt(md_getpass(JSTR("魔法使いの合言葉は？", "wizard's password: ")), "mT")) == 0)
	{
	    wizard = TRUE;
	    player.t_flags |= SEEMONST;
	    argv++;
	    argc--;
#ifdef _WIN32
            windows_argument_index++;
#endif
	}

#endif

    /*
     * get home and options from environment
     */

#ifdef __CYGWIN__
    jrogue_strlcpy(file_name, DEFAULT_SAVEFILE, MAXSTR);
#else
    jrogue_strlcpy(home, md_gethomedir(), MAXSTR);

	if (strlen(home) > MAXSTR - strlen(DEFAULT_SAVEFILE) - 1)
		*home = 0;
    jrogue_strlcpy(file_name, DEFAULT_SAVEFILE, MAXSTR);

#endif

#ifdef _WIN32
    env = jrogue_rogueopts_utf8();
#else
    env = getenv("ROGUEOPTS");
#endif
    had_rogueopts = (env != NULL);
    if (env != NULL)
    {
        parse_opts(env);
#ifdef _WIN32
        free(env);
#endif
    }
    jrogue_apply_language();
    if (!had_rogueopts || whoami[0] == '\0')
    {
        const char *username_utf8 = md_getusername();
        strucpy_name(whoami, username_utf8, strlen(username_utf8));
    }
    lowtime = time(NULL);
    /* RRP 5.4.5: SEED works in ordinary games as well as wizard games.
     * A fixed dungeon seed must also disable score registration. */
    if (getenv("SEED") != NULL)
    {
	dnum = atoi(getenv("SEED"));
	noscore = TRUE;
    }
    else
	dnum = (unsigned int) lowtime + md_getpid();
    seed = dnum;

    open_score();

    /* Drop setuid/setgid privileges after opening the scoreboard file. */
    md_normaluser();

    /*
     * check for print-score option
     */

    if (argc == 2)
    {
	if (strcmp(argv[1], "-s") == 0)
	{
	    noscore = TRUE;
	    score(0, -1, 0);
	    exit(0);
	}
	else if (strcmp(argv[1], "-d") == 0)
	{
	    dnum = rnd(100);	/* throw away some rnd()s to break patterns */
	    while (--dnum)
		rnd(100);
	    purse = rnd(100) + 1;
	    level = rnd(100) + 1;
	    initscr();
#ifdef _WIN32
    jrogue_windows_retry_direct_launch_resize(0);
#endif
	    start_custom_color();
	    getltchars();
            /* -d uses the same minimum and capped logical screen size as
             * a normal game.  A larger terminal must not change the
             * position of its last two lines. */
            if (LINES < NUMLINES || COLS < NUMCOLS)
            {
                endwin();
                printf(JSTR("\nごめん！ 画面の大きさは最低 %dx%d なんだ。\n",
                            "\nSorry, the screen must be at least %dx%d.\n"),
                       NUMLINES, NUMCOLS);
                my_exit(1);
            }
            _COLS = COLS > MAXCOLS ? MAXCOLS : COLS;
            _LINES = LINES > MAXLINES ? MAXLINES : LINES;
	    death(death_monst());
	    exit(0);
	}
    }
#ifdef _WIN32
    /* Only direct Explorer-style launches with no explicit argument offer
     * the start menu; shell invocations and explicit -r/-s/-d bypass it.
     * A dropped filename is read as UTF-16, not from the ANSI argv[] view. */
    if (argc == 1)
        windows_save_path = jrogue_windows_launch_save(file_name);
    else if (argc == 2 && argv[1][0] != '-')
        windows_save_path = jrogue_windows_argument_utf8(windows_argument_index);
#endif
    init_check();			/* check for legal startup */
#ifdef _WIN32
    if (windows_save_path != NULL)
    {
        if (strlen(windows_save_path) >= MAXSTR)
        {
            fputs(JSTR("セーブファイルのパスが長すぎます。\n",
                      "Save file path is too long.\n"), stderr);
            free(windows_save_path);
            my_exit(1);
        }
        if (!restore(windows_save_path))
            my_exit(1);
    }
    else
#endif
    if (argc == 2)
    {
        if (!restore(argv[1])) /* Note: restore will never return */
            my_exit(1);
    }
    jrogue_reset_screen();
    /* Reserve the welcome text first: 32 cells of name alone can still
     * exceed the screen when followed by a long Japanese sentence.  Old
     * names are clipped ONLY in the local display copy. */
    welcome_prefix = JSTR("やあ、", "Hello ");
#ifdef MASTER
    if (wizard)
        snprintf(welcome_suffix, sizeof welcome_suffix,
                 JSTR("。#%dのダンジョンへようこそ・・・",
                      ", welcome to dungeon #%d\n"), dnum);
    else
#endif
        snprintf(welcome_suffix, sizeof welcome_suffix, "%s",
                 JSTR("。 ダンジョンを掘っている間、ちょっと待っててね...",
                      ", just a moment while I dig the dungeon...\n"));
    jrogue_player_name_display(display_name, sizeof display_name, whoami,
                               NUMCOLS - 1 - jrogue_text_width(welcome_prefix)
                               - jrogue_text_width(welcome_suffix));
    printf("%s%s%s", welcome_prefix, display_name, welcome_suffix);
    if (idscr_md == TRUE)
	apply_idscroll_initial_probabilities();
    fflush(stdout);
    md_sleep(1);
    jrogue_reset_screen();

    initscr();				/* Start up cursor package */
#ifdef _WIN32
    jrogue_windows_retry_direct_launch_resize(0);
#endif
    start_custom_color();
    init_probs();			/* Set up prob tables for objects */
    init_player();			/* Set up initial player stats */
    init_names();			/* Set up names of scrolls */
    init_colors();			/* Set up colors of potions */
    init_stones();			/* Set up stone settings of rings */
    init_materials();			/* Set up materials of wands */
    setup();

    /*
     * The screen must be at least NUMLINES x NUMCOLS
     */
    if (LINES < NUMLINES || COLS < NUMCOLS)
    {
	endwin();
	printf(JSTR("\nごめん！ 画面の大きさは最低 %dx%d なんだ。\n", "\nSorry, the screen must be at least %dx%d.\n"), NUMLINES, NUMCOLS);
	my_exit(1);
    }

    if (COLS > MAXCOLS)
        _COLS = MAXCOLS;
    else
        _COLS  = COLS;
    if (LINES > MAXLINES)
        _LINES = MAXLINES;
    else
        _LINES = LINES;

    /*
     * Set up windows
     */
    hw = newwin(_LINES, _COLS, 0, 0);
    jrogue_wset_normal_attr(hw);
    if (color_mode)
    {
	bkgd(' ' | jrogue_normal_attr());
	attrset(jrogue_normal_attr());
	erase();
	refresh();
	wbkgd(hw, ' ' | jrogue_normal_attr());
	wattrset(hw, jrogue_normal_attr());
    }
    idlok(stdscr, TRUE);
    idlok(hw, TRUE);
    jrogue_restore_cursor();
#ifdef MASTER
    /* Do not undo the score suppression requested by SEED. */
    if (wizard)
        noscore = TRUE;
#endif
    new_level();			/* Draw current level */
    /*
     * Start up daemons and fuses
     */
    start_daemon(runners, 0, AFTER);
    start_daemon(doctor, 0, AFTER);
    fuse(swander, 0, WANDERTIME, AFTER);
    start_daemon(stomach, 0, AFTER);
    playit();
    return(0);
}

/*
 * endit:
 *	Exit the program abnormally.
 */

void
endit(int sig)
{
    (void)sig;
    fatal(JSTR("それでは、またね。\n", "Okay, bye bye!\n"));
}

/*
 * fatal:
 *	Exit the program, printing a message.
 */

void
fatal(const char *s)
{
    mvaddstr(_LINES - 2, 0, s);
    refresh();
    endwin();
    my_exit(0);
}

/*
 * fatal_error:
 *     Report an internal failure after leaving curses, where the message
 *     remains in the terminal scrollback.  Do not route endit() through this:
 *     its original "Okay, bye bye!" message is not an internal error.
 *
 * Only call from ordinary game code, never from a signal handler.  In
 * particular, CHECKTIME's SIGALRM-driven policy exit stays with fatal().
 */
void
fatal_error(const char *s)
{
    /* These failures occur during a running game with curses initialized. */
    if (!isendwin())
        endwin();
    restore_terminal_background();
    resetltchars();

    if (s != NULL && *s != '\0')
    {
        fputs(s, stderr);
        if (s[strlen(s) - 1] != '\n')
            fputc('\n', stderr);
    }
    fflush(stderr);

#ifdef _WIN32
    /* The console opened by Explorer may close when the process exits.
     * Shell/terminal launches must exit immediately, without a new prompt. */
    if (jrogue_windows_is_direct_launch())
    {
        fputs(JSTR("［リターンを押すと終了します］",
                   "[Press Enter to exit]"), stderr);
        fflush(stderr);
        for (;;)
        {
            wint_t key = _getwch();
            if (key == '\r' || key == '\n' || key == WEOF)
                break;
            if (key == 0 || key == 0xe0)
                (void)_getwch();  /* Consume the rest of a special key. */
        }
        fputc('\n', stderr);
    }
#endif
    exit(EXIT_FAILURE);
}

/*
 * rnd:
 *	Pick a very random number.
 */
int
rnd(int range)
{
    return range == 0 ? 0 : abs((int) RN) % range;
}

/*
 * roll:
 *	Roll a number of dice
 */
int 
roll(int number, int sides)
{
    int dtotal = 0;

    while (number--)
	dtotal += rnd(sides)+1;
    return dtotal;
}

/*
 * tstp:
 *	Handle stop and start signals
 */

void
tstp(int ignored)
{
    int y, x;
    int oy, ox;

	(void)ignored;

    /*
     * leave nicely
     */
    getyx(curscr, oy, ox);
    mvcur(0, _COLS - 1, _LINES - 1, 0);
    endwin();
    resetltchars();
    fflush(stdout);
	md_tstpsignal();

    /*
     * start back up again
     */
	md_tstpresume();
    raw();
    noecho();
    keypad(stdscr,1);
    playltchars();
    clearok(curscr, TRUE);
    wrefresh(curscr);
    getyx(curscr, y, x);
    mvcur(y, x, oy, ox);
    fflush(stdout);
    /* Modern curses/ncurses (including macOS) keeps WINDOW opaque. */
    wmove(curscr, oy, ox);
}

/*
 * playit:
 *	The main loop of the program.  Loop until the game is over,
 *	refreshing things and looking at the proper times.
 */

void
playit(void)
{
    char *opts;
    char whoami2[MAXSTR];
    int saved_idscr_md;
    /*
     * set up defaults for slow terminals
     */

    if (baudrate() <= 1200)
    {
	terse = TRUE;
	jump = TRUE;
	see_floor = FALSE;
    }

#ifndef MODERN_ROGUE_OPTIONS
    /* RRP 5.4.5 original default: select clear-style inventory when
     * the terminal supports clearing to the end of the line.  Modern
     * builds retain their existing default inventory presentation. */
    if (md_hasclreol())
	inv_type = INV_CLEAR;
#endif

    /*
     * parse environment declaration of options
     */
    jrogue_strlcpy(whoami2, whoami, sizeof whoami2);
    saved_idscr_md = idscr_md;
#ifdef _WIN32
    opts = jrogue_rogueopts_utf8();
#else
    opts = getenv("ROGUEOPTS");
#endif
    if (opts != NULL)
    {
        parse_opts(opts);
#ifdef _WIN32
        free(opts);
#endif
    }
    idscr_md = saved_idscr_md;
    jrogue_apply_language();
    jrogue_strlcpy(whoami, whoami2, MAXSTR);


    oldpos = hero;
    oldrp = roomin(&hero);
    while (playing)
	command();			/* Command execution */
    endit(0);
}

/*
 * quit:
 *	Have player make certain, then exit.
 */

void
quit(int sig)
{
    int oy, ox;
    int ch;

    (void)sig;

    /*
     * Reset the signal in case we got here via an interrupt
     */
    if (!q_comm)
	mpos = 0;
    getyx(curscr, oy, ox);
    msg(JSTR("本当に終了してよいのですか？ y/n ", "really quit?"));
    jrogue_show_cursor(TRUE);
    ch = readchar();
    jrogue_restore_cursor();
    if (ch == 'y')
    {
	signal(SIGINT, leave);
	clear();
	mvprintw(_LINES - 2, 0, JSTR("所持金は＄%d", "You quit with %d gold pieces"), purse);
	move(_LINES - 1, 0);
	refresh();
	score(purse, 1, 0);
    if (!jrogue_score_waited_for_exit)
    {
        printf("%s", JSTR("［リターンを押してください］", "[Press return to exit]\n"));
        fflush(stdout);
        (void) fgets(prbuf,10,stdin);
    }
	my_exit(0);
    }
    else
    {
	move(0, 0);
	clrtoeol();
	status();
	move(oy, ox);
	refresh();
	mpos = 0;
	count = 0;
	to_death = FALSE;
    }
}

/*
 * leave:
 *	Leave quickly, but curteously
 */

void
leave(int sig)
{
    static char buf[BUFSIZ];

    (void)sig;

    setbuf(stdout, buf);	/* throw away pending output */

    if (!isendwin())
    {
	mvcur(0, _COLS - 1, _LINES - 1, 0);
	endwin();
    }

    putchar('\n');
    my_exit(0);
}

/*
 * shell:
 *	Let them escape for a while
 */

void
shell(void)
{
    /*
     * Set the terminal back to original mode
     */
    move(_LINES-1, 0);
    refresh();
    endwin();
    /* Restore terminal colors for the external shell without changing
     * curses' saved game screen (including its status line). */
    jrogue_shell_suspend_colors();
    resetltchars();
    putchar('\n');
    in_shell = TRUE;
    after = FALSE;
    fflush(stdout);
    /*
     * Fork and do a shell
     */
    md_shellescape();

    noecho();
    raw();
    keypad(stdscr,1);
    playltchars();
    jrogue_shell_resume_colors();
    in_shell = FALSE;
    /* Repaint the unchanged curses screen, as in the original game. */
    clearok(stdscr, TRUE);
}

/*
 * my_exit:
 *	Leave the process properly
 */

void
my_exit(int st)
{
    restore_terminal_background();
    resetltchars();
    exit(st);
}

