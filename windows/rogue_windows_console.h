/* Windows-only console lifecycle, input, Unicode rendering and palette.
 * Include after <curses.h>: display helpers accept WINDOW pointers.
 * Windows API types remain private to the implementation.
 */
#ifndef JROGUE_WINDOWS_CONSOLE_H
#define JROGUE_WINDOWS_CONSOLE_H

#ifdef _WIN32
/* Before initscr(): enable VT true color and resize direct launches.
 * Shared launch classification is declared in rogue_windows.h. */
void jrogue_enable_vt_truecolor(void);

/* PDCursesMod uses curs_set(1) for an underline and endwin() may force
 * a block.  Track visibility through curses, but restore the terminal's
 * default cursor shape (DECSCUSR 0) after drawing and on shell exit. */
void jrogue_windows_set_cursor_visible(int show);
void jrogue_windows_restore_terminal_cursor(void);
/* As in 3.6 JEC, hide the physical cursor only while refreshing the screen;
 * never change PDCurses' internal visibility state during a redraw. */
int jrogue_windows_refresh_window(WINDOW *win);
void jrogue_windows_resize_direct_launch(void);
void jrogue_windows_retry_direct_launch_resize(int restoring);

/* Returns nonzero when the Windows console handled the input; otherwise
 * the option editor should fall back to its usual curses input path. */
int jrogue_windows_read_option_key(int *result);

/* Encodes one Unicode scalar as UTF-8; returns the number of bytes, or 0
 * for an invalid scalar.  The caller supplies at least five bytes. */
int jrogue_windows_encode_unicode(unsigned int cp, char out[5]);

/* Windows PDCurses color palette and ANSI/xterm color-number mapping.
 * Call the initialization only after start_color() / use_default_colors(). */
void jrogue_windows_init_palette(void);
short jrogue_windows_palette_color(int color);

/* Draw UTF-8 text through PDCursesMod without splitting non-BMP scalars. */
int jrogue_waddstr_utf8(WINDOW *win, const char *text);

/* Repaint supplementary-plane characters after a normal curses refresh. */
void jrogue_windows_option_vt_restore_nonbmp(WINDOW *win);
#endif

#endif /* JROGUE_WINDOWS_CONSOLE_H */
