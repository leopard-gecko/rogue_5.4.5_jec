/*
    mdport.c - Machine Dependent Code

    Copyright (C) 2005-2008 Nicholas J. Kisseberth
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name(s) of the author(s) nor the names of other contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS ``AS IS'' AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <Windows.h>
#include <Lmcons.h>
#include <io.h>
#include <stdint.h>
#include <conio.h>
#include <wchar.h>
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4201)
#endif
#include <shlobj.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#include <Shlwapi.h>
#undef MOUSE_MOVED
#endif

#include <curses.h> /* AIX requires curses.h before term.h */
#include "rogue.h"
#ifdef _WIN32
#include "windows/rogue_windows.h"
#endif
/* macOS does not provide setresuid()/setresgid(); use the setreuid()/setregid() fallback. */
#if defined(__APPLE__) && defined(__MACH__)
# undef HAVE_SETRESUID
# undef HAVE_SETRESGID
#endif


#if defined(HAVE_SYS_TYPES)
#include <sys/types.h>
#endif

#if defined(HAVE_PROCESS_H)
#include <process.h>
#endif

#if defined(HAVE_PWD_H)
#include <pwd.h>
#endif

#if defined(HAVE_SYS_UTSNAME)
#include <sys/utsname.h>
#endif

#if defined(HAVE_ARPA_INET_H)
#include <arpa/inet.h> /* Solaris 2.8 required this for htonl() and ntohl() */
#endif

#if defined(HAVE_TERMIOS_H)
#include <termios.h>
#endif

#if defined(HAVE_UNISTD_H)
#ifndef __USE_GNU
#define __USE_GNU
#include <unistd.h>
#undef __USE_GNU
#else
#include <unistd.h>
#endif
#endif

#if defined(HAVE_TERM_H)
#include <term.h>
#elif defined(HAVE_NCURSES_TERM_H)
#include <ncurses/term.h>
#endif

#if defined(HAVE_WORKING_FORK)
#include <sys/wait.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdio.h>
#include <signal.h>
#ifndef _WIN32
#include <sys/file.h>
#include <sys/select.h>
#endif
#include "extern.h"


#if !defined(PATH_MAX) && defined(_MAX_PATH)
#define PATH_MAX _MAX_PATH
#endif

#if !defined(PATH_MAX) && defined(_PATH_MAX)
#define PATH_MAX _PATH_MAX
#endif

#define NOOP(x) (x += 0)

void
jrogue_reset_screen(void)
{
#ifdef _WIN32
    /* PDCurses owns the Windows screen.  Sending raw VT escapes to stdout
     * here may show up as literal text in a non-VT Windows console.  The
     * following initscr()/clear() will initialize the full screen. */
    return;
#else
    (void) system("reset");
#endif
}

void
md_init(void)
{
#if defined(__INTERIX)
    char *term;

    term = getenv("TERM");

    if (term == NULL)
        setenv("TERM","interix");
#elif defined(__DJGPP__)
    _fmode = _O_BINARY;
#elif defined(_WIN32)
    _fmode = _O_BINARY;
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    /* The Windows VT backend needs virtual terminal processing enabled. */
    {
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode;
        if (out != INVALID_HANDLE_VALUE && GetConsoleMode(out, &mode))
            (void) SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif

/* Restore the RRP 5.4.5 Esc/keypad ambiguity delay for local
 * ncurses terminals.  Honor an explicit ESCDELAY environment setting
 * (e.g. for remote terminals); do not alter PDCurses input on Windows.
 * md_init() runs before initscr(), as in the original implementation. */
#if !defined(_WIN32) && (defined(HAVE_ESCDELAY) || defined(NCURSES_VERSION))
    if (getenv("ESCDELAY") == NULL)
        ESCDELAY = 64;
#endif

#if defined(DUMP)
	md_onsignal_default();
#else
	md_onsignal_exit();
#endif
}

void
md_onsignal_default(void)
{
#ifdef SIGHUP
    signal(SIGHUP, SIG_DFL);
#endif
#ifdef SIGQUIT
    signal(SIGQUIT, SIG_DFL);
#endif
#ifdef SIGILL
    signal(SIGILL, SIG_DFL);
#endif
#ifdef SIGTRAP
    signal(SIGTRAP, SIG_DFL);
#endif
#ifdef SIGIOT
    signal(SIGIOT, SIG_DFL);
#endif
#ifdef SIGEMT
    signal(SIGEMT, SIG_DFL);
#endif
#ifdef SIGFPE
    signal(SIGFPE, SIG_DFL);
#endif
#ifdef SIGBUS
    signal(SIGBUS, SIG_DFL);
#endif
#ifdef SIGSEGV
    signal(SIGSEGV, SIG_DFL);
#endif
#ifdef SIGSYS
    signal(SIGSYS, SIG_DFL);
#endif
#ifdef SIGTERM
    signal(SIGTERM, SIG_DFL);
#endif
}

void
md_onsignal_exit(void)
{
#ifdef SIGHUP
    signal(SIGHUP, SIG_DFL);
#endif
#ifdef SIGQUIT
    signal(SIGQUIT, exit);
#endif
#ifdef SIGILL
    signal(SIGILL, exit);
#endif
#ifdef SIGTRAP
    signal(SIGTRAP, exit);
#endif
#ifdef SIGIOT
    signal(SIGIOT, exit);
#endif
#ifdef SIGEMT
    signal(SIGEMT, exit);
#endif
#ifdef SIGFPE
    signal(SIGFPE, exit);
#endif
#ifdef SIGBUS
    signal(SIGBUS, exit);
#endif
#ifdef SIGSEGV
    signal(SIGSEGV, exit);
#endif
#ifdef SIGSYS
    signal(SIGSYS, exit);
#endif
#ifdef SIGTERM
    signal(SIGTERM, exit);
#endif
}

extern void auto_save(int sig);
extern void endit(int sig);
extern void quit(int sig);

void
md_onsignal_autosave(void)
{

#ifdef SIGHUP
    signal(SIGHUP, auto_save);
#endif
#ifdef SIGQUIT
	signal(SIGQUIT, endit);
#endif
#ifdef SIGILL
    signal(SIGILL, auto_save);
#endif
#ifdef SIGTRAP
    signal(SIGTRAP, auto_save);
#endif
#ifdef SIGIOT
    signal(SIGIOT, auto_save);
#endif
#ifdef SIGEMT
    signal(SIGEMT, auto_save);
#endif
#ifdef SIGFPE
    signal(SIGFPE, auto_save);
#endif
#ifdef SIGBUS
    signal(SIGBUS, auto_save);
#endif
#ifdef SIGSEGV
    signal(SIGSEGV, auto_save);
#endif
#ifdef SIGSYS
    signal(SIGSYS, auto_save);
#endif
#ifdef SIGTERM
    signal(SIGTERM, auto_save);
#endif
#ifdef SIGINT
    signal(SIGINT, quit);
#endif
}

int
md_hasclreol(void)
{
#if defined(clr_eol)
#ifdef NCURSES_VERSION
    if (cur_term == NULL)
	return(0);
/*    if (cur_term->type.Strings == NULL)
	return(0); */
#endif
    return((clr_eol != NULL) && (*clr_eol != 0));
#elif defined(__PDCURSES__) || defined(PDCURSES)
    return(TRUE);
#else
    return((CE != NULL) && (*CE != 0));
#endif
}

void
md_putchar(int c)
{
    putchar(c);
}

#ifdef _WIN32
/* The score list is printed after endwin(), directly through stdout rather
 * than through PDCurses.  Mixing Win32 attributes and raw SGR escapes was
 * responsible for the wrong score colors on Windows.  Use a single backend:
 * VT RGB on capable consoles, Win32 16-color attributes otherwise. */
static int md_score_vt = 0;
static int md_standout_mode = 0;
static WORD md_standout_original_attr;

void
md_raw_score_prepare(void)
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;

    md_score_vt = 0;
    md_standout_mode = 0;
    if (console != NULL && console != INVALID_HANDLE_VALUE &&
        GetConsoleMode(console, &mode))
    {
        if (SetConsoleMode(console, mode | ENABLE_PROCESSED_OUTPUT |
                           ENABLE_VIRTUAL_TERMINAL_PROCESSING))
            md_score_vt = 1;
    }
}

int
md_raw_score_uses_vt(void)
{
    return md_score_vt;
}

/* PDCurses may leave the grave or winner screen in the console buffer
 * when endwin() returns to ordinary stdout.  A plain score list must start
 * on a clean screen, not underneath that old curses drawing.  Perform this
 * only after endwin(), when raw stdout once again owns the console.
 *
 * VT-capable terminals can clear the visible screen with SGR/ED.  For older
 * ConsoleHost setups use Win32 APIs instead of emitting literal ESC text.
 * Clear just the visible viewport (not the scrollback or whole buffer).
 */
void
md_raw_score_clear_screen(void)
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    COORD origin;
    DWORD written;
    DWORD width;
    int row;
    const WORD plain_attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

    if (md_score_vt)
    {
        fflush(stdout);
        fputs("\033[0m\033[H\033[2J", stdout);
        fflush(stdout);
        return;
    }

    if (console == NULL || console == INVALID_HANDLE_VALUE ||
        !GetConsoleScreenBufferInfo(console, &info))
        return;  /* Redirected stdout: no terminal screen to clear. */

    fflush(stdout);
    width = (DWORD)(info.srWindow.Right - info.srWindow.Left + 1);
    origin.X = info.srWindow.Left;
    for (row = info.srWindow.Top; row <= info.srWindow.Bottom; ++row)
    {
        origin.Y = (SHORT)row;
        (void) FillConsoleOutputCharacterW(console, L' ', width,
                                            origin, &written);
        (void) FillConsoleOutputAttribute(console, plain_attr, width,
                                          origin, &written);
    }
    origin.Y = info.srWindow.Top;
    (void) SetConsoleCursorPosition(console, origin);
}

/* 221, 114, and 153 are the xterm colors used by the macOS version.
 * Express them as absolute RGB on Windows, so a console's 16-color theme
 * cannot turn the pastel colors into a darker cyan/blue. */
void
md_raw_score_color(int index)
{
    HANDLE console;
    WORD attr;
    const char *sequence;

    if (md_score_vt)
    {
        sequence = index == 221 ? "\033[38;2;255;215;95m" :
                   index == 114 ? "\033[38;2;135;215;135m" : NULL;
        if (sequence != NULL)
            fputs(sequence, stdout);
        return;
    }

    /* Non-VT stdout fallback: use Windows 16-color attributes. */
    console = GetStdHandle(STD_OUTPUT_HANDLE);
    attr = index == 221 ?
        (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY) :
        (FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    fflush(stdout);
    if (console != NULL && console != INVALID_HANDLE_VALUE)
        (void) SetConsoleTextAttribute(console, attr);
}

void
md_raw_score_normal(int use_color)
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

    if (md_score_vt)
    {
        /* Use the same white on black as the Mac score display. */
        if (use_color)
            fputs("\033[0m\033[38;2;229;229;229m\033[48;2;0;0;0m", stdout);
        else
            fputs("\033[0m\033[38;2;229;229;229m", stdout);
    }
    else
    {
        fflush(stdout);
        if (console != NULL && console != INVALID_HANDLE_VALUE)
            (void) SetConsoleTextAttribute(console,
                FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
    md_standout_mode = 0;
}
#endif

/* Used only while printing scores after curses has ended. */

void
md_raw_standout(int use_color)
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD fg, bg, other;

    if (md_score_vt)
    {
        if (use_color)
            fputs("\033[48;2;175;215;255m\033[38;2;0;0;0m", stdout);
        else
            fputs("\033[7m", stdout);
        md_standout_mode = 1;
        return;
    }

    fflush(stdout);
    if (!md_standout_mode && console != NULL &&
        console != INVALID_HANDLE_VALUE &&
        GetConsoleScreenBufferInfo(console, &info))
    {
        fg = info.wAttributes & 0x0f;
        bg = info.wAttributes & 0xf0;
        other = info.wAttributes & (WORD)~0x00ff;
        if (SetConsoleTextAttribute(console,
                other | (WORD)(fg << 4) | (WORD)(bg >> 4)))
        {
            md_standout_original_attr = info.wAttributes;
            md_standout_mode = 1;
        }
    }
#else
    /* Keep the original macOS/Linux 256-color score palette. */
    if (use_color)
        printf("\x1b[48;5;153m\x1b[30m");
    else
        printf("\x1b[7m");
#endif
}

void
md_raw_standend(int use_color)
{
#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

    if (md_score_vt)
    {
        md_raw_score_normal(use_color);
        return;
    }

    fflush(stdout);
    if (md_standout_mode)
    {
        if (console != NULL && console != INVALID_HANDLE_VALUE)
            (void) SetConsoleTextAttribute(console, md_standout_original_attr);
        md_standout_mode = 0;
    }
#else
    if (use_color)
        printf("\033[0m\033[37m%s", jrogue_black_background_ansi());
    else
        printf("\033[0m");
#endif
}


/*
 * OS-dependent scoreboard file operations.  Higher-level open_score(),
 * lock_sc(), and unlock_sc() live in mach_dep.c.  Lock the already-open
 * scoreboard itself so there is no separate .lck file.  Never close the
 * FILE* or its underlying descriptor/handle when releasing the lock.
 * md_score_open() retains non-truncating creation and UTF-8 Windows paths.
 */
#ifdef _WIN32

static HANDLE score_lock_handle = INVALID_HANDLE_VALUE;
static OVERLAPPED score_lock_overlapped;

static wchar_t *
md_score_utf8_to_wide(const char *path)
{
    int len;
    wchar_t *wide;

    if (path == NULL)
    {
        errno = EINVAL;
        return NULL;
    }
    len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                              path, -1, NULL, 0);
    if (len <= 0)
    {
        errno = EILSEQ;
        return NULL;
    }
    wide = malloc((size_t)len * sizeof *wide);
    if (wide == NULL)
        return NULL;
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                            path, -1, wide, len) != len)
    {
        free(wide);
        errno = EILSEQ;
        return NULL;
    }
    return wide;
}

FILE *
md_score_open(const char *path, int *created)
{
    wchar_t *wide = md_score_utf8_to_wide(path);
    int fd, is_new = 0;
    FILE *file;

    if (created != NULL)
        *created = 0;
    if (wide == NULL)
        return NULL;
    fd = _wopen(wide, _O_RDWR | _O_CREAT | _O_EXCL | _O_BINARY | _O_NOINHERIT,
                _S_IREAD | _S_IWRITE);
    if (fd >= 0)
        is_new = 1;
    else if (errno == EEXIST)
        fd = _wopen(wide, _O_RDWR | _O_BINARY | _O_NOINHERIT);
    free(wide);
    if (fd < 0)
        return NULL;
    file = _fdopen(fd, "r+b");
    if (file == NULL)
    {
        _close(fd);
        return NULL;
    }
    if (created != NULL)
        *created = is_new;
    return file;
}

int
md_score_lock_acquire(FILE *file, int timeout_ms)
{
    intptr_t os_handle;
    HANDLE handle;
    int attempt, max_attempts;
    DWORD error;

    if (score_lock_handle != INVALID_HANDLE_VALUE)
    {
        errno = EALREADY;
        return 0;
    }
    if (file == NULL || _fileno(file) < 0)
    {
        errno = EBADF;
        return 0;
    }
    os_handle = _get_osfhandle(_fileno(file));
    if (os_handle == -1)
    {
        errno = EBADF;
        return 0;
    }
    /* Use the CRT FILE*'s own HANDLE: I/O and byte-range lock must share it. */
    handle = (HANDLE)os_handle;
    memset(&score_lock_overlapped, 0, sizeof score_lock_overlapped);
    max_attempts = timeout_ms > 0 ? (timeout_ms + 99) / 100 : 0;
    for (attempt = 0; attempt <= max_attempts; ++attempt)
    {
        if (LockFileEx(handle, LOCKFILE_EXCLUSIVE_LOCK |
                       LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0,
                       &score_lock_overlapped))
        {
            score_lock_handle = handle;
            return 1;
        }
        error = GetLastError();
        if (error != ERROR_LOCK_VIOLATION && error != ERROR_IO_PENDING)
        {
            errno = EIO;
            return 0;
        }
        if (attempt != max_attempts)
            Sleep(100);
    }
    errno = EWOULDBLOCK;
    return 0;
}

void
md_score_lock_release(void)
{
    if (score_lock_handle != INVALID_HANDLE_VALUE)
    {
        (void)UnlockFileEx(score_lock_handle, 0, 1, 0,
                           &score_lock_overlapped);
        /* The CRT still owns this handle through scoreboard: do not close. */
        score_lock_handle = INVALID_HANDLE_VALUE;
    }
}

#else /* POSIX */

static int score_lock_fd = -1;

FILE *
md_score_open(const char *path, int *created)
{
    int fd, is_new = 0;
    FILE *file;

    if (created != NULL)
        *created = 0;
    if (path == NULL)
    {
        errno = EINVAL;
        return NULL;
    }
    fd = open(path, O_RDWR | O_CREAT | O_EXCL, 0664);
    if (fd >= 0)
        is_new = 1;
    else if (errno == EEXIST)
        fd = open(path, O_RDWR);
    if (fd < 0)
        return NULL;
    (void)fcntl(fd, F_SETFD, FD_CLOEXEC);
    file = fdopen(fd, "r+b");
    if (file == NULL)
    {
        close(fd);
        return NULL;
    }
    if (created != NULL)
        *created = is_new;
    return file;
}

int
md_score_lock_acquire(FILE *file, int timeout_ms)
{
    int fd, attempt, max_attempts, error;

    if (score_lock_fd >= 0)
    {
        errno = EALREADY;
        return 0;
    }
    if (file == NULL || (fd = fileno(file)) < 0)
    {
        errno = EBADF;
        return 0;
    }
    max_attempts = timeout_ms > 0 ? (timeout_ms + 99) / 100 : 0;
    for (attempt = 0; attempt <= max_attempts; ++attempt)
    {
        if (flock(fd, LOCK_EX | LOCK_NB) == 0)
        {
            score_lock_fd = fd;
            return 1;
        }
        error = errno;
        if (error != EWOULDBLOCK && error != EAGAIN && error != EINTR)
        {
            errno = error;
            return 0;
        }
        if (attempt != max_attempts)
        {
            struct timeval wait = { 0, 100000 };
            (void)select(0, NULL, NULL, NULL, &wait);
        }
    }
    errno = EWOULDBLOCK;
    return 0;
}

void
md_score_lock_release(void)
{
    if (score_lock_fd >= 0)
    {
        (void)flock(score_lock_fd, LOCK_UN);
        /* scoreboard retains ownership of the descriptor until game exit. */
        score_lock_fd = -1;
    }
}
#endif

int
md_unlink_open_file(const char *file, FILE *inf)
{
#ifdef _WIN32
    fclose(inf);
    (void) jrogue_windows_chmod_utf8(file, 0600);
    return jrogue_windows_unlink_utf8(file);
#else
    return(unlink(file));
#endif
}

int
md_unlink(char *file)
{
#ifdef _WIN32
    (void) jrogue_windows_chmod_utf8(file, 0600);
    return jrogue_windows_unlink_utf8(file);
#else
    return(unlink(file));
#endif
}

int
md_chmod(const char *filename, int mode)
{
#ifdef _WIN32
    return jrogue_windows_chmod_utf8(filename, mode);
#else
    return( chmod(filename, mode) );
#endif
}

void
md_normaluser(void)
{
#if defined(HAVE_GETGID) && defined(HAVE_GETUID)
	gid_t realgid = getgid();
	uid_t realuid = getuid();

#if defined(HAVE_SETRESGID)
    if (setresgid(-1, realgid, realgid) != 0) {
#elif defined (HAVE_SETREGID) 
    if (setregid(realgid, realgid) != 0) {
#elif defined (HAVE_SETGID)
	if (setgid(realgid) != 0) {
#else
	if (0) {
#endif
		perror("Could not drop setgid privileges.  Aborting.");
		exit(1);
    }

#if defined(HAVE_SETRESUID)
    if (setresuid(-1, realuid, realuid) != 0) {
#elif defined(HAVE_SETREUID)
    if (setreuid(realuid, realuid) != 0) {
#elif defined(HAVE_SETUID)
	if (setuid(realuid) != 0) {
#else
	if (0) {
#endif
	perror("Could not drop setuid privileges.  Aborting.");
	exit(1);
    }
#endif
}

jrogue_uid_t
md_getuid(void)
{
#ifdef HAVE_GETUID
    return( getuid() );
#else
    return(42);
#endif
}

pid_t
md_getpid(void)
{
#ifdef _WIN32
    return( _getpid() );
#else
    return( getpid() );
#endif
}

char *
md_getusername(void)
{
#ifdef _WIN32
    /* GetUserNameA() uses the active ANSI code page (often CP932), not
     * UTF-8.  The rest of Rogue, including whoami and score files, uses
     * UTF-8, so obtain the Windows account name as Unicode instead. */
    static char login[4 * (UNLEN + 1)];
    WCHAR account[UNLEN + 1];
    DWORD account_chars = UNLEN + 1;
    const char *fallback;

    if (GetUserNameW(account, &account_chars) &&
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                            account, -1, login, (int) sizeof login,
                            NULL, NULL) > 0)
        return login;

    /* Fallback for environments without a readable Windows account.
     * These variables are generally ASCII; never run them through a
     * CP932-dependent isprint() filter. */
    fallback = getenv("USERNAME");
    if (fallback == NULL || *fallback == '\0')
        fallback = getenv("USER");
    if (fallback == NULL || *fallback == '\0')
        fallback = getenv("LOGNAME");
    if (fallback == NULL || *fallback == '\0')
        fallback = "nobody";
    jrogue_strlcpy(login, fallback, sizeof login);
    return login;
#else
    static char login[80];
    const char *l = getenv("LOGNAME");

#if defined(HAVE_GETPWUID) && !defined(__DJGPP__)
    if (l == NULL || *l == '\0')
    {
        struct passwd *pw = getpwuid(getuid());
        if (pw != NULL)
            l = pw->pw_name;
    }
#endif
    if (l == NULL || *l == '\0')
        l = getenv("USERNAME");
    if (l == NULL || *l == '\0')
        l = getenv("USER");
    if (l == NULL || *l == '\0')
        l = "nobody";
    jrogue_strlcpy(login, l, sizeof login);
    return login;
#endif
}

#ifdef _WIN32
/* Convert the UTF-16 Windows folder path to the UTF-8 used by JEC's
 * save-file code.  Never return a truncated UTF-8 path: it would name a
 * different directory, or end in the middle of a multibyte character. */
static int
md_home_from_wide(char *out, size_t out_size, const WCHAR *wide)
{
    int needed;
    size_t len;

    if (wide == NULL || *wide == L'\0' || out_size > INT_MAX)
        return FALSE;
    needed = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                 wide, -1, NULL, 0, NULL, NULL);
    if (needed <= 0 || (size_t)needed > out_size)
        return FALSE;
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                            wide, -1, out, (int)out_size,
                            NULL, NULL) != needed)
    {
        out[0] = '\0';
        return FALSE;
    }

    len = strlen(out);
    if (len != 0 && out[len - 1] != '\\' && out[len - 1] != '/')
    {
        if (len + 1 >= out_size)
        {
            out[0] = '\0';
            return FALSE;
        }
        out[len++] = '\\';
        out[len] = '\0';
    }
    return TRUE;
}

static int
md_getenv_path_wide(const WCHAR *name, WCHAR *value, DWORD capacity)
{
    DWORD length = GetEnvironmentVariableW(name, value, capacity);
    /* Windows returns the required size when the buffer is too small. */
    return length != 0 && length < capacity;
}
#endif /* _WIN32 */

char *
md_gethomedir(void)
{
#ifdef _WIN32
    /* MAXSTR is also the limit of JEC's save-file path.  Keep the home
     * directory in UTF-8, never in the active ANSI code page (CP932 etc.).
     * CSIDL_PERSONAL preserves the original Documents-folder selection. */
    static char homedir[MAXSTR];
    WCHAR wide[MAXSTR];
    WCHAR path[MAXSTR];
    size_t drive_len, path_len;

    homedir[0] = '\0';
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL,
                                    SHGFP_TYPE_CURRENT, wide)) &&
        md_home_from_wide(homedir, sizeof homedir, wide))
        return homedir;

    if (md_getenv_path_wide(L"HOME", wide, MAXSTR) &&
        md_home_from_wide(homedir, sizeof homedir, wide))
        return homedir;

    /* Preserve the original HOME -> HOMEDRIVE+HOMEPATH fallback, using
     * wide-character environment access for redirected/Japanese paths. */
    if (md_getenv_path_wide(L"HOMEDRIVE", wide, MAXSTR))
    {
        drive_len = wcslen(wide);
        if (md_getenv_path_wide(L"HOMEPATH", path, MAXSTR))
        {
            path_len = wcslen(path);
            if (drive_len + path_len < MAXSTR)
                memcpy(wide + drive_len, path,
                       (path_len + 1) * sizeof *wide);
            else
                wide[0] = L'\0'; /* Never use a truncated path. */
        }
        if (md_home_from_wide(homedir, sizeof homedir, wide))
            return homedir;
    }
    homedir[0] = '\0'; /* Relative save path remains the safe fallback. */
    return homedir;
#else
    /* Unix and DJGPP paths retain the original locale/byte conventions. */
    static char homedir[PATH_MAX];
    char *h = NULL;
    size_t len;
#ifdef DJGPP
    char slash = '\\';
#else
    char slash = '/';
    struct passwd *pw;
    pw = getpwuid(getuid());

    h = pw->pw_dir;

    if (strcmp(h, "/") == 0)
        h = NULL;
#endif
    homedir[0] = 0;
    if ( (h == NULL) || (*h == '\0') )
    {
        if ( (h = getenv("HOME")) == NULL )
        {
            if ( (h = getenv("HOMEDRIVE")) == NULL)
                h = "";
            else
            {
                jrogue_strlcpy(homedir, h, sizeof homedir);

                if ( (h = getenv("HOMEPATH")) == NULL)
                    h = "";
            }
        }
    }

    len = strlen(homedir);
    jrogue_strlcat(homedir, h, sizeof homedir);
    len = strlen(homedir);

    if ((len > 0) && (homedir[len - 1] != slash)) {
        homedir[len] = slash;
        homedir[len + 1] = 0;
    }

    return homedir;
#endif
}

void
md_sleep(int s)
{
#ifdef _WIN32
    if (s > 0) Sleep((DWORD)s * 1000);
#else
    struct timeval tv;
    tv.tv_sec = s;
    tv.tv_usec = 0;
    select(0, NULL, NULL, NULL, &tv);
#endif
}

char *
md_getshell(void)
{
    static char shell[PATH_MAX];
    char *s = NULL;
#ifdef _WIN32
    char *def = "C:\\WINDOWS\\SYSTEM32\\CMD.EXE";
#elif defined(__DJGPP__)
    char *def = "C:\\COMMAND.COM";
#else
    char *def = "/bin/sh";
    struct passwd *pw;
    pw = getpwuid(getuid());
    s = pw->pw_shell;
#endif
    if ((s == NULL) || (*s == '\0'))
        if ( (s = getenv("COMSPEC")) == NULL)
            if ( (s = getenv("SHELL")) == NULL)
                if ( (s = getenv("SystemRoot")) == NULL)
                    s = def;

    jrogue_strlcpy(shell, s, sizeof shell);

    return(shell);
}

int
md_shellescape(void)
{
#if defined(_WIN32)
    /* Run the Windows command interpreter in the current console, waiting
     * for "exit" before returning to the existing curses restore path.
     * Use wide strings so a Unicode COMSPEC path works with MinGW-w64. */
    const wchar_t *shell = _wgetenv(L"COMSPEC");

    if (shell == NULL || *shell == L'\0')
        shell = L"cmd.exe";
    return (int)_wspawnlp(_P_WAIT, shell, L"cmd.exe", (const wchar_t *)NULL);
#elif defined(HAVE_WORKING_FORK)
    int ret_status;
    int pid;
    void (*old_sigint)(int);
#ifdef SIGQUIT
    void (*old_sigquit)(int);
#endif
    char *sh;

    sh = md_getshell();

    while((pid = fork()) < 0)
        md_sleep(1);

    if (pid == 0) /* Shell Process */
    {
        /*
         * Set back to original user, just in case
         */
        md_normaluser();
        execl(sh == NULL ? "/bin/sh" : sh, "shell", "-i", NULL);
        perror("No shelly");
        _exit(-1);
    }
    else /* Application */
    {
        old_sigint = signal(SIGINT, SIG_IGN);
#ifdef SIGQUIT
        old_sigquit = signal(SIGQUIT, SIG_IGN);
#endif
        while (wait(&ret_status) != pid)
            continue;

        /* Restore each signal's own handler (the RRP version swapped them). */
        signal(SIGINT, old_sigint);
#ifdef SIGQUIT
        signal(SIGQUIT, old_sigquit);
#endif
    }
    return(ret_status);
#elif defined(HAVE__SPAWNL) 
    return((int)_spawnl(_P_WAIT,md_getshell(),"shell",NULL,0));
#elif defined(HAVE_SPAWNL)
    return ( spawnl(P_WAIT,md_getshell(),"shell",NULL,0) );
#else
	return(0);
#endif
}

int
directory_exists(char *dirname)
{
    struct stat sb;

    if (stat(dirname, &sb) == 0) /* path exists */
        #ifdef _WIN32
        return (sb.st_mode & _S_IFDIR) != 0;
#else
        return (sb.st_mode & S_IFDIR) != 0;
#endif

    return(0);
}

char *
md_getrealname(jrogue_uid_t uid)
{
    static char uidstr[20];
#if !defined(_WIN32) && !defined(DJGPP)
    struct passwd *pp;

	if ((pp = getpwuid(uid)) == NULL)
    {
        snprintf(uidstr, sizeof uidstr, "%d", uid);
        return(uidstr);
    }
	else
	    return(pp->pw_name);
#else
   snprintf(uidstr, sizeof uidstr, "%ld", (long) uid);
   return(uidstr);
#endif
}

char *
md_crypt(const char *key, const char *salt)
{
    return( xcrypt(key,salt) );
}

char *
md_getpass(const char *prompt)
{
#ifndef HAVE_GETPASS
    static char password_buffer[9];
    char *p = password_buffer;
    int c, count = 0;
    int max_length = 9;

    fflush(stdout);
    /* If we can't prompt, abort */
    if (fputs(prompt, stderr) < 0)
    {
        *p = '\0';
        return NULL;
    }

    for(;;)
    {
        /* Get a character with no echo */
        c = _getch();

        /* Exit on interrupt (^c or ^break) */
        if (c == '\003' || c == 0x100)
            exit(1);

        /* Terminate on end of line or file (^j, ^m, ^d, ^z) */
        if (c == '\r' || c == '\n' || c == '\004' || c == '\032')
            break;

        /* Back up on backspace */
        if (c == '\b')
        {
            if (count)
                count--;
            else if (p > password_buffer)
                p--;
            continue;
        }

        /* Ignore DOS extended characters */
        if ((c & 0xff) != c)
            continue;

        /* Add to password if it isn't full */
        if (p < password_buffer + max_length - 1)
            *p++ = (char) c;
        else
            count++;
    }
   *p = '\0';

   fputc('\n', stderr);

   return password_buffer;
#else
   return( getpass(prompt) );
#endif
}

int
md_erasechar(void)
{
#if defined(_WIN32)
    return '\b';
#elif defined(HAVE_ERASECHAR)
    return( erasechar() ); /* process erase character */
#elif defined(VERASE)
    return(_tty.c_cc[VERASE]); /* process erase character */
#else
    return(_tty.sg_erase); /* process erase character */
#endif
}

int
md_killchar(void)
{
#if defined(_WIN32)
    return 21; /* Ctrl-U */
#elif defined(HAVE_KILLCHAR)
    return( killchar() );
#elif defined(VKILL)
    return(_tty.c_cc[VKILL]);
#else
    return(_tty.sg_kill);
#endif
}

int
md_dsuspchar(void)
{
#if defined(VDSUSP)			/* POSIX has priority */
    struct termios attr;
    tcgetattr(STDIN_FILENO, &attr);
    return( attr.c_cc[VDSUSP] );
#elif defined(TIOCGLTC)
    struct ltchars ltc;
    ioctl(1, TIOCGLTC, &ltc);
    return(ltc.t_dsuspc);
#elif defined(_POSIX_VDISABLE)
    return(_POSIX_VDISABLE);
#else
    return(0);
#endif
}

int
md_setdsuspchar(int c)
{
#if defined(VDSUSP)			/* POSIX has priority */
    struct termios attr;
    /* Read the existing settings before changing the delayed suspend key.
     * The original called tcgetattr() twice, so the change was never set. */
    if (tcgetattr(STDIN_FILENO, &attr) == 0) {
        attr.c_cc[VDSUSP] = (cc_t)c;
        (void)tcsetattr(STDIN_FILENO, TCSANOW, &attr);
    }
#elif defined(TIOCSLTC)
    struct ltchars ltc;
    ioctl(1, TIOCGLTC, &ltc);
    ltc.t_dsuspc = c;
    ioctl(1, TIOCSLTC, &ltc);
#else
    (void)c;
#endif
    return(0);
}

int
md_suspchar(void)
{
#if defined(VSUSP)			/* POSIX has priority */
    struct termios attr;
    tcgetattr(STDIN_FILENO, &attr);
    return( attr.c_cc[VSUSP] );
#elif defined(TIOCGLTC)
    struct ltchars ltc;
    ioctl(1, TIOCGLTC, &ltc);
    return(ltc.t_suspc);
#elif defined(_POSIX_VDISABLE)
    return(_POSIX_VDISABLE);
#else
    return(0);
#endif
}

int
md_setsuspchar(int c)
{
#if defined(VSUSP)			/* POSIX has priority */
    struct termios attr;
    /* Apply the modified suspend key, preserving the other tty settings. */
    if (tcgetattr(STDIN_FILENO, &attr) == 0) {
        attr.c_cc[VSUSP] = (cc_t)c;
        (void)tcsetattr(STDIN_FILENO, TCSANOW, &attr);
    }
#elif defined(TIOCSLTC)
    struct ltchars ltc;
    ioctl(1, TIOCGLTC, &ltc);
    ltc.t_suspc = c;
    ioctl(1, TIOCSLTC, &ltc);
#else
    (void)c;
#endif

    return(0);
}

/*
    Cursor/Keypad Support

    Sadly Cursor/Keypad support is less straightforward than it should be.
    
    The various terminal emulators/consoles choose to differentiate the 
    cursor and keypad keys (with modifiers) in different ways (if at all!). 
    Furthermore they use different code set sequences for each key only
    a subset of which the various curses libraries recognize. Partly due
    to incomplete termcap/terminfo entries and partly due to inherent 
    limitations of those terminal capability databases.

    I give curses first crack at decoding the sequences. If it fails to decode
    it we check for common ESC-prefixed sequences.

    All cursor/keypad results are translated into standard rogue movement 
    commands.

    Unmodified keys are translated to walk commands: hjklyubn
    Modified (shift,control,alt) are translated to run commands: HJKLYUBN

    Console and supported (differentiated) keys
    Interix:  Cursor Keys, Keypad, Ctl-Keypad
    Cygwin:   Cursor Keys, Keypad, Alt-Cursor Keys
    MSYS:     Cursor Keys, Keypad, Ctl-Cursor Keys, Ctl-Keypad
    Win32:    Cursor Keys, Keypad, Ctl/Shift/Alt-Cursor Keys, Ctl/Alt-Keypad
    DJGPP:    Cursor Keys, Keypad, Ctl/Shift/Alt-Cursor Keys, Ctl/Alt-Keypad

    Interix Console (raw, ncurses)
    ==============================
    normal	shift		ctrl	    alt
    ESC [D,	ESC F^,		ESC [D,	    ESC [D	    /# Left	    #/
    ESC [C,	ESC F$,		ESC [C,	    ESC [C	    /# Right	    #/
    ESC [A,	ESC F-,		local win,  ESC [A	    /# Up	    #/
    ESC [B,	ESC F+,		local win,  ESC [B	    /# Down	    #/
    ESC [H,	ESC [H,		ESC [H,	    ESC [H	    /# Home	    #/
    ESC [S,	local win,	ESC [S,	    ESC [S	    /# Page Up	    #/
    ESC [T,	local win,	ESC [T,	    ESC [T	    /# Page Down    #/
    ESC [U,	ESC [U,		ESC [U,	    ESC [U	    /# End	    #/
    ESC [D,	ESC F^,		ESC [D,	    O		    /# Keypad Left  #/
    ESC [C,	ESC F$,		ESC [C,	    O		    /# Keypad Right #/
    ESC [A,	ESC [A,		ESC [-1,    O		    /# Keypad Up    #/
    ESC [B,	ESC [B,		ESC [-2,    O		    /# Keypad Down  #/
    ESC [H,	ESC [H,		ESC [-263,  O		    /# Keypad Home  #/
    ESC [S,	ESC [S,		ESC [-19,   O		    /# Keypad PgUp  #/
    ESC [T,	ESC [T,		ESC [-20,   O		    /# Keypad PgDn  #/
    ESC [U,	ESC [U,		ESC [-21,   O		    /# Keypad End   #/
    nothing,	nothing,	nothing,    O		    /# Kaypad 5     #/

    Interix Console (term=interix, ncurses)
    ==============================
    KEY_LEFT,	ESC F^,		KEY_LEFT,   KEY_LEFT	    /# Left	    #/
    KEY_RIGHT,	ESC F$,		KEY_RIGHT,  KEY_RIGHT	    /# Right	    #/
    KEY_UP,	0x146,		local win,  KEY_UP	    /# Up	    #/
    KEY_DOWN,	0x145,		local win,  KEY_DOWN	    /# Down	    #/
    ESC [H,	ESC [H,		ESC [H,	    ESC [H	    /# Home	    #/
    KEY_PPAGE,	local win,	KEY_PPAGE,  KEY_PPAGE	    /# Page Up	    #/
    KEY_NPAGE,	local win,	KEY_NPAGE,  KEY_NPAGE	    /# Page Down    #/
    KEY_LL,	KEY_LL,		KEY_LL,	    KEY_LL	    /# End	    #/
    KEY_LEFT,	ESC F^,		ESC [-4,    O		    /# Keypad Left  #/
    KEY_RIGHT,	ESC F$,		ESC [-3,    O		    /# Keypad Right #/
    KEY_UP,	KEY_UP,		ESC [-1,    O		    /# Keypad Up    #/
    KEY_DOWN,	KEY_DOWN,	ESC [-2,    O		    /# Keypad Down  #/
    ESC [H,	ESC [H,		ESC [-263,  O		    /# Keypad Home  #/
    KEY_PPAGE,	KEY_PPAGE,	ESC [-19,   O		    /# Keypad PgUp  #/
    KEY_NPAGE,	KEY_NPAGE,	ESC [-20,   O		    /# Keypad PgDn  #/
    KEY_LL,	KEY_LL,		ESC [-21,   O		    /# Keypad End   #/
    nothing,	nothing,	nothing,    O		    /# Keypad 5     #/

    Cygwin Console (raw, ncurses)
    ==============================
    normal	shift		ctrl	    alt
    ESC [D,	ESC [D,		ESC [D,	    ESC ESC [D	    /# Left	    #/
    ESC [C,	ESC [C,		ESC [C,	    ESC ESC [C	    /# Rght	    #/
    ESC [A,	ESC [A,		ESC [A,	    ESC ESC [A	    /# Up	    #/
    ESC [B,	ESC [B,		ESC [B,	    ESC ESC [B	    /# Down	    #/
    ESC [1~,	ESC [1~,	ESC [1~,    ESC ESC [1~	    /# Home	    #/
    ESC [5~,	ESC [5~,	ESC [5~,    ESC ESC [5~	    /# Page Up	    #/
    ESC [6~,	ESC [6~,	ESC [6~,    ESC ESC [6~	    /# Page Down    #/
    ESC [4~,	ESC [4~,	ESC [4~,    ESC ESC [4~	    /# End	    #/
    ESC [D,	ESC [D,		ESC [D,	    ESC ESC [D,O    /# Keypad Left  #/
    ESC [C,	ESC [C,		ESC [C,	    ESC ESC [C,O    /# Keypad Right #/
    ESC [A,	ESC [A,		ESC [A,	    ESC ESC [A,O    /# Keypad Up    #/
    ESC [B,	ESC [B,		ESC [B,	    ESC ESC [B,O    /# Keypad Down  #/
    ESC [1~,	ESC [1~,	ESC [1~,    ESC ESC [1~,O   /# Keypad Home  #/
    ESC [5~,	ESC [5~,	ESC [5~,    ESC ESC [5~,O   /# Keypad PgUp  #/
    ESC [6~,	ESC [6~,	ESC [6~,    ESC ESC [6~,O   /# Keypad PgDn  #/
    ESC [4~,	ESC [4~,	ESC [4~,    ESC ESC [4~,O   /# Keypad End   #/
    ESC [-71,	nothing,	nothing,    O	            /# Keypad 5	    #/

    Cygwin Console (term=cygwin, ncurses)
    ==============================
    KEY_LEFT,	KEY_LEFT,	KEY_LEFT,   ESC-260	    /# Left	    #/
    KEY_RIGHT,	KEY_RIGHT,	KEY_RIGHT,  ESC-261	    /# Rght	    #/
    KEY_UP,	KEY_UP,		KEY_UP,	    ESC-259	    /# Up	    #/
    KEY_DOWN,	KEY_DOWN,	KEY_DOWN,   ESC-258	    /# Down	    #/
    KEY_HOME,	KEY_HOME,	KEY_HOME,   ESC-262	    /# Home	    #/
    KEY_PPAGE,	KEY_PPAGE,	KEY_PPAGE,  ESC-339	    /# Page Up	    #/
    KEY_NPAGE,	KEY_NPAGE,	KEY_NPAGE,  ESC-338	    /# Page Down    #/
    KEY_END,	KEY_END,	KEY_END,    ESC-360	    /# End	    #/
    KEY_LEFT,	KEY_LEFT,	KEY_LEFT,   ESC-260,O	    /# Keypad Left  #/
    KEY_RIGHT,	KEY_RIGHT,	KEY_RIGHT,  ESC-261,O	    /# Keypad Right #/
    KEY_UP,	KEY_UP,		KEY_UP,	    ESC-259,O       /# Keypad Up    #/
    KEY_DOWN,	KEY_DOWN,	KEY_DOWN,   ESC-258,O       /# Keypad Down  #/
    KEY_HOME,	KEY_HOME,	KEY_HOME,   ESC-262,O       /# Keypad Home  #/
    KEY_PPAGE,	KEY_PPAGE,	KEY_PPAGE,  ESC-339,O	    /# Keypad PgUp  #/
    KEY_NPAGE,	KEY_NPAGE,	KEY_NPAGE,  ESC-338,O	    /# Keypad PgDn  #/
    KEY_END,	KEY_END,	KEY_END,    ESC-360,O       /# Keypad End   #/
    ESC [G,	nothing,	nothing,    O	            /# Keypad 5	    #/

    MSYS Console (raw, ncurses)
    ==============================
    normal	shift		ctrl	    alt
    ESC OD,	ESC [d,		ESC Od	    nothing	    /# Left	    #/
    ESC OE,	ESC [e,		ESC Oe,	    nothing	    /# Right	    #/
    ESC OA,	ESC [a,		ESC Oa,	    nothing	    /# Up	    #/
    ESC OB,	ESC [b,		ESC Ob,	    nothing	    /# Down	    #/
    ESC [7~,	ESC [7$,	ESC [7^,    nothing	    /# Home	    #/
    ESC [5~,	local window,   ESC [5^,    nothing	    /# Page Up      #/
    ESC [6~,	local window,   ESC [6^,    nothing	    /# Page Down    #/
    ESC [8~,	ESC [8$,	ESC [8^,    nothing	    /# End	    #/
    ESC OD,	ESC [d,		ESC Od	    O		    /# Keypad Left  #/
    ESC OE,	ESC [c,		ESC Oc,	    O		    /# Keypad Right #/
    ESC OA,	ESC [a,		ESC Oa,	    O		    /# Keypad Up    #/
    ESC OB,	ESC [b,		ESC Ob,	    O		    /# Keypad Down  #/
    ESC [7~,	ESC [7$,	ESC [7^,    O		    /# Keypad Home  #/
    ESC [5~,	local window,   ESC [5^,    O		    /# Keypad PgUp  #/
    ESC [6~,	local window,   ESC [6^,    O		    /# Keypad PgDn  #/
    ESC [8~,	ESC [8$,	ESC [8^,    O		    /# Keypad End   #/
    11,		11,		11,	    O		    /# Keypad 5     #/

    MSYS Console (term=rxvt, ncurses)
    ==============================
    normal	shift		ctrl	    alt
    KEY_LEFT,	KEY_SLEFT,	514	    nothing	    /# Left	    #/
    KEY_RIGHT,	KEY_SRIGHT,	516,	    nothing	    /# Right	    #/
    KEY_UP,	518,		519,	    nothing	    /# Up	    #/
    KEY_DOWN,	511,		512,	    nothing	    /# Down	    #/
    KEY_HOME,	KEY_SHOME,	ESC [7^,    nothing	    /# Home	    #/
    KEY_PPAGE,	local window,   ESC [5^,    nothing	    /# Page Up      #/
    KEY_NPAGE,	local window,   ESC [6^,    nothing	    /# Page Down    #/
    KEY_END,	KEY_SEND,	KEY_EOL,    nothing	    /# End	    #/
    KEY_LEFT,	KEY_SLEFT,	514	    O		    /# Keypad Left  #/
    KEY_RIGHT,	KEY_SRIGHT,	516,	    O		    /# Keypad Right #/
    KEY_UP,	518,		519,	    O		    /# Keypad Up    #/
    KEY_DOWN,	511,		512,	    O		    /# Keypad Down  #/
    KEY_HOME,	KEY_SHOME,	ESC [7^,    O		    /# Keypad Home  #/
    KEY_PPAGE,	local window,   ESC [5^,    O		    /# Keypad PgUp  #/
    KEY_NPAGE,	local window,   ESC [6^,    O		    /# Keypad PgDn  #/
    KEY_END,	KEY_SEND,	KEY_EOL,    O		    /# Keypad End   #/
    11,		11,		11,	    O		    /# Keypad 5     #/

    Win32 Console (raw, pdcurses)
    DJGPP Console (raw, pdcurses)
    ==============================
    normal	shift		ctrl	    alt
    260,	391,		443,	    493		    /# Left	    #/
    261,	400,		444,	    492		    /# Right	    #/
    259,	547,		480,	    490		    /# Up	    #/
    258,	548,		481,	    491		    /# Down	    #/
    262,	388,		447,	    524	    	    /# Home	    #/
    339,	396,		445,	    526	    	    /# Page Up	    #/
    338,	394,		446,	    520		    /# Page Down    #/
    358,	384,		448,	    518	 	    /# End	    #/
    452,	52('4'),	511,	    521		    /# Keypad Left  #/
    454,	54('6'),	513,	    523		    /# Keypad Right #/
    450,	56('8'),	515,	    525		    /# Keypad Up    #/
    456,	50('2'),	509,	    519		    /# Keypad Down  #/
    449,	55('7'),	514,	    524		    /# Keypad Home  #/
    451,	57('9'),	516,	    526		    /# Keypad PgUp  #/
    457,	51('3'),	510,	    520		    /# Keypad PgDn  #/
    455,	49('1'),	508,	    518		    /# Keypad End   #/
    453,	53('5'),	512,	    522		    /# Keypad 5     #/

    Win32 Console (pdcurses, MSVC/MingW32)
    DJGPP Console (pdcurses)
    ==============================
    normal	shift		ctrl	    alt
    KEY_LEFT,	KEY_SLEFT,	CTL_LEFT,   ALT_LEFT	    /# Left	    #/
    KEY_RIGHT,	KEY_SRIGHT,	CTL_RIGHT,  ALT_RIGHT	    /# Right	    #/
    KEY_UP,	KEY_SUP,	CTL_UP,	    ALT_UP	    /# Up	    #/
    KEY_DOWN,	KEY_SDOWN,	CTL_DOWN,   ALT_DOWN	    /# Down	    #/
    KEY_HOME,	KEY_SHOME,	CTL_HOME,   ALT_HOME	    /# Home	    #/
    KEY_PPAGE,	KEY_SPREVIOUS,  CTL_PGUP,   ALT_PGUP	    /# Page Up      #/
    KEY_NPAGE,	KEY_SNEXTE,	CTL_PGDN,   ALT_PGDN	    /# Page Down    #/
    KEY_END,	KEY_SEND,	CTL_END,    ALT_END	    /# End	    #/
    KEY_B1,	52('4'),	CTL_PAD4,   ALT_PAD4	    /# Keypad Left  #/
    KEY_B3,	54('6'),	CTL_PAD6,   ALT_PAD6	    /# Keypad Right #/
    KEY_A2,	56('8'),	CTL_PAD8,   ALT_PAD8	    /# Keypad Up    #/
    KEY_C2,	50('2'),	CTL_PAD2,   ALT_PAD2	    /# Keypad Down  #/
    KEY_A1,	55('7'),	CTL_PAD7,   ALT_PAD7	    /# Keypad Home  #/
    KEY_A3,	57('9'),	CTL_PAD9,   ALT_PAD9	    /# Keypad PgUp  #/
    KEY_C3,	51('3'),	CTL_PAD3,   ALT_PAD3	    /# Keypad PgDn  #/
    KEY_C1,	49('1'),	CTL_PAD1,   ALT_PAD1	    /# Keypad End   #/
    KEY_B2,	53('5'),	CTL_PAD5,   ALT_PAD5	    /# Keypad 5     #/

    Windows Telnet (raw)
    ==============================
    normal	shift		ctrl	    alt
    ESC [D,	ESC [D,		ESC [D,	    ESC [D	    /# Left	    #/
    ESC [C,	ESC [C,		ESC [C,	    ESC [C	    /# Right	    #/
    ESC [A,	ESC [A,		ESC [A,	    ESC [A	    /# Up	    #/
    ESC [B,	ESC [B,		ESC [B,	    ESC [B	    /# Down	    #/
    ESC [1~,	ESC [1~,	ESC [1~,    ESC [1~	    /# Home	    #/
    ESC [5~,	ESC [5~,	ESC [5~,    ESC [5~	    /# Page Up	    #/
    ESC [6~,	ESC [6~,	ESC [6~,    ESC [6~	    /# Page Down    #/
    ESC [4~,	ESC [4~,	ESC [4~,    ESC [4~	    /# End	    #/
    ESC [D,	ESC [D,		ESC [D,	    ESC [D	    /# Keypad Left  #/
    ESC [C,	ESC [C,		ESC [C,	    ESC [C	    /# Keypad Right #/
    ESC [A,	ESC [A,		ESC [A,	    ESC [A	    /# Keypad Up    #/
    ESC [B,	ESC [B,		ESC [B,	    ESC [B	    /# Keypad Down  #/
    ESC [1~,	ESC [1~,	ESC [1~,    ESC [1~	    /# Keypad Home  #/
    ESC [5~,	ESC [5~,	ESC [5~,    ESC [5~	    /# Keypad PgUp  #/
    ESC [6~,	ESC [6~,	ESC [6~,    ESC [6~	    /# Keypad PgDn  #/
    ESC [4~,	ESC [4~,	ESC [4~,    ESC [4~	    /# Keypad End   #/
    nothing,	nothing,	nothing,    nothing	    /# Keypad 5     #/

    Windows Telnet (term=xterm)
    ==============================
    normal	shift		ctrl	    alt
    KEY_LEFT,	KEY_LEFT,	KEY_LEFT,   KEY_LEFT	    /# Left	    #/
    KEY_RIGHT,	KEY_RIGHT,	KEY_RIGHT,  KEY_RIGHT	    /# Right	    #/
    KEY_UP,	KEY_UP,		KEY_UP,	    KEY_UP	    /# Up	    #/
    KEY_DOWN,	KEY_DOWN,	KEY_DOWN,   KEY_DOWN	    /# Down	    #/
    ESC [1~,	ESC [1~,	ESC [1~,    ESC [1~	    /# Home	    #/
    KEY_PPAGE,	KEY_PPAGE,	KEY_PPAGE,  KEY_PPAGE	    /# Page Up	    #/
    KEY_NPAGE,	KEY_NPAGE,	KEY_NPAGE,  KEY_NPAGE	    /# Page Down    #/
    ESC [4~,	ESC [4~,	ESC [4~,    ESC [4~	    /# End	    #/
    KEY_LEFT,	KEY_LEFT,	KEY_LEFT,   O		    /# Keypad Left  #/
    KEY_RIGHT,	KEY_RIGHT,	KEY_RIGHT,  O		    /# Keypad Right #/
    KEY_UP,	KEY_UP,		KEY_UP,	    O		    /# Keypad Up    #/
    KEY_DOWN,	KEY_DOWN,	KEY_DOWN,   O		    /# Keypad Down  #/
    ESC [1~,	ESC [1~,	ESC [1~,    ESC [1~	    /# Keypad Home  #/
    KEY_PPAGE,	KEY_PPAGE,	KEY_PPAGE,  KEY_PPAGE	    /# Keypad PgUp  #/
    KEY_NPAGE,	KEY_NPAGE,	KEY_NPAGE,  KEY_NPAGE	    /# Keypad PgDn  #/
    ESC [4~,	ESC [4~,	ESC [4~,    O		    /# Keypad End   #/
    ESC [-71,	nothing,	nothing,    O	            /# Keypad 5	    #/

    PuTTY
    ==============================
    normal	shift		ctrl	    alt
    ESC [D,	ESC [D,		ESC OD,	    ESC [D	    /# Left	    #/
    ESC [C,	ESC [C,		ESC OC,	    ESC [C	    /# Right	    #/
    ESC [A,	ESC [A,		ESC OA,	    ESC [A	    /# Up	    #/
    ESC [B,	ESC [B,		ESC OB,	    ESC [B	    /# Down	    #/
    ESC [1~,	ESC [1~,	local win,  ESC [1~	    /# Home	    #/
    ESC [5~,	local win,	local win,  ESC [5~	    /# Page Up	    #/
    ESC [6~,	local win,	local win,  ESC [6~	    /# Page Down    #/
    ESC [4~,	ESC [4~,	local win,  ESC [4~	    /# End	    #/
    ESC [D,	ESC [D,		ESC [D,	    O		    /# Keypad Left  #/
    ESC [C,	ESC [C,		ESC [C,	    O		    /# Keypad Right #/
    ESC [A,	ESC [A,		ESC [A,	    O		    /# Keypad Up    #/
    ESC [B,	ESC [B,		ESC [B,	    O		    /# Keypad Down  #/
    ESC [1~,	ESC [1~,	ESC [1~,    O		    /# Keypad Home  #/
    ESC [5~,	ESC [5~,	ESC [5~,    O		    /# Keypad PgUp  #/
    ESC [6~,	ESC [6~,	ESC [6~,    O		    /# Keypad PgDn  #/
    ESC [4~,	ESC [4~,	ESC [4~,    O		    /# Keypad End   #/
    nothing,	nothing,	nothing,    O		    /# Keypad 5	    #/

    PuTTY
    ==============================
    normal	shift		ctrl	    alt
    KEY_LEFT,	KEY_LEFT,	ESC OD,	    ESC KEY_LEFT    /# Left	    #/
    KEY_RIGHT	KEY_RIGHT,	ESC OC,	    ESC KEY_RIGHT   /# Right	    #/
    KEY_UP,	KEY_UP,		ESC OA,	    ESC KEY_UP	    /# Up	    #/
    KEY_DOWN,	KEY_DOWN,	ESC OB,	    ESC KEY_DOWN    /# Down	    #/
    ESC [1~,	ESC [1~,	local win,  ESC ESC [1~	    /# Home	    #/
    KEY_PPAGE	local win,	local win,  ESC KEY_PPAGE   /# Page Up	    #/
    KEY_NPAGE	local win,	local win,  ESC KEY_NPAGE   /# Page Down    #/
    ESC [4~,	ESC [4~,	local win,  ESC ESC [4~	    /# End	    #/
    ESC Ot,	ESC Ot,		ESC Ot,	    O		    /# Keypad Left  #/
    ESC Ov,	ESC Ov,		ESC Ov,	    O		    /# Keypad Right #/
    ESC Ox,	ESC Ox,		ESC Ox,	    O		    /# Keypad Up    #/
    ESC Or,	ESC Or,		ESC Or,	    O		    /# Keypad Down  #/
    ESC Ow,	ESC Ow,		ESC Ow,     O		    /# Keypad Home  #/
    ESC Oy,	ESC Oy,		ESC Oy,     O		    /# Keypad PgUp  #/
    ESC Os,	ESC Os,		ESC Os,     O		    /# Keypad PgDn  #/
    ESC Oq,	ESC Oq,		ESC Oq,     O		    /# Keypad End   #/
    ESC Ou,	ESC Ou,		ESC Ou,	    O		    /# Keypad 5	    #/
*/

#define M_NORMAL 0
#define M_ESC    1
#define M_KEYPAD 2
#define M_TRAIL  3

int undo[5];
int uindex = -1;

int
reread()
{
    int redo;

    if (uindex < 0)
        return 0;

    redo = undo[0];
    undo[0] = undo[1];
    undo[1] = undo[2];
    undo[2] = undo[3];
    undo[3] = undo[4];
    uindex--;
    return redo;
}

void
unread(int c)
{
    if (uindex >= 4)
        abort();

    undo[++uindex] = c;
}

/* JEC: normalize curses input without interpreting UTF-8 bytes or
 * unmapped Windows special keys as Rogue control commands. */
static inline int
jrogue_finish_keycode(int ch)
{
#if defined(_WIN32)
    /* MSYS2 UCRT64 / PDCursesMod can report Ctrl-J as CTL_ENTER,
     * rather than the ASCII line feed (0x0a).  This was observed as
     * 0xED11 with keyname=CTL_ENTER in an actual key-probe log.
     *
     * The named constant is preferred where available.  Retain the
     * observed numerical encoding as a narrow compatibility fallback
     * because installed PDCurses variants assign differing key values.
     * Do not reduce all special keys with '& 0x7f': that turns them into
     * unrelated Ctrl-key commands (e.g. ^Q).
     */
# if defined(CTL_ENTER)
    if (ch == CTL_ENTER)
        return 0x0a;  /* Rogue's Ctrl-J: run downward */
# endif
    if (ch == 0xED11)
        return 0x0a;
#endif
#if defined(_WIN32) && defined(KEY_MIN)
    /* Don't turn an unhandled KEY_* such as 0x111 (KEY_F9) into ^Q.
     * The normalizer in io.c ignores INVALID_INPUT.
     */
    if (ch >= KEY_MIN)
        return INVALID_INPUT;
#endif
    /* Multibyte text is filtered in io.c.  Keep the bytes unchanged. */
    if (ch >= 0x80 && ch <= 0xff)
        return ch;
    return ch & 0x7f;
}

int
md_readchar(WINDOW *win)
{
    int ch = 0;
    int lastch = 0;
    int mode = M_NORMAL;
    int mode2 = M_NORMAL;
    int nodelayf = 0;
    int count = 0;

    for(;;)
    {
        if (mode == M_NORMAL && uindex >= 0) 
	{
	    ch = reread();
	    break;
	}

	ch = wgetch(win);

        if (ch == ERR) /* timed out  or error */
        {
            if (nodelayf)               /* likely timed out, switch to */
            {                           /* normal mode and block on    */
                mode = M_NORMAL;        /* next read                   */
                nodelayf = 0;
                nodelay(win,0);
            }
            else if (count > 10)        /* after 10 errors assume      */
                auto_save(0);           /* input stream is broken and  */
            else                        /* auto save and exit          */
                count++;

            continue;
        }

        count = 0;                      /* reset input error count     */

	if (mode == M_TRAIL)
	{
	    if (ch == '^')		/* msys console  : 7,5,6,8: modified*/
		ch = CTRL( toupper(lastch) );
            else if (ch == '~')		/* cygwin console: 1,5,6,4: normal  */
		ch = tolower(lastch);   /* windows telnet: 1,5,6,4: normal  */
					/* msys console  : 7,5,6,8: normal  */
	    else if (mode2 == M_ESC)		/* cygwin console: 1,5,6,4: modified*/
		ch = CTRL( toupper(ch) );
	    else
	    {
	    	mode = M_NORMAL;
		unread(ch);
		continue;
	    }

	    break;
	}

	if (mode == M_ESC) 
	{
	    if (ch == 27)
	    {
		mode2 = M_ESC;
		unread(ch);
		continue;
	    }

	    if ((ch == 'F') || (ch == 'O') || (ch == '['))
	    {
		mode = M_KEYPAD;
		unread(ch);
		continue;
	    }


	    switch(ch)
	    {
		/* Cygwin Console   */
		/* PuTTY	    */
		case KEY_LEFT :	ch = CTRL('H'); break;
		case KEY_RIGHT: ch = CTRL('L'); break;
		case KEY_UP   : ch = CTRL('K'); break;
		case KEY_DOWN : ch = CTRL('J'); break;
		case KEY_HOME : ch = CTRL('Y'); break;
		case KEY_PPAGE: ch = CTRL('U'); break;
		case KEY_NPAGE: ch = CTRL('N'); break;
		case KEY_END  : ch = CTRL('B'); break;

		default: mode = M_NORMAL;
		         mode2 = M_NORMAL;
			 unread(ch);
		         continue;
	    }

	    break;
	}

	if (mode == M_KEYPAD)
	{
	    switch(ch)
	    {
		/* ESC F - Interix Console codes */
		case   '^': ch = CTRL('H'); break;	/* Shift-Left	    */
		case   '$': ch = CTRL('L'); break;	/* Shift-Right	    */

		/* ESC [ - Interix Console codes */
		case   'H': ch = 'y'; break;		/* Home		    */
		case     1: ch = CTRL('K'); break;	/* Ctl-Keypad Up    */
		case     2: ch = CTRL('J'); break;	/* Ctl-Keypad Down  */
		case     3: ch = CTRL('L'); break;	/* Ctl-Keypad Right */
		case     4: ch = CTRL('H'); break;	/* Ctl-Keypad Left  */
		case   263: ch = CTRL('Y'); break;	/* Ctl-Keypad Home  */
		case    19: ch = CTRL('U'); break;	/* Ctl-Keypad PgUp  */
		case    20: ch = CTRL('N'); break;	/* Ctl-Keypad PgDn  */
		case    21: ch = CTRL('B'); break;	/* Ctl-Keypad End   */

		/* ESC [ - Cygwin Console codes */
		case   'G': ch = '.'; break;		/* Keypad 5	    */
		case   '7': lastch = 'Y'; mode=M_TRAIL; break;	/* Ctl-Home */
		case   '5': lastch = 'U'; mode=M_TRAIL; break;	/* Ctl-PgUp */
		case   '6': lastch = 'N'; mode=M_TRAIL; break;	/* Ctl-PgDn */

		/* ESC [ - Win32 Telnet, PuTTY */
		case   '1': lastch = 'y'; mode=M_TRAIL; break;	/* Home	    */
		case   '4': lastch = 'b'; mode=M_TRAIL; break;	/* End	    */

		/* ESC O - PuTTY */
		case   'D': ch = CTRL('H'); break;
		case   'C': ch = CTRL('L'); break;
		case   'A': ch = CTRL('K'); break;
		case   'B': ch = CTRL('J'); break;
		case   't': ch = 'h'; break;
		case   'v': ch = 'l'; break;
		case   'x': ch = 'k'; break;
		case   'r': ch = 'j'; break;
		case   'w': ch = 'y'; break;
		case   'y': ch = 'u'; break;
		case   's': ch = 'n'; break;
		case   'q': ch = 'b'; break;
		case   'u': ch = '.'; break;
	    }

	    if (mode != M_KEYPAD)
	    {
	        unread(ch);
		continue;
	    }
	}

	if (ch == 27)
	{
	    nodelay(win,1);
	    mode = M_ESC;
	    nodelayf = 1;
	    unread(ch);
	    continue;
	}

	switch(ch)
	{
	    case KEY_LEFT   : ch = 'h'; break;
	    case KEY_DOWN   : ch = 'j'; break;
	    case KEY_UP     : ch = 'k'; break;
	    case KEY_RIGHT  : ch = 'l'; break;
	    case KEY_HOME   : ch = 'y'; break;
	    case KEY_PPAGE  : ch = 'u'; break;
	    case KEY_END    : ch = 'b'; break;
#ifdef KEY_LL
	    case KEY_LL	    : ch = 'b'; break;
#endif
	    case KEY_NPAGE  : ch = 'n'; break;

#ifdef KEY_B1
	    case KEY_B1	    : ch = 'h'; break;
	    case KEY_C2     : ch = 'j'; break;
	    case KEY_A2     : ch = 'k'; break;
	    case KEY_B3	    : ch = 'l'; break;
#endif
	    case KEY_A1     : ch = 'y'; break;
	    case KEY_A3     : ch = 'u'; break;
	    case KEY_C1     : ch = 'b'; break;
	    case KEY_C3     : ch = 'n'; break;
            /* next should be '.', but for problem with putty/linux */
	    case KEY_B2	    : ch = 'u'; break;

#ifdef KEY_SLEFT
	    case KEY_SRIGHT  : ch = CTRL('L'); break;
	    case KEY_SLEFT   : ch = CTRL('H'); break;
#ifdef KEY_SUP
	    case KEY_SUP     : ch = CTRL('K'); break;
	    case KEY_SDOWN   : ch = CTRL('J'); break;
#endif
	    case KEY_SHOME   : ch = CTRL('Y'); break;
	    case KEY_SPREVIOUS:ch = CTRL('U'); break;
	    case KEY_SEND    : ch = CTRL('B'); break;
	    case KEY_SNEXT   : ch = CTRL('N'); break;
#endif
	    case 0x146       : ch = CTRL('K'); break; 	/* Shift-Up	*/
	    case 0x145       : ch = CTRL('J'); break; 	/* Shift-Down	*/


#ifdef CTL_RIGHT
	    case CTL_RIGHT   : ch = CTRL('L'); break;
	    case CTL_LEFT    : ch = CTRL('H'); break;
	    case CTL_UP      : ch = CTRL('K'); break;
	    case CTL_DOWN    : ch = CTRL('J'); break;
	    case CTL_HOME    : ch = CTRL('Y'); break;
	    case CTL_PGUP    : ch = CTRL('U'); break;
	    case CTL_END     : ch = CTRL('B'); break;
	    case CTL_PGDN    : ch = CTRL('N'); break;
#endif
#ifdef KEY_EOL
	    case KEY_EOL     : ch = CTRL('B'); break;
#endif

#ifndef CTL_PAD1
	    /* MSYS rxvt console */
	    case 511	     : ch = CTRL('J'); break; /* Shift Dn */
	    case 512         : ch = CTRL('J'); break; /* Ctl Down */
	    case 514	     : ch = CTRL('H'); break; /* Ctl Left */
	    case 516	     : ch = CTRL('L'); break; /* Ctl Right*/
	    case 518	     : ch = CTRL('K'); break; /* Shift Up */
	    case 519	     : ch = CTRL('K'); break; /* Ctl Up   */
#endif

#ifdef CTL_PAD1
	    case CTL_PAD1   : ch = CTRL('B'); break;
	    case CTL_PAD2   : ch = CTRL('J'); break;
	    case CTL_PAD3   : ch = CTRL('N'); break;
	    case CTL_PAD4   : ch = CTRL('H'); break;
	    case CTL_PAD5   : ch = '.'; break;
	    case CTL_PAD6   : ch = CTRL('L'); break;
	    case CTL_PAD7   : ch = CTRL('Y'); break;
	    case CTL_PAD8   : ch = CTRL('K'); break;
	    case CTL_PAD9   : ch = CTRL('U'); break;
#endif

#ifdef ALT_RIGHT
	    case ALT_RIGHT  : ch = CTRL('L'); break;
	    case ALT_LEFT   : ch = CTRL('H'); break;
	    case ALT_DOWN   : ch = CTRL('J'); break;
	    case ALT_HOME   : ch = CTRL('Y'); break;
	    case ALT_PGUP   : ch = CTRL('U'); break;
	    case ALT_END    : ch = CTRL('B'); break;
	    case ALT_PGDN   : ch = CTRL('N'); break;
#endif

#ifdef ALT_PAD1
	    case ALT_PAD1   : ch = CTRL('B'); break;
	    case ALT_PAD2   : ch = CTRL('J'); break;
	    case ALT_PAD3   : ch = CTRL('N'); break;
	    case ALT_PAD4   : ch = CTRL('H'); break;
	    case ALT_PAD5   : ch = '.'; break;
	    case ALT_PAD6   : ch = CTRL('L'); break;
	    case ALT_PAD7   : ch = CTRL('Y'); break;
	    case ALT_PAD8   : ch = CTRL('K'); break;
	    case ALT_PAD9   : ch = CTRL('U'); break;
#endif
#ifdef KEY_BACKSPACE /* NCURSES in Keypad mode sends this for Ctrl-H */
            case KEY_BACKSPACE: ch = CTRL('H'); break;
#endif
	}

	break;
    }

    if (nodelayf)
	nodelay(win,0);

    /*
     * A standalone ESC followed immediately by an ordinary character
     * queues both characters: this call returns ESC and the next call must
     * still receive the ordinary character.  The original unconditional
     * uindex = -1 here discarded that second key.
     *
     * For recognized ESC/keypad sequences, the queued ESC and prefix bytes
     * are decoder state rather than user input, so discard them as before.
     * Do not change ESCDELAY here (that is a separate input-timing change).
     */
    if (mode != M_NORMAL)
        uindex = -1;

    /*
     * Keep UTF-8 bytes intact.  Masking every byte with 0x7f turns Japanese
     * input bytes such as 0xe3 0x81 0x82 into ordinary Rogue commands
     * ('c', '^A', '^B'), which can accidentally open command prompts.
     */
    return jrogue_finish_keycode(ch);
}

#if defined(LOADAV) && defined(HAVE_NLIST_H) && defined(HAVE_NLIST)
/*
 * loadav:
 *	Looking up load average in core (for system where the loadav()
 *	system call isn't defined
 */

#include <nlist.h>

struct nlist avenrun = {
    "_avenrun"
};

void
md_loadav(double *avg)
{
    int kmem;

    if ((kmem = open("/dev/kmem", 0)) < 0)
	goto bad;
    nlist(NAMELIST, &avenrun);
    if (avenrun.n_type == 0)
    {
	close(kmem);
bad:
	avg[0] = 0.0;
	avg[1] = 0.0;
	avg[2] = 0.0;
	return;
    }

    lseek(kmem, avenrun.n_value, 0);
    read(kmem, (char *) avg, 3 * sizeof (double));
    close(kmem);
}
#else
void
md_loadav(double *avg)
{
#if defined(HAVE_LOADAV)
	loadav(avg);
#elif defined(HAVE_GETLOADAVG)
	getloadavg(avg,3);
#else
	avg[0] = avg[1] = avg[2] = 0;
#endif
}
#endif

#ifndef NSIG
#define NSIG 32
#endif

void
md_ignoreallsignals(void)
{
	int i;

	for (i = 0; i < NSIG; i++)
		signal(i, SIG_IGN);
}

void
md_tstphold(void)
{
#ifdef SIGTSTP
    /*
     * If a process can be suspended, this code wouldn't work
     */
# ifdef SIG_HOLD
    signal(SIGTSTP, SIG_HOLD);
# else
    signal(SIGTSTP, SIG_IGN);
# endif
#endif
}

void
md_tstpresume(void)
{
#ifdef SIGTSTP
    signal(SIGTSTP, tstp);
#endif
}

void
md_tstpsignal(void)
{
#ifdef SIGTSTP
    kill(0, SIGTSTP);		/* send actual signal and suspend process */
#endif
}

#if defined(CHECKTIME)
void
md_start_checkout_timer(int time)
{
    int  checkout();

#if defined(HAVE_ALARM) && defined(SIGALRM)
    signal(SIGALRM, checkout);
	alarm(time);
#endif
}

void
md_stop_checkout_timer(void)
{
#if defined(SIGALRM)
    signal(SIGALRM, SIG_IGN);
#endif
}

#endif

