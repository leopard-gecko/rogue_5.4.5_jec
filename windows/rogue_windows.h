/* Windows-only services for Rogue 5.4.5 JEC.
 * This header intentionally avoids windows.h and curses.h so the game's
 * portable C files need not depend on Win32 types or PDCurses macros.
 */
#ifndef JROGUE_WINDOWS_H
#define JROGUE_WINDOWS_H

#ifdef _WIN32
#include <stdio.h>
#include <wchar.h>
#include <sys/stat.h>

/* Unicode environment options. */
char *jrogue_rogueopts_utf8(void); /* Caller must free the returned string. */

/* Shared direct-launch classification (implemented by the console layer).
 * The launch menu needs it without depending on curses/WINDOW types. */
int jrogue_windows_is_direct_launch(void);

/* Save-file selection and UTF-8 command-line arguments. */
char *jrogue_windows_launch_save(const char *default_save_path);
char *jrogue_windows_argument_utf8(int argument_index);

/* UTF-8 paths for Win32 file operations. */
FILE *jrogue_windows_fopen_utf8(const char *path, const wchar_t *mode);
int jrogue_windows_stat_utf8(const char *path, struct stat *st);
int jrogue_windows_unlink_utf8(const char *path);
int jrogue_windows_chmod_utf8(const char *path, int mode);
#endif /* _WIN32 */

#endif /* JROGUE_WINDOWS_H */
