/* config.h for the Makefile-only macOS/Linux build. */
/* This file is maintained in the source tree.  ./configure writes config.mk only. */

/* Score file behavior. */
#define ALLSCORES 1
#define NUMNAME JSTR_STATIC("テン", "Ten")
#define NUMSCORES 10

/* Optional site policy features.  They are disabled by default. */
/* #undef CHECKTIME */
/* #undef MAXLOAD */
/* #undef MAXUSERS */
/* #undef LOADAV */
/* #undef NAMELIST */
/* #undef UCOUNT */
/* #undef UTMP */

/* Wizard mode is normally enabled from the Makefile by passing MASTER=1. */
/* #undef MASTER */
/* #undef PASSWD */

/* Default per-program file names.  Makefile command-line values override these. */
#ifndef DEFAULT_SAVEFILE
#define DEFAULT_SAVEFILE "rogue.save"
#endif

#ifndef SCOREFILE
#define SCOREFILE "rogue.scr"
#endif

#if !defined(_WIN32)
/* Platform features expected by the supported macOS/Linux build. */
#define HAVE_ALARM 1
#define HAVE_ARPA_INET_H 1
#define HAVE_CURSES_H 1
#define HAVE_ERASECHAR 1
#define HAVE_ESCDELAY 1
#define HAVE_GETGID 1
#define HAVE_GETLOADAVG 1
#define HAVE_GETPASS 1
#define HAVE_GETPWUID 1
#define HAVE_GETUID 1
#define HAVE_KILLCHAR 1
#define HAVE_PWD_H 1
#define HAVE_SETGID 1
#define HAVE_SETREGID 1
#define HAVE_SETREUID 1
#define HAVE_SETUID 1
#define HAVE_TERMIOS_H 1
#define HAVE_TERM_H 1
#define HAVE_UNISTD_H 1
#define HAVE_WORKING_FORK 1

#else /* Native Win32/PDCurses */
#define HAVE_CURSES_H 1
#define HAVE_PROCESS_H 1
#endif
