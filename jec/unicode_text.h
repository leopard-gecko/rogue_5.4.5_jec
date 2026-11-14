/* Shared UTF-8 text handling for option input and terminal cell widths.
 * Copyright (c) 2026 leopardgecko. See LICENSE.TXT.
 * This module does not depend on curses, Windows APIs or the current locale.
 */
#ifndef JROGUE_UNICODE_TEXT_H
#define JROGUE_UNICODE_TEXT_H

#include <stddef.h>

/* Positions are UTF-8 byte offsets; the returned offsets are whole
 * extended-character boundaries.  Invalid UTF-8 bytes stand alone. */
size_t jrogue_text_next(const char *text, size_t pos);
size_t jrogue_text_prev(const char *text, size_t pos);

/* Largest complete character prefix within max_bytes bytes. */
size_t jrogue_text_prefix(const char *text, size_t max_bytes);

/* Terminal columns, not code points or UTF-8 bytes.  The _n variant
 * measures complete characters in the first max_bytes bytes. */
int jrogue_text_width(const char *text);
/* Legacy name retained for callers that measure terminal-cell widths. */
int mbstrlen(const char *text);
int jrogue_text_width_n(const char *text, size_t max_bytes);

/* Longest complete-grapheme prefix fitting within max_cells columns.
 * The returned offset can safely terminate a UTF-8 string. */
size_t jrogue_text_prefix_cells(const char *text, int max_cells);

/* Append before + a possibly shortened label + after to dst.  Reserve
 * room for the fixed suffix first, and clip only label at whole grapheme
 * boundaries (with ASCII dots if necessary).  The limit is the width
 * of the entire result including the existing contents of dst.
 * This is a display-only operation; the saved label is never changed. */
void jrogue_text_append_labeled(char *dst, size_t dst_size, int max_cells,
                                const char *before, const char *label,
                                const char *after);

#endif /* JROGUE_UNICODE_TEXT_H */
