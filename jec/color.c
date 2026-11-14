/*
 * JEC color tables and curses color initialization.
 *
 * JEC color tables, initialization, and drawing wrappers live here.
 * Terminal colors, text attributes, and background switching live here.
 * Display refresh and color-mode transitions live here as well.
 * See LICENSE.TXT for copyright and licensing information.
 */

#include <curses.h>
#include <stdio.h>
#include "rogue.h"
#ifdef _WIN32
#include "windows/rogue_windows_console.h"
#endif

int	ccolor[0x60] =
{
/*      !  "  #  $  %  &  '  (  )  *  +  ,  -  .  /  */
     0,13,13,42,13,48,12,12,13,13,12,47,13,41,11,13,
/*   0  1  2  3  4  5  6  7  8  9  :  ;  <  =  >  ?  */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,13,13,13,13,13,13,
/*   @  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  */
    49,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,
/*   P  Q  R  S  T  U  V  W  X  Y  Z  [  \  ]  ^  _  */
    30,31,32,33,34,35,36,37,38,39,40,13,13,13,14,13,
/*   `  a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  */
    13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*   p  q  r  s  t  u  v  w  x  y  z  {  |  }  ~     */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,13,41,13,13,13

};

/*		          0   1   2   3   4	  5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  */
int fcolor[] = { 0, 1, 2, 3, 4, 5, 6, JROGUE_PAIR_NORMAL, 2, 3, 255, 248, 180, 180, 161, 14, 144, 223, 46, 130, 70, 252, 108, 15, 112, 166, 49, 207, 123, 153, 20, 189, 214, 34, 131, 245, 33, 59, 160, 117, 99, 145, 242, 59, 210, 117, 222, 173, 248, 254, 114, 221};
int hcolor[] = {  0,  1,  1,  0,  1,  1,  1,  0,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1};
int bcolor[] = { -1, -1, -1, -1, -1, -1, -1,  0, -1, -1, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,  0,  0, -1, 17, 17, 17,  0, 17, 17, -1, -1};


static int
jrogue_can_use_true_black_background(void)
{
    return COLORS > 16;
}

const char *
jrogue_black_background_ansi(void)
{
    return jrogue_can_use_true_black_background()
        ? "\033[48;5;16m"
        : "\033[40m";
}

static void
jrogue_use_true_black_background_pairs(void)
{
    int i;

    if (!jrogue_can_use_true_black_background())
        return;

    for (i = 1; i <= JROGUE_PAIR_MAX; i++)
        if (bcolor[i] == COLOR_BLACK || bcolor[i] == -1)
            bcolor[i] = 16;
}


static short
jrogue_platform_color(int color)
{
#ifdef _WIN32
    return jrogue_windows_palette_color(color);
#else
    return (short) color;
#endif
}

void
start_custom_color(void)
{
    int i;

    start_color();
    use_default_colors();

    /*
     * Some terminals display curses/ANSI color 0 as dark gray instead of
     * true black.  Also, cells drawn with the default background (-1) can
     * reveal the terminal profile background instead of the game background.
     * On 256-color terminals, use xterm color 16 for every black/default
     * background cell so color mode remains consistently black.
     */
    jrogue_use_true_black_background_pairs();

#ifdef _WIN32
    /* Exact xterm RGB for all available game colors, including regular
     * text, tombstone, victory art, monsters, items and the blue floor. */
    jrogue_windows_init_palette();
#else
    /* Retain the original macOS/Linux background redefinition unchanged. */
    if (can_change_color() && COLORS > 16)
	init_color(16, 0, 0, 0);
    if (can_change_color() && COLORS > 17)
	init_color(17, 0, 0, 250);
#endif
    for (i = 1; i <= JROGUE_PAIR_MAX; i++)
	init_pair((short)i, jrogue_platform_color(fcolor[i]),
                  jrogue_platform_color(bcolor[i]));

    jrogue_apply_color_background();
}

/* Terminal background state, ANSI setup, and curses text attributes.
 * Keep this logic identical on macOS/Linux and Windows. */
static int color_background_changed = FALSE;


static void
write_ansi(const char *seq)
{
#ifdef _WIN32
    /* Do not mix raw terminal escapes with PDCursesMod's own rendering.
     * Sending raw escapes while PDCurses owns the screen may corrupt
     * its cursor position.  Background/color are set via curses. */
    (void) seq;
#else
    fputs(seq, stdout);
    fflush(stdout);
#endif
}


chtype
jrogue_normal_attr(void)
{
    return COLOR_PAIR(JROGUE_PAIR_NORMAL) | A_NORMAL;
}

void
jrogue_set_normal_attr(void)
{
    if (color_mode_applied != FALSE)
	attrset(jrogue_normal_attr());
    else
	attrset(A_NORMAL);
}

void
jrogue_wset_normal_attr(WINDOW *wn)
{
    if (color_mode_applied != FALSE)
	wattrset(wn, jrogue_normal_attr());
    else
	wattrset(wn, A_NORMAL);
}

void
jrogue_prepare_input_attr(WINDOW *wn)
{
    if (wn != NULL) {
	jrogue_wset_normal_attr(wn);
	wrefresh(wn);
    }
    if (color_mode_applied != FALSE) {
	write_ansi("\033]10;#c0c0c0\007");
	write_ansi("\033]11;#000000\007");
	write_ansi("\033[0m\033[37m");
	write_ansi(jrogue_black_background_ansi());
    }
}

void
jrogue_apply_color_background(void)
{
    if (color_mode) {
        /*
         * Match the color-mode behavior used by the Rogue Clone III macOS
         * port: only while color is enabled, ask terminal emulators that
         * understand OSC 11 to use a black background, force ANSI black
         * background, and make curses erase/blank cells use the normal
         * black-background color pair.
         */
        write_ansi("\033]10;#c0c0c0\007");
        write_ansi("\033]11;#000000\007");
        color_background_changed = TRUE;
        write_ansi("\033[37m");
        write_ansi(jrogue_black_background_ansi());
        bkgd(' ' | jrogue_normal_attr());
        jrogue_set_normal_attr();
    }
    else {
        bkgd(' ' | A_NORMAL);
        attrset(A_NORMAL);
        if (color_background_changed) {
            write_ansi("\033]110\007");
            write_ansi("\033]111\007");
            color_background_changed = FALSE;
        }
        write_ansi("\033[39;49m");
    }
}


/* When temporarily entering a shell, change only the actual terminal
 * colors.  In particular, do not call bkgd(): it changes the attributes
 * stored in the curses screen and would require redrawing the status line. */
void
jrogue_shell_suspend_colors(void)
{
    if (color_background_changed) {
        write_ansi("\033]110\007");
        write_ansi("\033]111\007");
    }
    write_ansi("\033[39;49m");
    /* Keep color_background_changed intact: the game is only suspended. */
}

void
jrogue_shell_resume_colors(void)
{
    if (!color_background_changed)
        return;

    write_ansi("\033]10;#c0c0c0\007");
    write_ansi("\033]11;#000000\007");
    write_ansi("\033[37m");
    write_ansi(jrogue_black_background_ansi());
}

void
restore_terminal_background(void)
{
    if (!isendwin()) {
        bkgd(' ' | A_NORMAL);
        attrset(A_NORMAL);
    }
    if (color_background_changed) {
        write_ansi("\033]110\007");
        write_ansi("\033]111\007");
        color_background_changed = FALSE;
    }
    write_ansi("\033[39;49m");
}

/* Curses wrappers for the JEC color/standout display.
 * Bodies and call ordering are retained from extern.c. */
int mystout = FALSE;

/* PDCursesMod's VT backend does not treat A_STANDOUT as reverse video
 * when using 64-bit chtype: it brightens the background instead.  Use an
 * explicit A_REVERSE for Windows so wizard/detection monsters are inverted.
 * Retain the original standout() semantics on macOS/Linux. */
#ifdef _WIN32
#define JROGUE_STANDOUT_ATTR A_REVERSE
#else
#define JROGUE_STANDOUT_ATTR A_STANDOUT
#endif

static int
jrogue_color_index(chtype c, int *ccp)
{
    int ch;

    ch = (int) CCHAR(c);
    if (ch < 0x20 || ch >= 0x20 + (int)(sizeof ccolor / sizeof ccolor[0]))
	return FALSE;
    *ccp = ccolor[ch - 0x20];
    return TRUE;
}


void
mvaddch_projectile(int y, int x, int ch, int pair)
{
    if (color_mode_applied != FALSE) {
	attrset(COLOR_PAIR(pair) | A_NORMAL);
	mvaddch(y, x, ch);
	jrogue_set_normal_attr();
    }
    else
	jrogue_mvaddch(y, x, ch);
}

int
jrogue_addch(const chtype c)
{
    int cc, rc;

    if (color_mode_applied != FALSE && jrogue_color_index(c, &cc))
    {
	switch (hcolor[cc])
	{
	case 0:
	    attrset(cc == 0 ? jrogue_normal_attr() : (COLOR_PAIR(cc) | A_NORMAL));
	    break;
	case 1:
	    attrset(cc == 0 ? jrogue_normal_attr() : (COLOR_PAIR(cc) | A_NORMAL));
	    break;
	case 2:
	    attrset(COLOR_PAIR(cc) | A_REVERSE | A_BLINK);
	    break;
	case 3:
	    attrset(COLOR_PAIR(cc) | A_REVERSE );
	    break;
	default:
	    attrset(COLOR_PAIR(cc) | A_BLINK);
	    break;
	}
    }
    /* Both color and monochrome drawings must honor explicit reverse
     * video in c; a palette change must not discard monster detection. */
    if (mystout || (c & (A_REVERSE | A_STANDOUT)) != 0)
	attron(JROGUE_STANDOUT_ATTR);
    rc = addch(c);
    if (color_mode_applied == FALSE)
	standend();
    else
	jrogue_set_normal_attr();
    return rc;
}

int
jrogue_waddch(WINDOW *wn, const chtype c)
{
    int cc, rc;

    if (color_mode_applied != FALSE && jrogue_color_index(c, &cc))
    {
	switch (hcolor[cc])
	{
	case 0:
	    wattrset(wn, cc == 0 ? jrogue_normal_attr() : (COLOR_PAIR(cc) | A_NORMAL));
	    break;
	case 1:
	    wattrset(wn, cc == 0 ? jrogue_normal_attr() : (COLOR_PAIR(cc) | A_NORMAL));
	    break;
	case 2:
	    wattrset(wn, COLOR_PAIR(cc) | A_REVERSE | A_BLINK);
	    break;
	case 3:
	    wattrset(wn, COLOR_PAIR(cc) | A_REVERSE );
	    break;
	default:
	    wattrset(wn, COLOR_PAIR(cc) | A_BLINK);
	    break;
	}
    }
    /* Apply highlighting on the destination window, including mono mode.
     * Input attributes alone are not sufficient across curses backends. */
    if (mystout || (c & (A_REVERSE | A_STANDOUT)) != 0)
	wattron(wn, JROGUE_STANDOUT_ATTR);
    rc = waddch(wn, c);
    /* Reset the window we wrote to, not stdscr.  Otherwise a mono
     * wizard map can leak reverse video into subsequent cells. */
    jrogue_wset_normal_attr(wn);
    return rc;
}

int
jrogue_mvaddch(int y, int x, const chtype c)
{
    int cc, rc;

    if (color_mode_applied != FALSE && jrogue_color_index(c, &cc))
    {
	switch (hcolor[cc])
	{
	case 0:
	    attrset(cc == 0 ? jrogue_normal_attr() : (COLOR_PAIR(cc) | A_NORMAL));
	    break;
	case 1:
	    attrset(cc == 0 ? jrogue_normal_attr() : (COLOR_PAIR(cc) | A_NORMAL));
	    break;
	case 2:
	    attrset(COLOR_PAIR(cc) | A_REVERSE | A_BLINK);
	    break;
	case 3:
	    attrset(COLOR_PAIR(cc) | A_REVERSE );
	    break;
	default:
	    attrset(COLOR_PAIR(cc) | A_BLINK);
	    break;
	}
    }
    /* Both color and monochrome drawings must honor explicit reverse
     * video in c; a palette change must not discard monster detection. */
    if (mystout || (c & (A_REVERSE | A_STANDOUT)) != 0)
	attron(JROGUE_STANDOUT_ATTR);
    rc = mvaddch(y, x, c);
    if (color_mode_applied == FALSE)
	standend();
    else
	jrogue_set_normal_attr();
    return rc;
}

int
jrogue_mvwaddch(WINDOW *wn, int y, int x, const chtype c)
{
    int cc, rc;

    if (color_mode_applied != FALSE && jrogue_color_index(c, &cc))
    {
	switch (hcolor[cc])
	{
	case 0:
	    wattrset(wn, cc == 0 ? jrogue_normal_attr() : (COLOR_PAIR(cc) | A_NORMAL));
	    break;
	case 1:
	    wattrset(wn, cc == 0 ? jrogue_normal_attr() : (COLOR_PAIR(cc) | A_NORMAL));
	    break;
	case 2:
	    wattrset(wn, COLOR_PAIR(cc) | A_REVERSE | A_BLINK);
	    break;
	case 3:
	    wattrset(wn, COLOR_PAIR(cc) | A_REVERSE );
	    break;
	default:
	    wattrset(wn, COLOR_PAIR(cc) | A_BLINK);
	    break;
	}
    }
    /* Apply highlighting on the destination window, including mono mode.
     * Input attributes alone are not sufficient across curses backends. */
    if (mystout || (c & (A_REVERSE | A_STANDOUT)) != 0)
	wattron(wn, JROGUE_STANDOUT_ATTR);
    rc = mvwaddch(wn, y, x, c);
    /* Reset the window we wrote to, not stdscr.  Otherwise a mono
     * wizard map can leak reverse video into subsequent cells. */
    jrogue_wset_normal_attr(wn);
    return rc;
}

int
jrogue_standout(void)
{
    int rc;
    
#ifdef _WIN32
    /* A_STANDOUT is not reverse video in PDCursesMod VT color mode. */
    rc = attron(JROGUE_STANDOUT_ATTR);
#else
    rc = standout();
#endif
    mystout = TRUE;
    return rc;
}

int
jrogue_standend(void)
{
    int rc;

    rc = standend();
    if (color_mode_applied != FALSE)
        jrogue_set_normal_attr();
    mystout = FALSE;
    return rc;
}



void
jrogue_refresh_display(void)
{
    int i, j;
    chtype saved_map[(STATLINE - 1) * NUMCOLS];

    /* Capture character and detection/wizard highlighting BEFORE bkgd()
     * changes the color pairs in the live screen.  Recolor the characters
     * using the current palette while retaining reverse/standout semantics. */
    for (j = 1; j < STATLINE; j++)
	for (i = 0; i < NUMCOLS; i++)
	    saved_map[(j - 1) * NUMCOLS + i] = mvinch(j, i);

    color_mode = (color_mode) ? 1 : 0;
    color_mode_applied = color_mode;

    jrogue_apply_color_background();
    jrogue_set_normal_attr();
    /* Row 0 is the message line, not part of the ASCII dungeon map.
     * In particular, the restore filename message may contain Japanese
     * (or other Unicode) characters.  Masking mvinch() with 0x7f and
     * rewriting each cell corrupts them into ASCII/control characters.
     * Leave the message line intact; msg()/endmsg() redraw it as needed. */
    for (i = 0; i < NUMCOLS; i++)
	for (j = 1; j < STATLINE; j++)
	{
	    chtype old = saved_map[(j - 1) * NUMCOLS + i];
	    chtype glyph = (old & 0x7f) |
	                   (old & (A_REVERSE | A_STANDOUT));
	    jrogue_mvaddch(j, i, glyph);
	}

    /*
     * The status line is intentionally not redrawn by the map loop above.
     * When color is switched off, its old color-pair attributes can otherwise
     * remain visible until some numeric status value changes.
     */
    move(STATLINE, 0);
    jrogue_set_normal_attr();
    clrtoeol();
    status();
}
