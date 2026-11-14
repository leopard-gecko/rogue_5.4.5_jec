/*
 * Various installation dependent routines
 *
 * @(#)mach_dep.c	4.37 (Berkeley) 05/23/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

/*
 * The various tuneable defines are:
 *
 *	SCOREFILE	Where/if the score file should live.
 *	ALLSCORES	Score file is top ten scores, not top ten
 *			players.  This is only useful when only a few
 *			people will be playing; otherwise the score file
 *			gets hogged by just a few people.
 *	NUMSCORES	Number of scores in the score file (default 10).
 *	NUMNAME		String version of NUMSCORES (first character
 *			should be capitalized) (default "Ten").
 *	MAXLOAD		What (if any) the maximum load average should be
 *			when people are playing.  Since it is divided
 *			by 10, to specify a load limit of 4.0, MAXLOAD
 *			should be "40".	 If defined, then
 *      LOADAV		Should it use it's own routine to get
 *		        the load average?
 *      NAMELIST	If so, where does the system namelist
 *		        hide?
 *	MAXUSERS	What (if any) the maximum user count should be
 *	                when people are playing.  If defined, then
 *      UCOUNT		Should it use it's own routine to count
 *		        users?
 *      UTMP		If so, where does the user list hide?
 *	CHECKTIME	How often/if it should check during the game
 *			for high load average.
 */

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <curses.h>
#include "extern.h"

#define NOOP(x) (x += 0)

# ifndef NUMSCORES
#	define	NUMSCORES	10
#	define	NUMNAME		JSTR("テン", "Ten")
# endif

int numscores = NUMSCORES;
const char *Numname = "Ten";

#ifdef ALLSCORES
int allscore = TRUE;
#else  /* ALLSCORES */
int allscore = FALSE;
#endif /* ALLSCORES */

#ifdef CHECKTIME
static int num_checks = 0;		/* times we've gone over in checkout() */
#endif /* CHECKTIME */

/*
 * init_check:
 *	Check out too see if it is proper to play the game now
 */

void
init_check(void)
{
#if defined(MAXLOAD) || defined(MAXUSERS)
    if (too_much())
    {
        char display_name[MAXSTR];
        jrogue_player_name_display(display_name, sizeof display_name, whoami,
                                   JROGUE_PLAYER_NAME_MAX_CELLS);
	printf("Sorry, %s, but the system is too loaded now.\n", display_name);
	printf("Try again later.  Meanwhile, why not enjoy a%s %s?\n",
	    vowelstr(fruit), jrogue_fruit_display());
	if (author())
	    printf("However, since you're a good guy, it's up to you\n");
	else
	    exit(1);
    }
#endif
}

/*
 * open_score:
 *	Open up the score file for future use
 */

void
open_score(void)
{
#ifdef SCOREFILE
    const char *scorefile = SCOREFILE;
    int created = 0;

    /* SCOREFILE/FILEDIR is the single source of the score file location.
     * Exclusive creation prevents simultaneous first runs from truncating
     * a score file another process has just created. */
    if (scoreboard != NULL)
    {
        rewind(scoreboard);
        return;
    }

    scoreboard = md_score_open(scorefile, &created);
    if (scoreboard != NULL && created)
        md_chmod(scorefile, 0664);
    if (scoreboard == NULL)
    {
        fprintf(stderr, "Could not open %s for writing: %s\n",
                scorefile, strerror(errno));
        fflush(stderr);
    }
#else
    scoreboard = NULL;
#endif
}

/*
 * getltchars:
 *	Get the local tty chars for later use
 */

void
getltchars(void)
{
    got_ltc = TRUE;
    orig_dsusp = md_dsuspchar();
    md_setdsuspchar( md_suspchar() );
}

/*
 * setup:
 *	Get starting setup for all games
 */

void
setup(void)
{
#ifdef DUMP
    md_onsignal_autosave();
#else
    md_onsignal_default();
#endif

#ifdef CHECKTIME
    md_start_checkout_timer(CHECKTIME*60);
    num_checks = 0;
#endif

    raw();				/* Raw mode */
    noecho();				/* Echo off */
    keypad(stdscr,1);
    getltchars();			/* get the local tty chars */
}

/* 
 * resetltchars: 
 *      Reset the local tty chars to original values. 
 */ 
void 
resetltchars(void) 
{ 
    if (got_ltc) {
        md_setdsuspchar(orig_dsusp);
    } 
} 
  
/* 
 * playltchars: 
 *      Set local tty chars to the values we use when playing. 
 */ 
void 
playltchars(void) 
{ 
    if (got_ltc) { 
        md_setdsuspchar( md_suspchar() );
    } 
} 

/*
 * start_score:
 *	Start the scoring sequence
 */

void
start_score(void)
{
#ifdef CHECKTIME
    md_stop_checkout_timer();
#endif
}

/* 	 	 
 * is_symlink: 	 	 
 *      See if the file has a symbolic link 	 	 
  */ 	 	 
int	 	 
is_symlink(char *sp) 	 	 
{ 	 	 
#ifdef S_IFLNK 	 	 
    struct stat sbuf2; 	 	 
 	 	 
    if (lstat(sp, &sbuf2) < 0) 	 	 
        return FALSE; 	 	 
    else 	 	 
        return ((sbuf2.st_mode & S_IFMT) != S_IFREG); 	 	 
#else
    (void)sp;
    return FALSE; 	 	 
#endif 
} 

#if defined(MAXLOAD) || defined(MAXUSERS)
/*
 * too_much:
 *	See if the system is being used too much for this game
 */
int
too_much(void)
{
#ifdef MAXLOAD
    double avec[3];
#else
    int cnt;
#endif

#ifdef MAXLOAD
    md_loadav(avec);
    if (avec[1] > (MAXLOAD / 10.0))
	return TRUE;
#endif
#ifdef MAXUSERS
    if (ucount() > MAXUSERS)
	return TRUE;
#endif
    return FALSE;
}

/*
 * author:
 *	See if a user is an author of the program
 */
int
author(void)
{
#ifdef MASTER
    if (wizard)
	return TRUE;
#endif
    switch (md_getuid())
    {
	case -1:
	    return TRUE;
	default:
	    return FALSE;
    }
}
#endif

#ifdef CHECKTIME
/*
 * checkout:
 *	Check each CHECKTIME seconds to see if the load is too high
 */

checkout(int sig)
{
    char *msgs[] = {
	"The load is too high to be playing.  Please leave in %0.1f minutes",
	"Please save your game.  You have %0.1f minutes",
	"Last warning.  You have %0.1f minutes to leave",
    };
    int checktime;

    if (too_much())
    {
	if (author())
	{
	    num_checks = 1;
	    chmsg("The load is rather high, O exaulted one");
	}
	else if (num_checks++ == 3)
	    fatal("Sorry.  You took too long.  You are dead\n");
	checktime = (CHECKTIME * 60) / num_checks;
	chmsg(msgs[num_checks - 1], ((double) checktime / 60.0));
    }
    else
    {
	if (num_checks)
	{
	    num_checks = 0;
	    chmsg("The load has dropped back down.  You have a reprieve");
	}
	checktime = (CHECKTIME * 60);
    }

	md_start_checkout_timer(checktime);
}

/*
 * chmsg:
 *	checkout()'s version of msg.  If we are in the middle of a
 *	shell, do a printf instead of a msg to a the refresh.
 */
/* VARARGS1 */

chmsg(char *fmt, int arg)
{
    if (!in_shell)
	msg(fmt, arg);
    else
    {
	printf(fmt, arg);
	putchar('\n');
	fflush(stdout);
    }
}
#endif

#ifdef UCOUNT
/*
 * ucount:
 *	count number of users on the system
 */
#include <utmp.h>

struct utmp buf;

int
ucount(void)
{
    struct utmp *up;
    FILE *utmp;
    int count;

    if ((utmp = fopen(UTMP, "r")) == NULL)
	return 0;

    up = &buf;
    count = 0;

    while (fread(up, 1, sizeof (*up), utmp) > 0)
	if (buf.ut_name[0] != '\0')
	    count++;
    fclose(utmp);
    return count;
}
#endif

/*
 * Lock the open scoreboard itself rather than a separate .lck file.
 * The OS lock covers score reading, ranking and writing, but no user input.
 * The scoreboard FILE* remains open after unlock_sc().
 */
int
lock_sc(void)
{
#ifdef SCOREFILE
    int saved_errno;

    if (md_score_lock_acquire(scoreboard, 5000))
    {
        /* A previous read may still be buffered while another process has
         * since updated the score file.  SEEK_END forces a real seek and
         * invalidates that stale buffer before the new locked read. */
        if (fseek(scoreboard, 0L, SEEK_END) == 0 &&
            fseek(scoreboard, 0L, SEEK_SET) == 0)
            return TRUE;
        saved_errno = errno ? errno : EIO;
        md_score_lock_release();
        errno = saved_errno;
    }
    fprintf(stderr, "%s: %s (%s)\n",
            JSTR("スコアのロックを取得できませんでした",
                 "Could not lock score file"),
            SCOREFILE, strerror(errno));
    fflush(stderr);
    return FALSE;
#else
    return TRUE;
#endif
}

void
unlock_sc(void)
{
#ifdef SCOREFILE
    md_score_lock_release();
#endif
}

/*
 * flush_type:
 *	Flush typeahead for traps, etc.
 */

void
flush_type(void)
{
    flushinp();
}
