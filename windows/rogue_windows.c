/* Windows file and launch services for Rogue 5.4.5 JEC.
 *
 * - Unicode ROGUEOPTS and command-line arguments.
 * - Save selection and UTF-8 paths through UTF-16 Win32/UCRT APIs.
 *
 * Console setup, sizing and direct-launch detection are implemented in
 * rogue_windows_console.c.  Makefile.win builds both files for Windows only.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <conio.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "jec/language.h"
#include "rogue_windows.h"

/* Unicode environment options (formerly in main.c). */
/*
 * The Windows environment is Unicode (UTF-16).  The narrow CRT getenv()
 * view may use the system ANSI code page rather than UTF-8, corrupting
 * Japanese names in ROGUEOPTS.  Read the Unicode value and convert it
 * explicitly before passing it to the existing UTF-8 option parser.
 *
 * The returned buffer is writable because parse_opts() temporarily
 * changes delimiters while parsing the language setting.  Callers free it.
 */
char *
jrogue_rogueopts_utf8(void)
{
    DWORD needed = GetEnvironmentVariableW(L"ROGUEOPTS", NULL, 0);
    DWORD got;
    wchar_t *wide;
    char *utf8;
    int bytes;

    if (needed == 0 || needed > 32767)
        return NULL;
    wide = (wchar_t *)malloc((size_t)needed * sizeof(*wide));
    if (wide == NULL)
        return NULL;
    got = GetEnvironmentVariableW(L"ROGUEOPTS", wide, needed);
    if (got == 0 || got >= needed)
    {
        free(wide);
        return NULL;
    }
    bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                wide, -1, NULL, 0, NULL, NULL);
    if (bytes <= 0)
    {
        free(wide);
        return NULL;
    }
    utf8 = (char *)malloc((size_t)bytes);
    if (utf8 != NULL &&
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                            wide, -1, utf8, bytes, NULL, NULL) != bytes)
    {
        free(utf8);
        utf8 = NULL;
    }
    free(wide);
    return utf8;
}


static wchar_t *
utf8_to_wide(const char *text)
{
    wchar_t *out;
    int chars;
    if (text == NULL)
        return NULL;
    chars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                text, -1, NULL, 0);
    if (chars <= 0)
        return NULL;
    out = malloc((size_t)chars * sizeof *out);
    if (out != NULL && MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                         text, -1, out, chars) != chars)
    {
        free(out);
        out = NULL;
    }
    return out;
}

static char *
wide_to_utf8(const wchar_t *text)
{
    char *out;
    int bytes;
    if (text == NULL)
        return NULL;
    bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                text, -1, NULL, 0, NULL, NULL);
    if (bytes <= 0)
        return NULL;
    out = malloc((size_t)bytes);
    if (out != NULL && WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                           text, -1, out, bytes, NULL, NULL) != bytes)
    {
        free(out);
        out = NULL;
    }
    return out;
}

/* Use Unicode Windows command line for dragged filenames. MinGW's main()
 * argv can be encoded in the Windows ANSI code page, losing Japanese text. */
char *
jrogue_windows_argument_utf8(int argument_index)
{
    int count = 0;
    LPWSTR *argv_w = CommandLineToArgvW(GetCommandLineW(), &count);
    char *value = NULL;
    if (argv_w != NULL)
    {
        if (argument_index >= 1 && argument_index < count)
            value = wide_to_utf8(argv_w[argument_index]);
        LocalFree(argv_w);
    }
    return value;
}

/* Check the actual startup save path (including FILEDIR and ROGUEOPTS).
 * A file-picker or the start menu must never change the working directory. */
static int
file_exists_utf8(const char *path)
{
    DWORD flags;
    wchar_t *wide = utf8_to_wide(path);
    if (wide == NULL)
        return 0;
    flags = GetFileAttributesW(wide);
    free(wide);
    return flags != INVALID_FILE_ATTRIBUTES &&
           !(flags & FILE_ATTRIBUTE_DIRECTORY);
}

static char *
choose_save_file(void)
{
    OPENFILENAMEW dlg;
    wchar_t *buffer;
    char *path = NULL;
    static const wchar_t filters[] =
        L"Saved games (*.save)\0*.save\0All files (*.*)\0*.*\0";

    buffer = calloc(32768, sizeof *buffer);
    if (buffer == NULL)
        return NULL;
    memset(&dlg, 0, sizeof dlg);
    dlg.lStructSize = sizeof dlg;
    dlg.lpstrFilter = filters;
    dlg.lpstrFile = buffer;
    dlg.nMaxFile = 32768;
    dlg.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
                OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
    dlg.lpstrTitle = jrogue_lang_is_japanese ?
        L"セーブファイルを選択" : L"Select a saved game";
    if (GetOpenFileNameW(&dlg))
        path = wide_to_utf8(buffer);
    free(buffer);
    return path;
}

/* Handle only direct starts without a positional argument. No default save:
 * start a new game immediately, as Rogue 3.6 v157 does. */
char *
jrogue_windows_launch_save(const char *default_save_path)
{
    int c;
    char *selection;
    if (!jrogue_windows_is_direct_launch() ||
        !file_exists_utf8(default_save_path))
        return NULL;

    for (;;)
    {
        /* Match the Rogue 3.6 v159 launch menu wording and key layout. */
        printf("\n                 Rogue 5.4.5\n\n");
        printf("%s\n\n", JSTR("セーブした冒険が見つかりました。",
                              "A saved adventure was found."));
        printf("[R/Enter] %s\n", JSTR("セーブした冒険を再開する",
                                      "Resume saved adventure"));
        printf("[N]       %s\n", JSTR("新しい冒険を始める",
                                      "Start a new adventure"));
        printf("[F]       %s\n", JSTR("別のセーブファイルを選ぶ",
                                      "Choose another save file"));
        printf("[Q]       %s\n\n", JSTR("終了する", "Quit"));
        fputs(JSTR("キーを押してください: ", "Press a key: "), stdout);
        fflush(stdout);
        c = _getwch();
        if (c == 0 || c == 0xe0)
        {
            (void)_getwch(); /* Ignore extended function keys. */
            continue;
        }
        putchar('\n');
        if (c == 'r' || c == 'R' || c == '\r' || c == '\n')
            return _strdup(default_save_path);
        if (c == 'n' || c == 'N')
            return NULL;
        if (c == 'q' || c == 'Q' || c == 27)
            exit(0);
        if (c == 'f' || c == 'F')
        {
            selection = choose_save_file();
            if (selection != NULL)
                return selection;
            /* Cancelled picker: offer the start menu again. */
        }
    }
}

FILE *
jrogue_windows_fopen_utf8(const char *path, const wchar_t *mode)
{
    wchar_t *wide = utf8_to_wide(path);
    FILE *stream = NULL;
    if (wide != NULL)
    {
        stream = _wfopen(wide, mode);
        free(wide);
    }
    return stream;
}

/* A portable subset of struct stat used in save.c's integrity checks. */
int
jrogue_windows_stat_utf8(const char *path, struct stat *st)
{
    wchar_t *wide = utf8_to_wide(path);
    struct _stat64 found;
    int result = -1;
    if (wide != NULL)
    {
        if (_wstat64(wide, &found) == 0)
        {
            memset(st, 0, sizeof *st);
            st->st_mode = found.st_mode;
            st->st_nlink = found.st_nlink;
            st->st_mtime = (time_t)found.st_mtime;
            result = 0;
        }
        free(wide);
    }
    return result;
}

int
jrogue_windows_unlink_utf8(const char *path)
{
    wchar_t *wide = utf8_to_wide(path);
    int result = -1;
    if (wide != NULL)
    {
        result = _wunlink(wide);
        free(wide);
    }
    return result;
}

int
jrogue_windows_chmod_utf8(const char *path, int mode)
{
    wchar_t *wide = utf8_to_wide(path);
    int result = -1;
    if (wide != NULL)
    {
        result = _wchmod(wide, mode);
        free(wide);
    }
    return result;
}
