/* Windows console services for Rogue 5.4.5 JEC.
 * Handles direct-launch console sizing, VT color setup, Unicode input,
 * display corrections and palette mapping.  All platform details remain
 * outside the portable game source files.
 */
#define WIN32_LEAN_AND_MEAN
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <tlhelp32.h>
/* Windows and PDCurses both define MOUSE_MOVED. */
#undef MOUSE_MOVED
#include <curses.h>
#include "rogue.h"
#include "rogue_windows.h"
#include "rogue_windows_console.h"

/* PDCursesMod's VT backend explicitly selects an underline for curs_set(1)
 * and a block on endwin().  Neither is the shape chosen in the user's
 * Windows Terminal / Console Host profile.  DECSCUSR 0 returns to the
 * user-configured shape without guessing whether it is a block or a bar.
 *
 * Only change PDCurses visibility when the desired state actually changes:
 * command() calls jrogue_restore_cursor() for every rendered game turn.
 * Reinitializing curses (restore, -d, shell escape) invalidates this cache.
 *
 * Visibility and cursor shape are separate.  PDCurses may select its own
 * underline again during a later wrefresh() even when visibility has not
 * changed.  An earlier DECSCUSR 0 is therefore insufficient at startup
 * and after shell escape.  Reapply the user's default shape *after* each
 * successful game refresh, without calling curs_set() or toggling visibility.
 */
static int game_cursor_visibility = -1;
static int cursor_resume_pending = 0;

static HANDLE
cursor_vt_output(void)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    const char *term;

    if (out == NULL || out == INVALID_HANDLE_VALUE)
        return NULL;
    if (GetConsoleMode(out, &mode))
    {
        if ((mode & (ENABLE_PROCESSED_OUTPUT |
                     ENABLE_VIRTUAL_TERMINAL_PROCESSING)) ==
                    (ENABLE_PROCESSED_OUTPUT |
                     ENABLE_VIRTUAL_TERMINAL_PROCESSING))
            return out;
        return NULL;
    }
    /* In MSYS2/mintty stdout is a VT pipe rather than a Win32 console.
     * Never emit escapes to regular redirected output files. */
    term = getenv("TERM");
    if (GetFileType(out) == FILE_TYPE_PIPE && getenv("MSYSTEM") != NULL &&
        term != NULL && *term != '\0' && strcmp(term, "dumb") != 0)
        return out;
    return NULL;
}

static void
cursor_write_vt(HANDLE out, const char *seq, DWORD len)
{
    DWORD written;

    fflush(stdout); /* PDCursesMod also writes to the CRT stdout stream. */
    (void)WriteFile(out, seq, len, &written, NULL);
}

/* This changes shape only, not visibility, so it does not re-show a cursor
 * that the game has deliberately hidden.  Sending the same shape is safe:
 * do it after curses has finished writing the updated screen. */
static void
cursor_restore_default_shape(void)
{
    static const char default_shape[] = "\033[0 q";
    HANDLE out = cursor_vt_output();

    if (out != NULL)
        cursor_write_vt(out, default_shape, sizeof default_shape - 1);
}

static void
cursor_resume_vt_processing(void)
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;

    /* endwin() removes VT processing on Windows.  Restore it before any
     * curs_set() or wrefresh() that resumes the game; otherwise PDCurses'
     * escape sequences can be printed literally by Console Host. */
    if (console != NULL && console != INVALID_HANDLE_VALUE &&
        GetConsoleMode(console, &mode))
        (void)SetConsoleMode(console, mode | ENABLE_PROCESSED_OUTPUT |
                                         ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

void
jrogue_windows_set_cursor_visible(int show)
{
    int visible = show ? 1 : 0;
    if (cursor_resume_pending)
        cursor_resume_vt_processing();
    if (game_cursor_visibility == visible)
        return;
    (void)curs_set(visible);  /* Keep PDCurses' own cursor state in sync. */
    game_cursor_visibility = visible;

    /* curs_set(1) selects an underline.  Undo that immediately; the next
     * completed redraw will repeat this as necessary after curses output. */
    if (visible)
        cursor_restore_default_shape();
}

int
jrogue_windows_refresh_window(WINDOW *win)
{
    static const char hide[] = "\033[?25l";
    static const char show[] = "\033[?25h";
    HANDLE out = NULL;
    int result;

    /* The first redraw may also be the first operation after the shell;
     * ensure VT is enabled even when no curs_set() preceded this call. */
    if (cursor_resume_pending)
        cursor_resume_vt_processing();

    /* Rogue 3.6 JEC: the VT renderer can move the physical cursor across
     * the screen while redrawing.  Temporarily hide it for the refresh only.
     * If the cursor option is off, do NOT send ESC[?25h afterward. */
    if (game_cursor_visibility == 1)
        out = cursor_vt_output();
    if (out != NULL)
        cursor_write_vt(out, hide, sizeof hide - 1);
    result = wrefresh(win);
    if (result != ERR)
    {
        /* PDCursesMod's init_color() leaves SP->dirty set during startup.
         * Its first input wait calls napms(), which flushes that pending
         * color repaint and then calls curs_set(1), selecting an underline
         * AFTER our earlier shape restoration.  Drain that repaint here,
         * while the physical cursor is still hidden, before restoring the
         * user's cursor shape.  napms(0) does not introduce a delay and
         * performs no repaint once the dirty flag has been cleared. */
        (void)napms(0);
    }
    if (out != NULL)
        cursor_write_vt(out, show, sizeof show - 1);

    /* A failed redraw must retain the pending shell-resume state so the
     * next successful redraw can synchronize the cursor correctly. */
    if (result == ERR)
        return result;

    if (cursor_resume_pending)
    {
        /* The first refresh after endwin() can restore old visibility.
         * Reapply the game's current option before restoring the shape. */
        if (game_cursor_visibility >= 0)
            (void)curs_set(game_cursor_visibility);
        cursor_resume_pending = 0;
    }

    /* PDCursesMod can reselect an underline during any redraw.  This
     * must remain the LAST cursor operation, even at startup or after
     * returning from the shell; it does not alter visibility. */
    if (game_cursor_visibility == 1)
        cursor_restore_default_shape();
    return result;
}

void
jrogue_windows_restore_terminal_cursor(void)
{
    static const char restore[] = "\033[0 q\033[?25h";
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode, wanted;

    /* PDCursesMod's VT PDC_scr_close() first forces a block cursor and then
     * disables ENABLE_VIRTUAL_TERMINAL_PROCESSING.  Temporarily enable VT
     * again to honor DECSCUSR 0, and restore the original console mode.
     * Without this, Console Host would ignore the cursor-shape reset. */
    if (out != NULL && out != INVALID_HANDLE_VALUE &&
        GetConsoleMode(out, &mode))
    {
        wanted = mode | ENABLE_PROCESSED_OUTPUT |
                        ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if ((wanted == mode || SetConsoleMode(out, wanted)))
        {
            cursor_write_vt(out, restore, sizeof restore - 1);
            if (wanted != mode)
                (void)SetConsoleMode(out, mode);
        }
    }
    else
    {
        /* MSYS2/mintty has a terminal VT pipe, not a Win32 console. */
        out = cursor_vt_output();
        if (out != NULL)
            cursor_write_vt(out, restore, sizeof restore - 1);
    }
    game_cursor_visibility = -1;
    cursor_resume_pending = 1;
}

/*
 * PDCursesMod's VT backend approximates RGB values with xterm's 256-color
 * cube unless PDC_VT contains "RGB".  The floor background is #000040;
 * in that approximation it becomes palette entry 16 (black), so the blue
 * floor disappears even though the curses color pair is correct.
 *
 * Windows 10/11 supports VT true-color output.  Enable it before the first
 * initscr() (including save restores and -d), preserving other PDC_VT flags
 * supplied by the user.  This only changes this process's environment.
 */
void
jrogue_enable_vt_truecolor(void)
{
    const char *settings = getenv("PDC_VT");
    size_t len;
    char *combined;

    if (settings == NULL || *settings == '\0')
    {
        (void)_putenv("PDC_VT=RGB");
        return;
    }
    if (strstr(settings, "RGB") != NULL)
        return;

    len = strlen(settings);
    combined = (char *)malloc((sizeof "PDC_VT=" - 1) + len + sizeof " RGB");
    if (combined == NULL)
        return;
    memcpy(combined, "PDC_VT=", sizeof "PDC_VT=" - 1);
    memcpy(combined + sizeof "PDC_VT=" - 1, settings, len);
    memcpy(combined + sizeof "PDC_VT=" - 1 + len, " RGB", sizeof " RGB");
    (void)_putenv(combined);
    free(combined);
}


/*
 * Direct-launch console sizing for Rogue 5.4.5 JEC.
 * Explorer double-clicks request 80 x 24 cells; command-line launches
 * preserve the existing terminal size. Console Host uses Win32 APIs and
 * Windows Terminal / ConPTY also receives CSI 8 ; 24 ; 80 t.
 */
#define ROGUE_WINDOW_COLS 80
#define ROGUE_WINDOW_ROWS 24

/* 0: detection not yet conclusive, 1: approved, -1: shell/shared. */
static int resize_permission = 0;

static int
is_interactive_shell(const wchar_t *name)
{
    return lstrcmpiW(name, L"cmd.exe") == 0 ||
           lstrcmpiW(name, L"powershell.exe") == 0 ||
           lstrcmpiW(name, L"pwsh.exe") == 0 ||
           lstrcmpiW(name, L"bash.exe") == 0 ||
           lstrcmpiW(name, L"sh.exe") == 0 ||
           lstrcmpiW(name, L"mintty.exe") == 0 ||
           lstrcmpiW(name, L"wsl.exe") == 0 ||
           lstrcmpiW(name, L"zsh.exe") == 0 ||
           lstrcmpiW(name, L"fish.exe") == 0 ||
           lstrcmpiW(name, L"nu.exe") == 0;
}

/* 1 = direct launcher, -1 = shell, 0 = unknown. */
static int
launch_parent_kind(void)
{
    HANDLE snap;
    PROCESSENTRY32W entry;
    DWORD self = GetCurrentProcessId();
    DWORD parent = 0;
    int kind = 0;

    snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;
    entry.dwSize = sizeof entry;
    if (Process32FirstW(snap, &entry))
    {
        do
        {
            if (entry.th32ProcessID == self)
            {
                parent = entry.th32ParentProcessID;
                break;
            }
        } while (Process32NextW(snap, &entry));
    }
    if (parent != 0 && Process32FirstW(snap, &entry))
    {
        do
        {
            const wchar_t *name;
            if (entry.th32ProcessID != parent)
                continue;
            name = entry.szExeFile;
            if (is_interactive_shell(name))
                kind = -1;
            else if (lstrcmpiW(name, L"explorer.exe") == 0 ||
                     lstrcmpiW(name, L"WindowsTerminal.exe") == 0 ||
                     lstrcmpiW(name, L"WindowsTerminalPreview.exe") == 0 ||
                     lstrcmpiW(name, L"OpenConsole.exe") == 0 ||
                     lstrcmpiW(name, L"conhost.exe") == 0 ||
                     lstrcmpiW(name, L"wt.exe") == 0)
                kind = 1;
            break;
        } while (Process32NextW(snap, &entry));
    }
    CloseHandle(snap);

    return kind;
}

static int
only_console_client(void)
{
    DWORD pids[32], count, i;
    DWORD self = GetCurrentProcessId();

    count = GetConsoleProcessList(pids, 32);

    if (count != 1)
        return 0;
    for (i = 0; i < count; ++i)
        if (pids[i] == self)
            return 1;
    return 0;
}

/* 1 if a shell is present; 0 if none; -1 if detection unavailable.
 * Defer inconclusive detection until curses initializes instead of treating
 * a transient ConPTY handoff failure as a permanent refusal. */
static int
console_has_shell(void)
{
    DWORD pids[64], count, i;
    HANDLE snap;
    PROCESSENTRY32W entry;
    int found = 0;

    count = GetConsoleProcessList(pids, 64);
    if (count == 0 || count > 64)
        return -1; /* Unable to verify safety: retry later. */
    snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return -1;
    entry.dwSize = sizeof entry;
    if (Process32FirstW(snap, &entry))
    {
        do
        {
            if (!is_interactive_shell(entry.szExeFile))
                continue;
            for (i = 0; i < count; ++i)
            {
                if (pids[i] == entry.th32ProcessID)
                {
                    found = 1;
                    break;
                }
            }
            if (found)
                break;
        } while (Process32NextW(snap, &entry));
    }
    CloseHandle(snap);

    return found;
}

static SHORT
min_short(SHORT x, SHORT y)
{
    return x < y ? x : y;
}

static SHORT
max_short(SHORT x, SHORT y)
{
    return x > y ? x : y;
}

/* Resize only a newly created, otherwise unused classic Console Host. */
static void
resize_console_host(HANDLE output)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    COORD desired, max_size, expanded;
    SMALL_RECT rect;

    if (!GetConsoleScreenBufferInfo(output, &info))
        return;
    max_size = GetLargestConsoleWindowSize(output);
    if (max_size.X < ROGUE_WINDOW_COLS || max_size.Y < ROGUE_WINDOW_ROWS)
        return;
    desired.X = ROGUE_WINDOW_COLS;
    desired.Y = ROGUE_WINDOW_ROWS;

    /* Shrink the visible area before reducing the screen buffer. */
    rect.Left = 0;
    rect.Top = 0;
    rect.Right = min_short((SHORT)(info.srWindow.Right - info.srWindow.Left + 1),
                           desired.X) - 1;
    rect.Bottom = min_short((SHORT)(info.srWindow.Bottom - info.srWindow.Top + 1),
                            desired.Y) - 1;
    if (!SetConsoleWindowInfo(output, TRUE, &rect))
        return;
    expanded.X = max_short(info.dwSize.X, desired.X);
    expanded.Y = max_short(info.dwSize.Y, desired.Y);
    if (!SetConsoleScreenBufferSize(output, expanded))
        return;
    if (!SetConsoleScreenBufferSize(output, desired))
        return;
    rect.Right = desired.X - 1;
    rect.Bottom = desired.Y - 1;
    (void)SetConsoleWindowInfo(output, TRUE, &rect);
}

static void
resize_pseudoconsole(HANDLE output, DWORD original_mode)
{
    static const char request[] = "\033[8;24;80t";
    DWORD written;
    if (!(original_mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) &&
        !SetConsoleMode(output, original_mode | ENABLE_PROCESSED_OUTPUT |
                        ENABLE_VIRTUAL_TERMINAL_PROCESSING))
        return;
    (void)WriteFile(output, request, (DWORD)(sizeof request - 1),
                    &written, NULL);
}

/* The first call runs before curses starts.  Under a Windows Terminal
 * handoff it may be too early to inspect the console.  A later call from
 * initscr() tries detection again when the first result was inconclusive.
 */
static void
try_initial_resize(void)
{
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    HWND window;
    int parent, sole_client, has_shell, visible;

    if (resize_permission != 0)
        return;
    if (output == NULL || output == INVALID_HANDLE_VALUE ||
        !GetConsoleMode(output, &mode))
    {
        return;
    }
    parent = launch_parent_kind();
    if (parent < 0)
    {
        resize_permission = -1;

        return;
    }
    sole_client = only_console_client();
    has_shell = console_has_shell();
    if (has_shell > 0)
    {
        resize_permission = -1;

        return;
    }
    if (has_shell < 0)
    {
        return;
    }
    if (parent == 0 && !sole_client)
    {
        return;
    }

    resize_permission = 1;
    window = GetConsoleWindow();
    visible = window != NULL && IsWindowVisible(window);

    /* A visible HWND does not reliably distinguish Console Host from
     * ConPTY in all Windows Terminal launch scenarios.  Try the Win32
     * API only on a private visible console, but *always* send the VT
     * request for an approved direct launch, as in Rogue 3.6 v138.
     * Legacy hosts harmlessly ignore this additional VT request. */
    if (visible && sole_client)
        resize_console_host(output);
    resize_pseudoconsole(output, mode);
}

void
jrogue_windows_resize_direct_launch(void)
{
    resize_permission = 0;
    try_initial_resize();
}

void
jrogue_windows_retry_direct_launch_resize(int restoring)
{
    HANDLE output;
    DWORD mode;
    if (resize_permission == -1)
        return;
    if (resize_permission == 0)
    {
        try_initial_resize();
        if (resize_permission != 1)
            return;
    }
    if (restoring)
        Sleep(350);
    output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output == NULL || output == INVALID_HANDLE_VALUE ||
        !GetConsoleMode(output, &mode))
        return;

    resize_pseudoconsole(output, mode);
}

/* The same direct-launch classification is used for the save-start menu.
 * Never show it in an already-running cmd/PowerShell/MSYS2 session. */
int
jrogue_windows_is_direct_launch(void)
{
    int parent = launch_parent_kind();
    int shells;

    if (parent < 0 || resize_permission < 0)
        return 0;
    shells = console_has_shell();
    if (shells != 0) /* Treat an inconclusive shell check conservatively. */
        return 0;
    return resize_permission == 1 || parent == 1 || only_console_client();
}

/* The editor stores UTF-8, while Windows console KEY_EVENT_RECORD contains
 * UTF-16 code units.  Encode a full Unicode scalar, never one surrogate half.
 * This also handles BMP Japanese characters delivered as a single key event. */
int
jrogue_windows_encode_unicode(unsigned int cp, char out[5])
{
    if (cp <= 0x7f)
    {
        out[0] = (char)cp;
        return 1;
    }
    if (cp <= 0x7ff)
    {
        out[0] = (char)(0xc0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3f));
        return 2;
    }
    if (cp >= 0xd800 && cp <= 0xdfff)
        return 0;
    if (cp <= 0xffff)
    {
        out[0] = (char)(0xe0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3f));
        out[2] = (char)(0x80 | (cp & 0x3f));
        return 3;
    }
    if (cp > 0x10ffff)
        return 0;
    out[0] = (char)(0xf0 | (cp >> 18));
    out[1] = (char)(0x80 | ((cp >> 12) & 0x3f));
    out[2] = (char)(0x80 | ((cp >> 6) & 0x3f));
    out[3] = (char)(0x80 | (cp & 0x3f));
    return 4;
}

/*
 * Read an option-editor key directly from Windows console input when it is
 * available.  PDCursesMod wgetch() can truncate U+1F34E to U+F34E; even
 * wget_wch() may lose emoji when a ConPTY/mintty IME supplies the UTF-16
 * high/low surrogates on key *release* events (observed in Rogue 3.6).
 *
 * Prefer characters on KeyDown and take surrogate halves from KeyUp only
 * if the corresponding KeyDown half was empty.  This prevents double entry
 * on terminals that send Unicode on both transitions.
 *
 * Return TRUE if we used the Windows console and wrote a Unicode scalar or
 * curses key code to *result.  Return FALSE for a non-console input handle,
 * where the ordinary curses reader remains in use.
 */
int
jrogue_windows_read_option_key(int *result)
{
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode, count;
    INPUT_RECORD record;
    static WCHAR down_high, down_low;
    unsigned int high = 0;

    if (input == NULL || input == INVALID_HANDLE_VALUE ||
        !GetConsoleMode(input, &mode))
        return FALSE;

    for (;;)
    {
        KEY_EVENT_RECORD *key;
        unsigned int cp;
        int keydown;

        if (!ReadConsoleInputW(input, &record, 1, &count) || count != 1)
        {
            *result = ERR;
            return TRUE;
        }
        if (record.EventType != KEY_EVENT)
            continue;
        key = &record.Event.KeyEvent;
        keydown = (key->bKeyDown != FALSE);
        cp = (unsigned int)key->uChar.UnicodeChar;

        /* The special keys have reliable virtual-key codes even in a
         * Unicode IME session; check them before the character path. */
        if (keydown)
        {
            switch (key->wVirtualKeyCode)
            {
                case VK_LEFT:   *result = KEY_LEFT; return TRUE;
                case VK_RIGHT:  *result = KEY_RIGHT; return TRUE;
                case VK_DELETE: *result = KEY_DC; return TRUE;
                case VK_BACK:   *result = KEY_BACKSPACE; return TRUE;
                case VK_RETURN: *result = '\n'; return TRUE;
                case VK_ESCAPE: *result = ESCAPE; return TRUE;
            }
        }
        if (cp >= 0xd800 && cp <= 0xdbff)
        {
            if (keydown)
                down_high = (WCHAR)cp;
            else if (down_high == (WCHAR)cp)
            {
                down_high = 0;
                continue;
            }
            high = cp;
            continue;
        }
        if (cp >= 0xdc00 && cp <= 0xdfff)
        {
            if (keydown)
                down_low = (WCHAR)cp;
            else if (down_low == (WCHAR)cp)
            {
                down_low = 0;
                continue;
            }
            if (!high)
                continue;
            *result = (int)(0x10000U + ((high - 0xd800U) << 10)
                                  + (cp - 0xdc00U));
            return TRUE;
        }
        if (!keydown || cp == 0)
            continue;
        high = 0;
        *result = (cp == '\r') ? '\n' : (int)cp;
        return TRUE;
    }
}

/*
 * PDCursesMod supports Unicode scalar values in a wide chtype.  Keeping
 * non-BMP characters as four separate bytes in a narrow curses output
 * call can render four mojibake glyphs on some PDCurses configurations.
 * Feed valid UTF-8 four-byte scalars (e.g. fruit emoji) to waddch() as a
 * *single* character; leave ordinary UTF-8 text to the existing routine.
 * The actual emoji glyph still depends on the terminal and its font.
 */
int
jrogue_waddstr_utf8(WINDOW *win, const char *s)
{
    size_t total, used = 0;
    const unsigned char *p;
    char *segment;
    int result = OK;

    if (win == NULL || s == NULL)
        return ERR;
    total = strlen(s);
    segment = (char *)malloc(total + 1);
    if (segment == NULL)
        return waddstr(win, s);

    p = (const unsigned char *)s;
    while (*p)
    {
        unsigned int cp = 0;
        if (p[0] >= 0xf0 && p[0] <= 0xf4 &&
            (size_t)(p - (const unsigned char *)s) + 4 <= total &&
            (p[1] & 0xc0) == 0x80 &&
            (p[2] & 0xc0) == 0x80 &&
            (p[3] & 0xc0) == 0x80)
        {
            cp = ((unsigned int)(p[0] & 7) << 18) |
                 ((unsigned int)(p[1] & 0x3f) << 12) |
                 ((unsigned int)(p[2] & 0x3f) << 6) |
                 (unsigned int)(p[3] & 0x3f);
        }
        if (cp >= 0x10000 && cp <= 0x10ffff)
        {
            if (used)
            {
                segment[used] = '\0';
                if (waddstr(win, segment) == ERR)
                    result = ERR;
                used = 0;
            }
            if (waddch(win, (chtype)cp) == ERR)
                result = ERR;
            p += 4;
        }
        else
            segment[used++] = (char)*p++;
    }
    if (used)
    {
        segment[used] = '\0';
        if (waddstr(win, segment) == ERR)
            result = ERR;
    }
    free(segment);
    return result;
}

/*
 * PDCursesMod's Windows VT backend converts each screen cell to UTF-8,
 * but currently narrows the Unicode scalar to a 16-bit wchar_t first.
 * Supplementary-plane characters (including emoji) therefore display as
 * their low 16 bits.  Keep the VT backend's correct full-width Japanese
 * rendering and repaint only affected option cells using UTF-8 output.
 *
 * The PDCurses window remains the owner of the screen and cursor coordinates.
 */
void
jrogue_windows_option_vt_restore_nonbmp(WINDOW *win)
{
    HANDLE out;
    DWORD written;
    int rows, cols, beg_y, beg_x, cur_y, cur_x, y, x;
    char output[80];

    if (win != hw)
        return;
    out = GetStdHandle(STD_ERROR_HANDLE);
    if (out == NULL || out == INVALID_HANDLE_VALUE)
        return;
    getmaxyx(win, rows, cols);
    getbegyx(win, beg_y, beg_x);
    getyx(win, cur_y, cur_x);
    for (y = 0; y < rows; y++)
    {
        for (x = 0; x < cols; x++)
        {
            unsigned int cp = (unsigned int)(mvwinch(win, y, x) & A_CHARTEXT);
            char utf8[4];
            int n;
            if (cp < 0x10000U || cp > 0x10ffffU || beg_x + x >= COLS - 1)
                continue;
            utf8[0] = (char)(0xf0 | (cp >> 18));
            utf8[1] = (char)(0x80 | ((cp >> 12) & 0x3f));
            utf8[2] = (char)(0x80 | ((cp >> 6) & 0x3f));
            utf8[3] = (char)(0x80 | (cp & 0x3f));
            n = snprintf(output, sizeof output, "\033[%d;%dH%c%c%c%c",
                         beg_y + y + 1, beg_x + x + 1,
                         utf8[0], utf8[1], utf8[2], utf8[3]);
            if (n > 0 && (size_t)n < sizeof output)
                (void)WriteFile(out, output, (DWORD)n, &written, NULL);
        }
    }
    (void)wmove(win, cur_y, cur_x);
    {
        int n = snprintf(output, sizeof output, "\033[%d;%dH",
                         beg_y + cur_y + 1, beg_x + cur_x + 1);
        if (n > 0 && (size_t)n < sizeof output)
            (void)WriteFile(out, output, (DWORD)n, &written, NULL);
    }
}

/*
 * fcolor[] and bcolor[] use ANSI/xterm color numbers.  PDCurses has
 * platform-specific color constants; the Unix build keeps its existing
 * curses mapping and palette unchanged.
 */
static const short jrogue_ansi_to_pdc[16] = {
    COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
    COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE,
    COLOR_BLACK + 8, COLOR_RED + 8, COLOR_GREEN + 8,
    COLOR_YELLOW + 8, COLOR_BLUE + 8, COLOR_MAGENTA + 8,
    COLOR_CYAN + 8, COLOR_WHITE + 8
};

/* Standard xterm/ANSI 16-color reference palette.  For 16-color-only
 * consoles these entries are an approximation of a Mac terminal theme,
 * which may itself customize its first sixteen ANSI colors. */
static const unsigned char jrogue_ansi_rgb[16][3] = {
    {  0,   0,   0}, {128,   0,   0}, {  0, 128,   0}, {128, 128,   0},
    {  0,   0, 128}, {128,   0, 128}, {  0, 128, 128}, {192, 192, 192},
    {128, 128, 128}, {255,   0,   0}, {  0, 255,   0}, {255, 255,   0},
    {  0,   0, 255}, {255,   0, 255}, {  0, 255, 255}, {255, 255, 255}
};

/* Derive the exact xterm 256-color RGB palette rather than relying on
 * the terminal's vendor-specific preset.  The floor background (17) has
 * a deliberate Rogue override of #000040, matching the existing Unix
 * init_color(17, 0, 0, 250) when the Unix terminal accepts it. */
static void
jrogue_xterm_rgb(int index, int *red, int *green, int *blue)
{
    static const unsigned char steps[6] = { 0, 95, 135, 175, 215, 255 };
    int n;

    if (index >= 0 && index < 16)
    {
        *red   = jrogue_ansi_rgb[index][0];
        *green = jrogue_ansi_rgb[index][1];
        *blue  = jrogue_ansi_rgb[index][2];
    }
    else if (index >= 16 && index < 232)
    {
        n = index - 16;
        *red   = steps[n / 36];
        *green = steps[(n / 6) % 6];
        *blue  = steps[n % 6];
    }
    else if (index >= 232 && index < 256)
        *red = *green = *blue = 8 + (index - 232) * 10;
    else
        *red = *green = *blue = 0;

    if (index == 17)
        *blue = 64;
}

/* On older 8/16-color consoles, select the closest available entry
 * rather than attempting to initialize a pair with an invalid index. */
static short
jrogue_nearest_basic_color(int index)
{
    int red, green, blue, best = 0, distance = 0x7fffffff;
    int i, dr, dg, db, d;
    int count = COLORS >= 16 ? 16 : COLORS >= 8 ? 8 : COLORS;

    jrogue_xterm_rgb(index, &red, &green, &blue);
    for (i = 0; i < count; ++i)
    {
        dr = red   - (int)jrogue_ansi_rgb[i][0];
        dg = green - (int)jrogue_ansi_rgb[i][1];
        db = blue  - (int)jrogue_ansi_rgb[i][2];
        d = dr * dr + dg * dg + db * db;
        if (d < distance)
        {
            distance = d;
            best = i;
        }
    }
    return jrogue_ansi_to_pdc[best];
}

void
jrogue_windows_init_palette(void)
{
    int index, red, green, blue;
    int max_index = COLORS < 256 ? COLORS : 256;

    if (!can_change_color())
        return;

    /* Do not insert raw ANSI sequences into the running PDCurses screen.
     * PDCursesMod VT handles the actual console output. */
    for (index = 0; index < max_index; ++index)
    {
        jrogue_xterm_rgb(index, &red, &green, &blue);
        (void)init_color(index < 16 ? jrogue_ansi_to_pdc[index] : (short)index,
                         (short)((red   * 1000 + 127) / 255),
                         (short)((green * 1000 + 127) / 255),
                         (short)((blue  * 1000 + 127) / 255));
    }
}


/* Map the original ANSI/xterm game colors to PDCurses color numbers.
 * Keep the same 8/16-color fallback as the previous extern.c implementation. */
short
jrogue_windows_palette_color(int color)
{
    if (color == -1)
        return -1;
    if (color < 0)
        return COLOR_BLACK;
    if (color < 16)
        return color < COLORS ? jrogue_ansi_to_pdc[color]
                              : jrogue_nearest_basic_color(color);
    if (color >= COLORS)
        return jrogue_nearest_basic_color(color);
    return (short) color;
}
