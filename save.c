/*
 * save and restore routines
 *
 * @(#)save.c	4.33 (Berkeley) 06/01/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <curses.h>
#include <locale.h>
#include "rogue.h"
#include "score.h"
#include "jec/unicode_text.h"
#ifdef _WIN32
#include "windows/rogue_windows.h"
#include "windows/rogue_windows_console.h"
#endif

typedef struct stat STAT;
static STAT sbuf;

/* Keep the save confirmation on one line.  Prefer the basename when
 * the full path is too long, then shorten the displayed name at complete
 * Unicode grapheme boundaries and append "..." if necessary.  The actual
 * save path is never changed. */
static void
confirm_save_name(const char *path)
{
    const char *display = path;
    const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');
    const char *before = JSTR("ファイル名（", "save file (");
    const char *after = JSTR("）でセーブしますか？", ")?");
    char prompt[MAXSTR];
    int line_cells = (_COLS < NUMCOLS ? _COLS : NUMCOLS) - 1;
    int name_cells = line_cells - mbstrlen(before) - mbstrlen(after);

    if (name_cells < 0)
    {
        msg(JSTR("このファイルにセーブしますか？", "Save to the current file?"));
        return;
    }
    if (mbstrlen(path) > name_cells)
    {
        if (slash != NULL && (backslash == NULL || slash > backslash))
            display = slash + 1;
        else if (backslash != NULL)
            display = backslash + 1;
    }

    prompt[0] = '\0';
    jrogue_text_append_labeled(prompt, sizeof prompt, line_cells,
                               before, display, after);
    msg("%s", prompt);
}

/* Sanitize the filename *only for the terminal*.  Keep the original
 * pathname byte-for-byte for subsequent saves.  In particular, do not
 * let a corrupt UTF-8 byte or terminal control escape into curses. */
static void
sanitize_restored_basename(const char *base, char *out, size_t capacity)
{
    const unsigned char *p = (const unsigned char *)base;
    size_t used = 0;

    if (capacity == 0)
        return;
    while (*p && used + 1 < capacity)
    {
        int count = 1, i, valid = 1;

        if (*p >= 0xc2 && *p <= 0xdf)
            count = 2;
        else if (*p >= 0xe0 && *p <= 0xef)
            count = 3;
        else if (*p >= 0xf0 && *p <= 0xf4)
            count = 4;
        else if (*p >= 0x80)
            valid = 0;
        for (i = 1; i < count; ++i)
            if (p[i] == '\0' || (p[i] & 0xc0) != 0x80)
            {
                valid = 0;
                break;
            }
        if (valid && count > 1 &&
            ((p[0] == 0xe0 && p[1] < 0xa0) ||
             (p[0] == 0xed && p[1] >= 0xa0) ||
             (p[0] == 0xf0 && p[1] < 0x90) ||
             (p[0] == 0xf4 && p[1] >= 0x90)))
            valid = 0;

        if (!valid || *p < 0x20 || *p == 0x7f ||
            (count == 2 && p[0] == 0xc2 && p[1] < 0xa0))
        {
            out[used++] = '?';
            p += valid ? count : 1;
        }
        else
        {
            if (used + (size_t)count >= capacity)
                break;
            memcpy(out + used, p, (size_t)count);
            used += (size_t)count;
            p += count;
        }
    }
    out[used] = '\0';
}

/* Restore the original 5.4 filename notice without displaying a long path.
 * Shortening is display-only.  The common text formatter clips at complete
 * grapheme boundaries (including ZWJ emoji, flags and combining marks),
 * reserving space for an ellipsis and respecting both cells and bytes. */
static void
show_restored_filename(const char *path)
{
    const char *base = path;
    const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');
    const char *label = JSTR("ファイル名：", "File name: ");
    char sanitized[MAXSTR];
    char message[MAXSTR];
    int line_cells = (_COLS < NUMCOLS ? _COLS : NUMCOLS) - 1;

    if (slash != NULL && (backslash == NULL || slash > backslash))
        base = slash + 1;
    else if (backslash != NULL)
        base = backslash + 1;
    if (line_cells <= mbstrlen(label))
        return;

    sanitize_restored_basename(base, sanitized, sizeof sanitized);
    jrogue_strlcpy(message, label, sizeof message);
    jrogue_text_append_labeled(message, sizeof message, line_cells,
                               "", sanitized, "");

    /* The restored curses window can contain cells from an old prompt.
     * Clear the entire first row rather than passing this notice through
     * msg()/endmsg() and its pending --More-- state. */
    jrogue_set_normal_attr();
    move(0, 0);
    clrtoeol();
#ifdef _WIN32
    jrogue_waddstr_utf8(stdscr, message);
#else
    addstr(message);
#endif
    clrtoeol();
    mpos = mbstrlen(message);
    if (save_msg)
        jrogue_strlcpy(huh, message, MAXSTR);
    jrogue_restore_cursor();
    refresh();
}

/* The original affirmative echo was followed immediately by saving.
 * Keep it visible briefly on modern terminals, using the same msleep()
 * mechanism as movement/throwing animations.  This is only called for Y
 * at the initial filename prompt; N, Esc and the overwrite *question*
 * are not delayed (the separate overwrite notice is handled below). */
static void
show_save_confirmation_yes(void)
{
    jrogue_set_normal_attr();
#ifdef _WIN32
    jrogue_waddstr_utf8(stdscr, JSTR("はい", "Yes"));
#else
    addstr(JSTR("はい", "Yes"));
#endif
    refresh();
    msleep(200);
}

/* The overwrite notice used to be cleared immediately by save_file().
 * msg() flushes the top row; keep that original notice visible for the
 * same brief interval as the affirmative reply above. */
static void
show_save_overwrite_notice(void)
{
    msg(JSTR("既存のセーブファイルを上書きします。",
             "Overwriting the save file."));
    msleep(200);
}

/* An open failure must be readable rather than disappearing beneath the
 * next filename prompt.  Capture errno at the failed fopen() call site;
 * curses/input routines may change it.  Esc cancels the save as elsewhere
 * in this dialog; any other key returns to filename entry. */
static int
show_save_open_error(int open_error)
{
    int key;

    /* The preceding overwrite notice (if any) was already displayed.
     * Avoid an unrelated --More-- prompt before the actual error. */
    mpos = 0;
    if (msg(JSTR("セーブファイルを開けません：%s（キーで再入力、Escで中止）",
                 "Cannot open save file: %s (key: retry, Esc: cancel)"),
            strerror(open_error ? open_error : EIO)) == ESCAPE)
        return ESCAPE;
    jrogue_show_cursor(TRUE);
    key = readchar();
    jrogue_restore_cursor();
    return key;
}

/*
 * save_game:
 *	Implement the "save game" command
 */

void
save_game(void)
{
    FILE *savef;
    int c;
    auto char buf[MAXSTR];

    /*
     * get file name
     */
    mpos = 0;
over:
    if (file_name[0] != '\0')
    {
	for (;;)
	{
	    confirm_save_name(file_name);
	jrogue_show_cursor(TRUE);
	    c = readchar();
	jrogue_restore_cursor();
	    mpos = 0;
	    if (c == ESCAPE)
	    {
		msg("");
		return;
	    }
	    else if (c == 'n' || c == 'N' || c == 'y' || c == 'Y')
		break;
	    else
		msg(JSTR("'Y'か'N'で答えてください。（ESCでキャンセル）", "please answer Y or N"));
	jrogue_show_cursor(TRUE);
	}
	if (c == 'y' || c == 'Y')
	{
	    show_save_confirmation_yes();
	    jrogue_strlcpy(buf, file_name, sizeof buf);
	    goto gotfile;
	}
    }

    do
    {
	mpos = 0;
	msg(JSTR("ファイル名：", "file name: "));
	jrogue_show_cursor(TRUE);
	buf[0] = '\0';
	jrogue_set_normal_attr();
	if (get_str_filename(buf, stdscr) == QUIT)
	{
quit_it:
	    msg("");
	jrogue_restore_cursor();
	    return;
	}
	mpos = 0;
gotfile:
	/*
	 * test to see if the file exists
	 */
	if (
#ifdef _WIN32
            jrogue_windows_stat_utf8(buf, &sbuf)
#else
            stat(buf, &sbuf)
#endif
            >= 0)
	{
	    for (;;)
	    {
		msg(JSTR("そのファイルは存在します。上書きしますか？", "File exists.  Do you wish to overwrite it?"));
	jrogue_show_cursor(TRUE);
		mpos = 0;
		if ((c = readchar()) == ESCAPE)
		    goto quit_it;
		if (c == 'y' || c == 'Y')
		    break;
		else if (c == 'n' || c == 'N')
		    goto over;
		else
		    msg(JSTR("'Y'か'N'で答えてください。", "Please answer Y or N"));
	jrogue_show_cursor(TRUE);
	    }
	    show_save_overwrite_notice();
	    md_unlink(buf);
	}
	jrogue_strlcpy(file_name, buf, MAXSTR);
	if ((savef =
#ifdef _WIN32
             jrogue_windows_fopen_utf8(file_name, L"wb")
#else
             fopen(file_name, "wb")
#endif
             ) == NULL)
        {
            /* Do not overwrite the error before the player can read it. */
            if (show_save_open_error(errno) == ESCAPE)
                goto quit_it;
        }
    } while (savef == NULL);
    msg("");
    save_file(savef);
	jrogue_restore_cursor();
    /* NOTREACHED */
}

/*
 * auto_save:
 *	Automatically save a file.  This is used if a HUP signal is
 *	received
 */

void
auto_save(int sig)
{
    FILE *savef = NULL;
    (void)sig;

    md_ignoreallsignals();
    if (file_name[0] != '\0')
    {
	savef =
#ifdef _WIN32
            jrogue_windows_fopen_utf8(file_name, L"wb");
#else
            fopen(file_name, "wb");
#endif
	if (savef == NULL)
	{
	    md_unlink(file_name);
	    savef =
#ifdef _WIN32
            jrogue_windows_fopen_utf8(file_name, L"wb");
#else
            fopen(file_name, "wb");
#endif
	}
	if (savef != NULL)
	    save_file(savef);
    }
    exit(1);
}

/*
 * save_file:
 *	Write the saved game on the file
 */

void
save_file(FILE *savef)
{
    char buf[80] = { 0 };
    int save_err = 0;
    size_t len;

    mvcur(0, _COLS - 1, _LINES - 1, 0); 
    putchar('\n');
    restore_terminal_background();
    jrogue_endwin();
    resetltchars();
    md_chmod(file_name, 0400);

    encclearerr();
    len = strlen(version) + 1;
    if (encwrite(version, len, savef) != len && encerror() == 0)
	encseterr(EIO);
    snprintf(buf, sizeof buf, "%d x %d lang %s\n", _LINES, _COLS, jrogue_language_code());
    if (encerror() == 0 && encwrite(buf, 80, savef) != 80 && encerror() == 0)
	encseterr(EIO);

    if (encerror() != 0)
	save_err = encclearerr();
    else
	save_err = rs_save_file(savef);

    if (fflush(savef) == EOF && save_err == 0)
	save_err = (errno != 0) ? errno : EIO;
    if (ferror(savef) && save_err == 0)
	save_err = (errno != 0) ? errno : EIO;
    if (fclose(savef) == EOF && save_err == 0)
	save_err = (errno != 0) ? errno : EIO;

    if (save_err != 0)
    {
	(void) md_unlink(file_name);
	fprintf(stderr, "%s: %s\n",
	    JSTR("セーブファイルの書き込みに失敗しました", "could not write save file"),
	    strerror(save_err));
	exit(1);
    }

    exit(0);
}

/*
 * restore:
 *	Restore a saved game from a file with elaborate checks for file
 *	integrity from cheaters
 */
int
restore(const char *file)
{
    FILE *inf;
    int syml;
    auto char buf[MAXSTR];
    auto STAT sbuf2;
    int lines = 0, cols = 0;
    int err;
    char saved_lang[8];
    char restore_path[MAXSTR];
    size_t path_len;

    if (strcmp(file, "-r") == 0)
        file = file_name;

    /* rs_restore_file() restores the saved value of the global file_name.
     * The -r argument can point into that same array: make a private copy
     * before loading so the actual restore path cannot change underneath
     * this function.  This also keeps the display name stable. */
    path_len = strlen(file);
    if (path_len >= sizeof restore_path)
    {
        fprintf(stderr, "%s", JSTR("セーブファイルのパスが長すぎます。\n",
                                      "Save file path is too long.\n"));
        return FALSE;
    }
    memcpy(restore_path, file, path_len + 1);
    file = restore_path;

    md_tstphold();

#ifdef _WIN32
    if ((inf = jrogue_windows_fopen_utf8(file, L"rb")) == NULL)
#else
    if ((inf = fopen(file,"rb")) == NULL)
#endif
    {
	perror(file);
	return FALSE;
    }
#ifdef _WIN32
    if (jrogue_windows_stat_utf8(file, &sbuf2) < 0)
#else
    if (stat(file, &sbuf2) < 0)
#endif
    {
	perror(file);
	fclose(inf);
	return FALSE;
    }
    syml = is_symlink(file);

    fflush(stdout);
    encclearerr();
    if (encread(buf, strlen(version) + 1, inf) != strlen(version) + 1 || encerror() != 0)
    {
	fclose(inf);
	printf("%s", JSTR("セーブファイルを読み込めません。\n", "Cannot read save file.\n"));
	return FALSE;
    }
    if (memcmp(buf, version, strlen(version) + 1) != 0)
    {
	fclose(inf);
	printf("%s", JSTR("古いバージョンのセーブファイルは使えません。\n", "Sorry, saved game is out of date.\n"));
	return FALSE;
    }
    if (encread(buf,80,inf) != 80 || encerror() != 0)
    {
	fclose(inf);
	printf("%s", JSTR("セーブファイルを読み込めません。\n", "Cannot read save file.\n"));
	return FALSE;
    }
    /* save_file() writes a NUL-terminated, zero-padded 80-byte header.
     * Do not pass a corrupt unterminated header to sscanf(). */
    if (memchr(buf, '\0', 80) == NULL)
    {
        fclose(inf);
        printf("%s", JSTR("セーブファイルを読み込めません。\n", "Cannot read save file.\n"));
        return FALSE;
    }
    saved_lang[0] = '\0';
    /* The map is always 80x24.  Old JEC saves may record a larger
     * physical screen in this header after a restore; accept those sane
     * dimensions for compatibility, but never accept an undersized or
     * implausibly huge saved screen.  The separate saved WINDOW dimensions
     * are checked by rs_read_window(). */
    if (sscanf(buf,"%d x %d lang %7s\n", &lines, &cols, saved_lang) < 2
        || lines < NUMLINES || cols < NUMCOLS ||
        lines > 512 || cols > 1024 ||
        (long long)lines * cols > 262144)
    {
	fclose(inf);
	printf("%s", JSTR("セーブファイルを読み込めません。\n", "Cannot read save file.\n"));
	return FALSE;
    }

    /* saved_lang records the language used when the file was saved.
     * The saved game itself is language-independent: object types and
     * discoveries are restored by internal IDs, while user-entered names
     * and labels remain raw UTF-8 strings.  A language mismatch is allowed.
     */

    jrogue_reset_screen();
    jrogue_init_locale();
    initscr();                          /* Start up cursor package */
#ifdef _WIN32
    jrogue_windows_retry_direct_launch_resize(1);
#endif
    start_custom_color();
    keypad(stdscr, 1);

    /* Match normal startup and -d: a physical terminal must support the
     * 80x24 dungeon, while the logical display is limited to 80x32.
     * Unlike the original 5.4 restore, do not reject an old save merely
     * because it was written with additional auxiliary screen rows.
     * rs_read_window() clips the old physical WINDOW to this display. */
    if (LINES < NUMLINES || COLS < NUMCOLS)
    {
        restore_terminal_background();
        jrogue_endwin();
        printf(JSTR("\nごめん！ 画面の大きさは最低 %dx%d なんだ。\n",
                    "\nSorry, the screen must be at least %dx%d.\n"),
               NUMLINES, NUMCOLS);
        fclose(inf);
        return FALSE;
    }
    _COLS = COLS > MAXCOLS ? MAXCOLS : COLS;
    _LINES = LINES > MAXLINES ? MAXLINES : LINES;

    hw = newwin(_LINES, _COLS, 0, 0);
    jrogue_wset_normal_attr(hw);
    if (color_mode)
    {
	bkgd(' ' | jrogue_normal_attr());
	attrset(jrogue_normal_attr());
	erase();
	refresh();
	wbkgd(hw, ' ' | jrogue_normal_attr());
	wattrset(hw, jrogue_normal_attr());
    }
    setup();

    if ((err = rs_restore_file(inf)) != 0)
    {
	restore_terminal_background();
	jrogue_endwin();
	fclose(inf);
	printf("%s: %s\n",
	    JSTR("セーブファイルが壊れているか、最後まで読み込めません",
	         "Save file is damaged or incomplete"),
	    strerror(err));
	return FALSE;
    }
    /*
     * we do not close the file so that we will have a hold of the
     * inode for as long as possible
     */

    if (
#ifdef MASTER
	!wizard &&
#endif
        md_unlink_open_file(file, inf) < 0)
    {
	restore_terminal_background();
	jrogue_endwin();
	printf("Cannot unlink file\n");
	return FALSE;
    }
    /* The filename notice is replaced by show_restored_filename() after
     * restoring the game state and validating the save file. */
    mpos = 0;
    /*
     * defeat multiple restarting from the same place
     */
#ifdef MASTER
    if (!wizard)
#endif
	if (sbuf2.st_nlink != 1 || syml)
	{
	    restore_terminal_background();
	    jrogue_endwin();
	    printf("%s", JSTR("\nそのファイルから再開することはできません。\n", "\nCannot restore from a linked file\n"));
#ifndef _WIN32
	    fclose(inf);
#endif
	    return FALSE;
	}

    if (pstats.s_hpt <= 0)
    {
	restore_terminal_background();
	jrogue_endwin();
	printf("%s", JSTR("\n\"彼は死んだんだよ、ジム。\"\n", "\n\"He's dead, Jim\"\n"));
#ifndef _WIN32
	fclose(inf);
#endif
	return FALSE;
    }

	md_tstpresume();

    jrogue_strlcpy(file_name, file, MAXSTR);
    /* A direct Windows launch printed its menu on the physical console.
     * Explicitly repaint the restored curses screen, rather than relying
     * on a subsequent move to overwrite text still visible underneath. */
#ifdef _WIN32
    clearok(stdscr, TRUE);
    refresh();
#else
    clearok(curscr, TRUE);
#endif
    srand(md_getpid());
#ifndef _WIN32
    fclose(inf);
#endif
    apply_idscroll_names();
    show_restored_filename(file);
    jrogue_restore_cursor();
    playit();
    /*NOTREACHED*/
    return(0);
}

static int encerrno = 0;

int
encerror()
{
    return encerrno;
}

void
encseterr(int err)
{
    encerrno = err;
}

int
encclearerr()
{
    int n = encerrno;

    encerrno = 0;

    return(n);
}

/*
 * encwrite:
 *	Perform an encrypted write
 */

size_t
encwrite(const char *start, size_t size, FILE *outf)
{
    const char *e1, *e2;
    char fb;
    int temp;
    size_t o_size = size;
    e1 = encstr;
    e2 = statlist;
    fb = 0;

    if (encerrno) {
	errno = encerrno;
	return 0;
    }

    while(size)
    {
	if (putc(*start++ ^ *e1 ^ *e2 ^ fb, outf) == EOF)
	{
	    encerrno = (errno != 0) ? errno : EIO;
            break;
	}

	temp = *e1++;
	fb = fb + ((char) (temp * *e2++));
	if (*e1 == '\0')
	    e1 = encstr;
	if (*e2 == '\0')
	    e2 = statlist;
	size--;
    }

    return(o_size - size);
}

/*
 * encread:
 *	Perform an encrypted read
 */
size_t
encread(char *start, size_t size, FILE *inf)
{
    const char *e1, *e2;
    char fb;
    int temp;
    size_t read_size;
    size_t items;
    fb = 0;

    if (encerrno) {
	errno = encerrno;
	return 0;
    }

    items = read_size = fread(start,1,size,inf);

    e1 = encstr;
    e2 = statlist;

    while (read_size--)
    {
	*start++ ^= *e1 ^ *e2 ^ fb;
	temp = *e1++;
	fb = fb + (char)(temp * *e2++);
	if (*e1 == '\0')
	    e1 = encstr;
	if (*e2 == '\0')
	    e2 = statlist;
    }

    if (items != size)
	encerrno = (errno != 0) ? errno : EIO;

    return(items);
}

/*
 * rd_score:
 * Read the existing scoreboard, which may be empty on the first run or
 * incomplete after an interrupted write.  The game's encrypted I/O layer
 * remembers a short read as a global error, so a first-run EOF must not
 * prevent wr_score() from saving the new score.
 */
void
rd_score(SCORE *top_ten)
{
    char name[MAXSTR];
    char scoreline[100];
    unsigned int uid, death_id, score_timestamp;
    int points, killer_id, depth;
    int i;

    if (scoreboard == NULL)
        return;

    encclearerr();
    rewind(scoreboard);
    for (i = 0; i < numscores; i++)
    {
        if (encread(name, sizeof name, scoreboard) != sizeof name ||
            encread(scoreline, sizeof scoreline, scoreboard) != sizeof scoreline)
            break;

        name[sizeof name - 1] = '\0';
        scoreline[sizeof scoreline - 1] = '\0';
        if (sscanf(scoreline, " %u %d %u %d %d %x",
                   &uid, &points, &death_id, &killer_id,
                   &depth, &score_timestamp) != 6)
            break;

        /* The four SC_DEATH_* values (0..3) index rip.c's reason[]
         * array.  A damaged score record must not introduce an invalid
         * index into the score display.  Stop at that record, just as
         * for a malformed or incomplete entry; preserve earlier scores. */
        if (death_id > SC_DEATH_AMULET)
            break;

        memcpy(top_ten[i].sc_name, name, sizeof name);
        top_ten[i].sc_uid = uid;
        top_ten[i].sc_score = points;
        top_ten[i].sc_death_id = death_id;
        top_ten[i].sc_killer_id = killer_id;
        top_ten[i].sc_level = depth;
        top_ten[i].sc_time = score_timestamp;
    }

    /* A normal EOF must not poison subsequent encrypted writes. */
    encclearerr();
    rewind(scoreboard);  /* Also clears the FILE* EOF/error indicators. */
}

/*
 * wr_score:
 * Write the same encrypted, fixed-size records as older releases.
 * No scoreboard format or save-file compatibility change is necessary.
 */
int
wr_score(SCORE *top_ten)
{
    char scoreline[100];
    int i;
    int failed = FALSE;

    if (scoreboard == NULL)
        return FALSE;

    encclearerr();
    rewind(scoreboard);

    for (i = 0; i < numscores; i++)
    {
        memset(scoreline, 0, sizeof scoreline);
        snprintf(scoreline, sizeof scoreline, " %u %d %u %d %d %x \n",
                 top_ten[i].sc_uid, top_ten[i].sc_score,
                 top_ten[i].sc_death_id, top_ten[i].sc_killer_id,
                 top_ten[i].sc_level, top_ten[i].sc_time);
        if (encwrite(top_ten[i].sc_name, MAXSTR, scoreboard) != MAXSTR ||
            encwrite(scoreline, sizeof scoreline, scoreboard) != sizeof scoreline)
        {
            failed = TRUE;
            break;
        }
    }
    if (fflush(scoreboard) == EOF)
        failed = TRUE;
    if (failed)
        fprintf(stderr, "Could not write score file: %s\n",
                strerror(encerror() ? encerror() : (errno ? errno : EIO)));

    encclearerr();
    rewind(scoreboard);
    return !failed;
}
