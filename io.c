/*
 * Various input/output functions
 *
 * @(#)io.c	4.32 (Berkeley) 02/05/99
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <curses.h>
#include <locale.h>
#include <ctype.h>
#include <string.h>
#include "rogue.h"
#include "jec/unicode_text.h"
#ifdef _WIN32
#include "windows/rogue_windows_console.h"
#endif

/* JEC: Unicode-safe message assembly and single-line paging.
 * These buffers and helpers are private to the game's I/O layer.
 * The fixed-size saved-message field and save format remain unchanged. */
struct jrogue_message_buffer {
    char *data;
    size_t length;
    size_t capacity;
};

static int
reserve(struct jrogue_message_buffer *buf, size_t required)
{
    size_t cap;
    char *grown;

    if (required <= buf->capacity)
        return 1;
    cap = buf->capacity ? buf->capacity : 128;
    while (cap < required)
    {
        if (cap > SIZE_MAX / 2)
        {
            cap = required;
            break;
        }
        cap *= 2;
    }
    grown = (char *)realloc(buf->data, cap);
    if (!grown)
        return 0;
    buf->data = grown;
    buf->capacity = cap;
    return 1;
}

static int
jrogue_message_vappend(struct jrogue_message_buffer *buf,
                       const char *fmt, va_list args)
{
    va_list probe;
    int size, actual;
    size_t required;

    if (!buf || !fmt)
        return 0;
    va_copy(probe, args);
    size = vsnprintf(NULL, 0, fmt, probe);
    va_end(probe);
    if (size < 0 || (size_t)size > SIZE_MAX - buf->length - 1)
        return 0;
    required = buf->length + (size_t)size + 1;
    if (!reserve(buf, required))
        return 0;
    actual = vsnprintf(buf->data + buf->length,
                       buf->capacity - buf->length, fmt, args);
    if (actual != size)
    {
        buf->data[buf->length] = '\0';
        return 0;
    }
    buf->length += (size_t)size;
    return 1;
}

static int
jrogue_message_set(struct jrogue_message_buffer *buf, const char *text)
{
    size_t length;
    if (!buf || !text)
        return 0;
    length = strlen(text);
    if (length == SIZE_MAX || !reserve(buf, length + 1))
        return 0;
    /* Current callers copy from a separate message buffer. */
    memcpy(buf->data, text, length + 1);
    buf->length = length;
    return 1;
}

static void
jrogue_message_clear(struct jrogue_message_buffer *buf)
{
    if (!buf)
        return;
    buf->length = 0;
    if (buf->data)
        buf->data[0] = '\0';
}

static size_t
jrogue_message_page_end(const char *text, size_t start,
                        int line_cells, int more_cells)
{
    const char *rest;
    size_t len, at, end, offset;
    size_t last_space = 0;

    if (!text)
        return 0;
    len = strlen(text);
    if (start >= len)
        return len;
    if (line_cells < 1 || more_cells < 1)
        return start;
    if (more_cells > line_cells)
        more_cells = line_cells;
    rest = text + start;
    if (jrogue_text_width(rest) <= line_cells)
        return len;

    end = jrogue_text_prefix_cells(rest, more_cells);
    if (end == 0) /* No progress would result in an endless More loop. */
        return start + jrogue_text_next(rest, 0);

    /* Where possible, start the next page at a word boundary.  Keep the
     * delimiter at the end of the previous page: no original byte is lost.
     * Avoid extremely short pages when a space happens near the start. */
    at = 0;
    while (at < end)
    {
        offset = jrogue_text_next(rest, at);
        if (rest[at] == ' ' && offset <= end)
            last_space = offset;
        at = offset;
    }
    if (last_space && jrogue_text_width_n(rest, last_space)
                      >= more_cells * 2 / 3)
        end = last_space;
    return start + end;
}

/*
 * msg:
 *	Display a message at the top of the screen.
 */
/* Keep the original 80-column top line.  The More marker reserves
 * its own columns on intermediate pages; the final page can use up to
 * column 78 without wrapping at the right edge. */
#define MESSAGE_LINE_CELLS (NUMCOLS - 1)

static struct jrogue_message_buffer msgbuf = { NULL, 0, 0 };
static struct jrogue_message_buffer full_history = { NULL, 0, 0 };
static char history_saved_prefix[MAXSTR];

/* The save file still contains the original fixed-size huh[] field.
 * A longer message can be replayed in full during this session; after
 * restoring an old/new save, the serialized prefix is used instead. */
const char *
jrogue_last_message(void)
{
    if (full_history.data && !strcmp(history_saved_prefix, huh))
        return full_history.data;
    return huh;
}

void
jrogue_message_forget_history(void)
{
    jrogue_message_clear(&full_history);
    history_saved_prefix[0] = '\0';
}

/* Serializing the entire live message would change the save format.
 * Keep a complete UTF-8 grapheme prefix in the legacy huh[] field. */
static void
remember_message(const char *full)
{
    size_t keep = jrogue_text_prefix(full, MAXSTR - 1);
    if (save_msg)
    {
        if (!jrogue_message_set(&full_history, full))
            jrogue_message_clear(&full_history);
        memcpy(huh, full, keep);
        huh[keep] = '\0';
        memcpy(history_saved_prefix, huh, keep + 1);
    }
}

/* Display a More prompt without running beyond the 80-column row.
 * For a previous, nearly full line, show the prompt on a cleared row
 * rather than overwriting its trailing text or wrapping into the map. */
static int
message_more(int at, int redraw_map)
{
    const char *prompt = JSTR(" --続く--", " --More--");
    int ch;
    int limit = NUMCOLS - 1 - jrogue_text_width(prompt);

    jrogue_restore_cursor();
    if (redraw_map)
        look(FALSE);
    jrogue_set_normal_attr();
    if (at > limit)
    {
        move(0, 0);
        clrtoeol();
        at = 0;
    }
#ifdef _WIN32
    move(0, at);
    jrogue_waddstr_utf8(stdscr, prompt);
#else
    mvaddstr(0, at, prompt);
#endif
    jrogue_show_cursor(TRUE);
    refresh();
    if (!msg_esc)
        wait_for(stdscr, ' ');
    else
    {
        while ((ch = readchar()) != ' ')
            if (ch == ESCAPE)
            {
                jrogue_restore_cursor();
                return ESCAPE;
            }
    }
    jrogue_restore_cursor();
    return ~ESCAPE;
}

/* VARARGS1 */
int
msg(const char *fmt, ...)
{
    va_list args;

    /*
     * if the string is "", just clear the line
     */
    if (*fmt == '\0')
    {
	jrogue_set_normal_attr();
	move(0, 0);
	clrtoeol();
	mpos = 0;
	return ~ESCAPE;
    }
    /*
     * otherwise add to the message and flush it out
     */
    va_start(args, fmt);
    doadd(fmt, args);
    va_end(args);
    return endmsg();
}

/*
 * addmsg:
 *	Add things to the current message
 */
/* VARARGS1 */
void
addmsg(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    doadd(fmt, args);
    va_end(args);
}

/*
 * endmsg:
 * Render every byte of a logical message, splitting only at Unicode
 * grapheme boundaries if the top line would otherwise wrap.  The existing
 * --More-- prompt serves both previous-message and intra-message paging.
 */
int
endmsg(void)
{
    char empty[1] = { 0 };
    char *full = msgbuf.data ? msgbuf.data : empty;
    size_t start = 0, end, length;
    int cells, marker_cells, more_cells;

    length = strlen(full);
    /* Original English capitalization applies to the logical message,
     * not to the start of each page. */
    if (!jrogue_lang_is_japanese &&
        islower((unsigned char)full[0]) && !lower_msg && full[1] != ')')
        full[0] = (char)toupper((unsigned char)full[0]);

    if (mpos && message_more(mpos, TRUE) == ESCAPE)
        goto cancelled;

    /* Preserve an exact live Ctrl-P replay, independently of pages. */
    remember_message(full);
    marker_cells = jrogue_text_width(JSTR(" --続く--", " --More--"));
    more_cells = MESSAGE_LINE_CELLS - marker_cells;
    if (more_cells < 1)
        more_cells = 1;

    do
    {
        int more;
        char saved;
        end = jrogue_message_page_end(full, start,
                                      MESSAGE_LINE_CELLS, more_cells);
        if (end <= start && start < length) /* Defensive progress guard. */
            end = jrogue_text_next(full, start);
        more = end < length;
        saved = full[end];
        full[end] = '\0';
        cells = jrogue_text_width(full + start);
        jrogue_set_normal_attr();
        move(0, 0);
#ifdef _WIN32
        jrogue_waddstr_utf8(stdscr, full + start);
#else
        addstr(full + start);
#endif
        clrtoeol();
        full[end] = saved;
        mpos = cells;
        jrogue_restore_cursor();
        refresh();
        if (!more)
            break;
        if (message_more(mpos, FALSE) == ESCAPE)
            goto cancelled;
        start = end;
    } while (1);

    jrogue_message_clear(&msgbuf);
    return ~ESCAPE;

cancelled:
    jrogue_message_clear(&msgbuf);
    mpos = 0;
    return ESCAPE;
}

/*
 * doadd:
 * Assemble one complete logical message.  The former MAXMSG auto-flush
 * could lose an addmsg() fragment or split it in the middle of a UTF-8
 * grapheme; paging must happen only when endmsg() is called.
 */
void
doadd(const char *fmt, va_list args)
{
    (void)jrogue_message_vappend(&msgbuf, fmt, args);
}

/*
 * step_ok:
 *	Returns true if it is ok to step on ch
 */
int
step_ok(int ch)
{
    switch (ch)
    {
	case ' ':
	case '|':
	case '-':
	    return FALSE;
	default:
	    return (!isalpha(ch));
    }
}


static int
utf8_expected_len(int ch)
{
    unsigned char c = (unsigned char) ch;

    if (c < 0x80)
	return 1;
    if ((c & 0xe0) == 0xc0)
	return 2;
    if ((c & 0xf0) == 0xe0)
	return 3;
    if ((c & 0xf8) == 0xf0)
	return 4;
    return 1;
}

static int
read_command_char(WINDOW *win)
{
    int ch, i, len, next;

    jrogue_prepare_input_attr(win);
    ch = md_readchar(win);

    if (ch >= 0x80 && ch <= 0xff)
    {
	len = utf8_expected_len(ch);
	for (i = 1; i < len; i++)
	{
	    next = md_readchar(win);
	    if (!(next >= 0x80 && next <= 0xff)
	    || (((unsigned char)next & 0xc0) != 0x80))
		break;
	}
	return INVALID_INPUT;
    }
    return ch;
}


/*
 * readchar:
 *	Reads and returns a character, checking for gross input errors
 */

int
readchar(void)
{
    int ch;

    ch = read_command_char(stdscr);

    if (ch == 3)
    {
		quit(0);
        return(27);
    }

    return(ch);
}

int
wreadchar(WINDOW *win)
{
    int ch;

    ch = read_command_char(win);

    if (ch == 3)
    {
		quit(0);
        return(27);
    }

    return(ch);
}


/*
 * status_format:
 *	Build a status line using progressively more compact, but legible,
 *	formats.  The original RRP layout is tried first.  In Japanese mode
 *	we keep Japanese labels (not "HP") and preserve "体力" and "経験"
 *	as long as possible.  No game state, timing, or random numbers change.
 *
 *	The top-row '@' message reserves room for " --More--" / " --続く--";
 *	the bottom status row can use almost the full 80-column map width.
 */
#define STATUS_MSG_CELLS 70
#define STATUS_LINE_CELLS (NUMCOLS - 1)

static const char *const status_formats_ja[] = {
    /* Original JEC format. */
    "階: %d  金塊: %-5d  体力: %*d(%*d)  強さ: %2d(%d)  守備: %-2d  経験: %d/%d  %s",
    /* 1: Reduce separation between the fields. */
    "階: %d 金塊: %-5d 体力: %*d(%*d) 強さ: %2d(%d) 守備: %-2d 経験: %d/%d %s",
    /* 2: Omit blanks after colons and number-alignment padding. */
    "階:%d 金塊:%d 体力:%d(%d) 強さ:%d(%d) 守備:%d 経験:%d/%d %s",
    /* 3: Shorten only 金塊 to 金. */
    "階:%d 金:%d 体力:%d(%d) 強さ:%d(%d) 守備:%d 経験:%d/%d %s",
    /* 4: Shorten 強さ and 守備, retain 体力 and 経験. */
    "階:%d 金:%d 体力:%d/%d 力:%d/%d 守:%d 経験:%d/%d %s",
    /* Last resort before clipping: remove punctuation/extra spaces. */
    "階%d 金%d 体力%d/%d 力%d/%d 守%d 経験%d/%d %s"
};

static const char *const status_formats_en[] = {
    "Level: %d  Gold: %-5d  Hp: %*d(%*d)  Str: %2d(%d)  Arm: %-2d  Exp: %d/%d  %s",
    "Level: %d Gold: %-5d Hp: %*d(%*d) Str: %2d(%d) Arm: %-2d Exp: %d/%d %s",
    "Level:%d Gold:%d Hp:%d(%d) Str:%d(%d) Arm:%d Exp:%d/%d %s",
    "Lv:%d Gold:%d Hp:%d(%d) Str:%d(%d) Arm:%d Exp:%d/%d %s",
    "Lv:%d Gold:%d Hp:%d/%d Str:%d/%d Arm:%d Exp:%d/%d %s",
    "Lv%d G%d Hp%d/%d Str%d/%d A%d Exp%d/%d %s"
};

/* This is only a display safeguard for pathological numeric values. */
static void
status_clip_line(char *line, int cells)
{
    size_t i = 0, keep = 0, last_field_end = 0, next;

    if (mbstrlen(line) <= cells)
        return;

    /* Reserve one cell for the ellipsis; never split a UTF-8 grapheme.
     * Prefer a complete field over showing part of a numeric value. */
    while (line[i] != '\0')
    {
        next = jrogue_text_next(line, i);
        if (next <= i || jrogue_text_width_n(line, next) > cells - 1)
            break;
        if (line[i] == ' ' && i > 0)
            last_field_end = i;
        keep = next;
        i = next;
    }
    if (last_field_end != 0)
        keep = last_field_end;
    line[keep] = '\0';
    jrogue_strlcat(line, "…", MAXSTR);
}

static void
status_format_line(char *line, size_t line_size, int cells, int hpwidth,
                   int armor, const char *hunger)
{
    const char *const *formats = jrogue_lang_is_japanese
        ? status_formats_ja : status_formats_en;
    size_t i, count = sizeof(status_formats_ja) / sizeof(status_formats_ja[0]);

    for (i = 0; i < count; ++i)
    {
        if (i < 2)
            snprintf(line, line_size, formats[i],
                level, purse, hpwidth, pstats.s_hpt, hpwidth, max_hp,
                pstats.s_str, max_stats.s_str, armor,
                pstats.s_lvl, pstats.s_exp, hunger);
        else
            snprintf(line, line_size, formats[i],
                level, purse, pstats.s_hpt, max_hp,
                pstats.s_str, max_stats.s_str, armor,
                pstats.s_lvl, pstats.s_exp, hunger);
        if (mbstrlen(line) <= cells)
            return;
    }

    status_clip_line(line, cells);
}

/*
 * status:
 *	Display the important stats line.  Keep the cursor where it was.
 *	Use a narrower line for the '@' message, so --More-- stays on row 0.
 */
void
status(void)
{
    int oy, ox, temp;
    int hunger;
    char line[MAXSTR];
    static int hpwidth = 0;
    static int s_hungry = 0;
    static int s_lvl = 0;
    static int s_pur = -1;
    static int s_hp = 0;
    static int s_maxhp = -1;
    static int s_maxstr = -1;
    static int s_explvl = -1;
    static int s_arm = 0;
    static int s_str = 0;
    static int s_exp = 0;
    static int s_color_mode = -1;
    static int s_lang_is_japanese = -1;
    const char *state_name[] =
    {"", JSTR("空腹", "Hungry"), JSTR("飢餓", "Weak"), JSTR("瀕死", "Faint")};

    temp = (cur_armor != NULL ? cur_armor->o_arm : pstats.s_arm);
    if (s_hp == pstats.s_hpt && s_exp == pstats.s_exp && s_pur == purse
        && s_arm == temp && s_str == pstats.s_str && s_lvl == level
        && s_maxhp == max_hp && s_maxstr == max_stats.s_str
        && s_explvl == pstats.s_lvl && s_hungry == hungry_state
        && s_color_mode == color_mode_applied
        && s_lang_is_japanese == jrogue_lang_is_japanese
        && !stat_msg)
        return;

    getyx(stdscr, oy, ox);
    if (s_maxhp != max_hp)
    {
        int value = max_hp;
        for (hpwidth = 0; value; ++hpwidth)
            value /= 10;
        if (hpwidth == 0)
            hpwidth = 1;
    }

    s_lvl = level;
    s_pur = purse;
    s_hp = pstats.s_hpt;
    s_maxhp = max_hp;
    s_maxstr = max_stats.s_str;
    s_explvl = pstats.s_lvl;
    s_arm = temp;
    s_str = pstats.s_str;
    s_exp = pstats.s_exp;
    s_hungry = hungry_state;
    s_color_mode = color_mode_applied;
    s_lang_is_japanese = jrogue_lang_is_japanese;

    hunger = hungry_state;
    if (hunger < 0 || hunger > 3)
        hunger = 0;

    status_format_line(line, sizeof line,
        stat_msg ? STATUS_MSG_CELLS : STATUS_LINE_CELLS,
        hpwidth, 10 - s_arm, state_name[hunger]);

    jrogue_set_normal_attr();
    if (stat_msg)
    {
        /* msg() already clears the entire top row. */
        msg("%s", line);
    }
    else
    {
        move(STATLINE, 0);
#ifdef _WIN32
        jrogue_waddstr_utf8(stdscr, line);
#else
        addstr(line);
#endif
        clrtoeol();
    }
    move(oy, ox);
}

/*
 * wait_for
 *	Sit around until the guy types the right key
 */
void
wait_for(WINDOW *win, int ch)
{
    int c;

    if (ch == '\n')
        while ((c = wreadchar(win)) != '\n' && c != '\r')
	    continue;
    else
        while (wreadchar(win) != ch)
	    continue;
}

/*
 * show_win:
 *	Function used to display a window and wait before returning
 */
void
show_win(const char *message)
{
    WINDOW *win;

    win = hw;
    jrogue_wset_normal_attr(win);
    wmove(win, 0, 0);
    waddstr(win, message);
    touchwin(win);
    wrefresh(win);
    wait_for(win, ' ');
    clearok(curscr, TRUE);
    touchwin(stdscr);
}
