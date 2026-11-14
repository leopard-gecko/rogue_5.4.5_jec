/*
 * JEC shared color tables and color initialization.
 * See LICENSE.TXT for copyright and licensing information.
 */
#ifndef JROGUE_COLOR_H
#define JROGUE_COLOR_H

#define JROGUE_PAIR_NORMAL 7
#define JROGUE_PAIR_PROJECTILE_RED 44
#define JROGUE_PAIR_PROJECTILE_CYAN 45
#define JROGUE_PAIR_PROJECTILE_YELLOW 46
#define JROGUE_PAIR_WIN_GREEN 50
#define JROGUE_PAIR_WIN_YELLOW 51
#define JROGUE_PAIR_MAX 51

extern int ccolor[0x60];
extern int fcolor[];
extern int bcolor[];
extern int hcolor[];

/* JEC color/standout drawing wrappers (stage 2).
 * Include after curses has declared chtype and WINDOW. */
extern int mystout;

void mvaddch_projectile(int y, int x, int ch, int pair);
int jrogue_addch(const chtype c);
int jrogue_waddch(WINDOW *wn, const chtype c);
int jrogue_mvaddch(int y, int x, const chtype c);
int jrogue_mvwaddch(WINDOW *wn, int y, int x, const chtype c);
int jrogue_standout(void);
int jrogue_standend(void);

void jrogue_refresh_display(void);
void start_custom_color(void);
const char *jrogue_black_background_ansi(void);

/* Shared curses attributes and terminal background lifecycle (stage 3).
 * color_mode and color_mode_applied remain game settings defined in extern.c.
 * Include this header only after curses has defined chtype and WINDOW. */
chtype jrogue_normal_attr(void);
void jrogue_set_normal_attr(void);
void jrogue_wset_normal_attr(WINDOW *wn);
void jrogue_prepare_input_attr(WINDOW *wn);
void jrogue_apply_color_background(void);
void restore_terminal_background(void);
void jrogue_shell_suspend_colors(void);
void jrogue_shell_resume_colors(void);

#endif /* JROGUE_COLOR_H */
