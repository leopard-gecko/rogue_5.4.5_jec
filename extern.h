/*
 * Defines for things used in mach_dep.c
 *
 * @(#)extern.h	4.35 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */


#include "jec/language.h"

#ifdef HAVE_CONFIG_H
#ifdef PDCURSES
#undef HAVE_UNISTD_H
#undef HAVE_LIMITS_H
#undef HAVE_MEMORY_H
#undef HAVE_STRING_H
#endif
#include "config.h"
#elif defined(__DJGPP__)
#define HAVE_SYS_TYPES_H 1
#define HAVE_PROCESS_H 1
#define HAVE_PWD_H 1
#define HAVE_TERMIOS_H 1
#define HAVE_SETGID 1
#define HAVE_GETGID 1
#define HAVE_SETUID 1
#define HAVE_GETUID 1
#define HAVE_GETPASS 1
#define HAVE_SPAWNL 1
#define HAVE_ALARM 1
#define HAVE_ERASECHAR 1
#define HAVE_KILLCHAR 1
#elif defined(_WIN32)
#define HAVE_CURSES_H
#define HAVE_TERM_H
#define HAVE__SPAWNL
#define HAVE_SYS_TYPES_H
#define HAVE_PROCESS_H
#define HAVE_ERASECHAR 1
#define HAVE_KILLCHAR 1
#elif defined(__CYGWIN__)
#define HAVE_SYS_TYPES_H 1
#define HAVE_PWD_H 1
#define HAVE_PWD_H 1
#define HAVE_SYS_UTSNAME_H 1
#define HAVE_ARPA_INET_H 1
#define HAVE_UNISTD_H 1
#define HAVE_TERMIOS_H 1
#define HAVE_NCURSES_TERM_H 1
#define HAVE_ESCDELAY
#define HAVE_SETGID 1
#define HAVE_GETGID 1
#define HAVE_SETUID 1
#define HAVE_GETUID 1
#define HAVE_GETPASS 1
#define HAVE_GETPWUID 1
#define HAVE_WORKING_FORK 1
#define HAVE_ALARM 1
#define HAVE_SPAWNL 1
#define HAVE__SPAWNL 1
#define HAVE_ERASECHAR 1
#define HAVE_KILLCHAR 1
#else /* POSIX */
#define HAVE_SYS_TYPES_H 1
#define HAVE_PWD_H 1
#define HAVE_PWD_H 1
#define HAVE_SYS_UTSNAME_H 1
#define HAVE_ARPA_INET_H 1
#define HAVE_UNISTD_H 1
#define HAVE_TERMIOS_H 1
#define HAVE_TERM_H 1
#define HAVE_SETGID 1
#define HAVE_GETGID 1
#define HAVE_SETUID 1
#define HAVE_GETUID 1
#define HAVE_SETREUID 1
#define HAVE_SETREGID 1
#define HAVE_GETPASS 1
#define HAVE_GETPWUID 1
#define HAVE_WORKING_FORK 1
#define HAVE_ERASECHAR 1
#define HAVE_KILLCHAR 1
#ifndef _AIX
#define HAVE_GETLOADAVG 1
#endif
#define HAVE_ALARM 1
#endif

#ifdef __DJGPP__
#undef HAVE_GETPWUID /* DJGPP's limited version doesn't even work as documented */
#endif

/*
 * Windows (MinGW-w64) does not expose the POSIX uid_t type.  Avoid defining a
 * global uid_t alias that could conflict with the C runtime or other headers.
 * Keep the native POSIX type on macOS/Linux.
 */
#ifndef JROGUE_UID_TYPE_DEFINED
#define JROGUE_UID_TYPE_DEFINED 1
#ifdef _WIN32
typedef unsigned int jrogue_uid_t;
#else
#include <sys/types.h>
typedef uid_t jrogue_uid_t;
#endif
#endif

/*
 * Don't change the constants, since they are used for sizes in many
 * places in the program.
 */

#include <stdlib.h>

#undef SIGTSTP

#define MAXSTR		1024	/* maximum length of strings */
#define PRBUF_SIZE	(2*MAXSTR)	/* bytes in the global sprintf buffer */
size_t	jrogue_strlcpy(char *dst, const char *src, size_t dstsize);
size_t	jrogue_strlcat(char *dst, const char *src, size_t dstsize);
#define MAXLINES	32	/* maximum number of screen lines used */
#define MAXCOLS		80	/* maximum number of screen columns used */

#define RN		(((seed = seed*11109+13849) >> 16) & 0xffff)
#ifdef CTRL
#undef CTRL
#endif
#define CTRL(c)		(c & 037)

/*
 * Now all the global variables
 */

extern int got_ltc, in_shell;
extern int	wizard;
extern char	fruit[], prbuf[], whoami[];
extern int orig_dsusp;
extern FILE	*scoreboard;
extern int numscores;
extern const char *Numname;
extern int allscore;

extern int	idscr_md;
extern int	color_mode;
extern int	color_mode_applied;
extern int	show_cursor;
extern int	_COLS, _LINES;

#include "jec/color.h"

void	jrogue_show_cursor(int show);
void	jrogue_restore_cursor(void);

/*
 * Function types
 */

int	md_chmod(const char *filename, int mode);
/* Score file I/O and OS-managed locking; the high-level Rogue API is in mach_dep.c. */
FILE *md_score_open(const char *path, int *created);
int   md_score_lock_acquire(FILE *file, int timeout_ms);
void  md_score_lock_release(void);
char	*md_crypt(const char *key, const char *salt);
int	md_dsuspchar(void);
int	md_erasechar(void);
char	*md_gethomedir(void);
char	*md_getusername(void);
jrogue_uid_t	md_getuid(void);
char	*md_getpass(const char *prompt);
pid_t	md_getpid(void);
char	*md_getrealname(jrogue_uid_t uid);
void	md_init(void);
void jrogue_reset_screen(void);
int	md_killchar(void);
void	md_normaluser(void);
void	md_raw_standout(int use_color);
void	md_raw_standend(int use_color);
#ifdef _WIN32
void	md_raw_score_normal(int use_color);
void	md_raw_score_prepare(void);
void	md_raw_score_clear_screen(void);
int	md_raw_score_uses_vt(void);
void	md_raw_score_color(int index);
#endif
int	md_readchar(WINDOW *win);
int	md_setdsuspchar(int c);
int	md_shellescape(void);
void	md_sleep(int s);
int	md_suspchar(void);
int	md_hasclreol(void);
int	md_unlink(char *file);
int	md_unlink_open_file(const char *file, FILE *inf);
void md_tstpsignal(void);
void md_tstphold(void);
void md_tstpresume(void);
void md_ignoreallsignals(void);
void md_onsignal_autosave(void);
void md_onsignal_exit(void);
void md_onsignal_default(void);
int md_issymlink(char *sp);
extern char *xcrypt(const char *key, const char *setting);

