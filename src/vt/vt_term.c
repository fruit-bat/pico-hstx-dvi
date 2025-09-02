/* Copyright (c) 2025 fruit-bat
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the copyright holder nor the
 *     names of contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS,
 * COPYRIGHT HOLDERS, OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "vt_term.h"
#include "vt_cell.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void vt_term_reset_attr(
    vt_term_t *t // The terminal
)
{
    // The default attributes
    t->attr = vt_cell_enc_attr(
        VT_TERM_DEFAULT_FG,
        VT_TERM_DEFAULT_BG,
        VT_TERM_DEFAULT_FLAGS);
}

static void vt_term_clear_line(
    vt_term_t *t,  // The terminal
    vt_coord_t ri, // Row index
    vt_coord_t sc, // The start column
    vt_coord_t ec  // The end column
) {
    vt_cell_t c = vt_cell_combine(t->attr, (vt_char_t)' ');
    vt_cell_t *rp = t->rp[ri];
    if (ec > t->w)
        ec = t->w;
    for (vt_coord_t i = sc; i < ec; i++)
    {
        rp[i] = c;
    }
}

static void vt_term_clear_lines(
    vt_term_t *t,  // The terminal
    vt_coord_t rs, // Row start
    vt_coord_t rc  // Row count
) {
    if (rs >= t->h)
        return;
    if (rs + rc >= t->h)
        rc = t->h - rs;
    for (vt_coord_t i = 0; i < rc; ++i)
    {
        vt_term_clear_line(t, rs + i, 0, t->w);
    }
}

static inline void vt_term_clear_screen(
    vt_term_t *t // The terminal
) {
    vt_term_clear_lines(t, 0, t->w);
}

// Write a character at the cursor
void vt_term_putch(
    vt_term_t *t, // The terminal
    vt_char_t ch
) {
    if (t->hang == VT_HANG_BOTTOM) {
        vt_term_scroll_up(t, t->mt, 1);
    }
    t->hang = VT_HANG_NONE;

    t->rp[t->r][t->c] = vt_cell_combine(t->attr, ch);

    if (t->c < t->w - 1) {
        t->c++;
    }
    else {
        t->hang = VT_HANG_LINE;
        t->c = 0;
        t->r++;
    }

    if ((t->hang != VT_HANG_NONE) && (t->r > t->mb)) {
        t->r = t->mb;
        t->hang = VT_HANG_BOTTOM;
    }
}


void vt_term_reset(
    vt_term_t* t  // The terminal
) {
    // Clear down any flags
    t->flags = 0;

    // Clear the delayed scroll
    t->hang = VT_HANG_NONE;

    // Cursor top left
    t->c = 0; // Cursor column
    t->r = 0; // Cursor row

    // No margins
    t->mt = 0;        // Margin top (exclusive)
    t->mb = t->h - 1; // Margin bottom (exclusive)

    // The default attributes
    vt_term_reset_attr(t);

    // Save the cursor
    vt_term_save_cursor(t);

    // Clear the screen
    vt_term_clear_screen(t);
}

void vt_term_save_cursor(
    vt_term_t* t  // The terminal
) {
    t->s_r = t->r;
    t->s_c = t->c;
    t->s_attr = t->attr;
}

void vt_term_restore_cursor(
    vt_term_t* t  // The terminal
) {
    t->r = t->s_r;
    t->c = t->s_c;
    t->attr = t->s_attr;
}

void vt_term_init(
    vt_term_t *t,    // The terminal
    vt_cell_t *grid, // Cell grid for the display
    vt_coord_t w,    // Terminal width in characters
    vt_coord_t h     // Terminal height in characters
) {
    // Set the terminal size
    t->w = w; // Terminal width
    t->h = h; // Terminal height

    // TODO perhaps these should be passed in?
    // Initialize the row pointers
    for (vt_coord_t r = 0; r < h; ++r)
    {
        t->rp[r] = grid;
        grid += w;
    }

    vt_term_reset(t);
}

void vt_term_cursor_set(
    vt_term_t *t,
    vt_coord_t r,
    vt_coord_t c
) {
    t->r = MIN(r, t->h - 1);
    t->c = MIN(c, t->w - 1);
}

void vt_term_cursor_set_col(
    vt_term_t *t,
    vt_coord_t c
) {
    t->c = MIN(c, t->w - 1);
}

void vt_term_cursor_set_row(
    vt_term_t *t,
    vt_coord_t r
) {
    t->r = MIN(r, t->h - 1);
}

void vt_term_scroll_up(
    vt_term_t *t,  // The terminal
    vt_coord_t rs, // The start row
    vt_coord_t n   // Number of rows to scroll
)
{
    // Don't scroll the lower margin
    if (rs > t->mb)
        return;
    // Don't scroll in the bottom margin
    vt_coord_t h = t->mb - rs + 1;
    if (n >= h)
    {
        // If scrolling more than visible just clear the lines
        vt_term_clear_lines(t, rs, h);
    }
    else
    {
        n = n < h ? n : h;
        // Rotate the line indexs
        vt_cell_t **rp = t->rp;
        vt_cell_t *tmp[n];
        // Save the first n row pointers
        for (vt_coord_t i = 0; i < n; ++i)
        {
            tmp[i] = rp[rs + i];
        }
        // Shift the remaining row pointers up
        for (vt_coord_t i = 0; i < h - n; ++i)
        {
            rp[rs + i] = rp[rs + i + n];
        }
        // Restore the saved row pointers to the end
        for (vt_coord_t i = 0; i < n; ++i)
        {
            rp[rs + h - n + i] = tmp[i];
        }
        // Clear the lines that have been moved to the bottom
        vt_term_clear_lines(t, rs + h - n, n);
    }
}

void vt_term_scroll_down(
    vt_term_t *t,  // The terminal
    vt_coord_t rs, // The start row
    vt_coord_t n   // Number of rows to scroll
)
{
    // Don't scroll the lower margin
    if (rs > t->mb)
        return;
    // Don't scroll in the bottom margin
    vt_coord_t h = t->mb - rs + 1;
    if (n >= h)
    {
        // If scrolling more than visible just clear the lines
        vt_term_clear_lines(t, rs, h);
    }
    else
    {
        n = n < h ? n : h;
        // Rotate the line indexes down
        vt_cell_t **rp = t->rp;
        vt_cell_t *tmp[n];
        // Save the last n row pointers
        for (vt_coord_t i = 0; i < n; ++i)
        {
            tmp[i] = rp[rs + h - n + i];
        }
        // Shift the remaining row pointers down
        for (vt_coord_t i = h - n; i > 0; --i)
        {
            rp[rs + i + n - 1] = rp[rs + i - 1];
        }
        // Restore the saved row pointers to the start
        for (vt_coord_t i = 0; i < n; ++i)
        {
            rp[rs + i] = tmp[i];
        }
        // Clear the lines that have been moved to the top
        vt_term_clear_lines(t, rs, n);
    }
}

// Erase in Display is a terminal control function (from ANSI escape codes, specifically ESC [ n J) that clears parts or all of the terminal screen, depending on the parameter n:
//
// n = 0: Erase from cursor to end of screen (including cursor position).
// n = 1: Erase from start of screen to cursor (including cursor position).
// n = 2: Erase entire screen.
void vt_term_erase_in_display(
    vt_term_t *t, // The terminal
    uint32_t p0   // Parameter to specify the erase mode (0, 1, 2)
)
{
    // Default range: clear the entire screen
    vt_coord_t b = 0;    // Start row (inclusive)
    vt_coord_t e = t->h; // End row (exclusive)

    // Handle the different erase modes
    switch (p0)
    {
    case 0:
        // Erase from the cursor to the end of the screen
        b = t->r + 1;                            // Start clearing from the row after the cursor
        vt_term_clear_line(t, t->r, t->c, t->w); // Clear from cursor column to the end of the current row
        break;
    case 1:
        // Erase from the start of the screen to the cursor
        e = t->r;                             // End clearing at the row before the cursor
        vt_term_clear_line(t, t->r, 0, t->c + 1); // Clear from the start of the current row to the cursor column
        break;
    case 2:
        // Erase the entire screen (default range is already set)
        break;
    case 3:
        // Erase the entire screen (default range is already set)
        // Also erase and scrollback lines (not implemented at present)
        break;
    default:
        // Invalid parameter, do nothing
        return;
    }

    // Clear the lines in the specified range
    vt_term_clear_lines(t, b, e - b);
}

void vt_term_insert_characters(
    vt_term_t *t, // The terminal
    uint32_t n    // The number of characters to insert (note: 0 means 0)
)
{
    if (n > t->w - t->c)
        n = t->w - t->c;
    if (n == 0)
        return;
    vt_coord_t to = t->w - 1;
    vt_coord_t fr = t->w - n - 1;
    vt_cell_t *rp = t->rp[t->r];
    while (fr > t->c)
    {
        rp[to--] = rp[fr--];
    }
    rp[to] = rp[fr];
    vt_term_clear_line(t, t->r, t->c, t->c + n);
}

void vt_term_delete_characters(
    vt_term_t *t, // The terminal
    uint32_t n    // The number of characters to delete (note: 0 means 0)
)
{
    if (n > t->w - t->c)
        n = t->w - t->c;
    if (n == 0)
        return;
    vt_coord_t to = t->c;
    vt_coord_t fr = t->c + n;
    vt_cell_t *rp = t->rp[t->r];
    while (fr < t->w)
    {
        rp[to++] = rp[fr++];
    }
    vt_cell_attr_t a = vt_cell_get_attr(rp[t->w - n]); // Copied from libtmt; is this correct?
    vt_cell_t cb = vt_cell_combine(a, (vt_char_t)' ');
    while (to < t->w)
    {
        rp[to++] = cb;
    }
}
/**
 * @brief Erases part or all of the current line in the terminal based on the specified mode.
 *
 * This function erases characters in the current line of the terminal, starting from or up to the cursor position,
 * depending on the value of the parameter `p0`:
 *   - If `p0 == 0`: Erases from the cursor position to the end of the line.
 *   - If `p0 == 1`: Erases from the beginning of the line up to and including the cursor position.
 *   - If `p0 == 2`: Erases the entire line.
 *
 * @param t  Pointer to the terminal structure.
 * @param p0 Erase mode (0: from cursor to end, 1: from start to cursor, 2: entire line).
 */

void vt_term_erase_in_line(
    vt_term_t *t, // The terminal
    uint32_t p0   // Parameter to specify the erase mode (0, 1, 2)
)
{
    switch (p0)
    {
    case 0:
        vt_term_clear_line(t, t->r, t->c, t->w);
        break;
    case 1:
        vt_term_clear_line(t, t->r, 0, MIN(t->c + 1, t->w));
        break;
    case 2:
        vt_term_clear_line(t, t->r, 0, t->w);
        break;
    }
}

void vt_term_flags_add(
    vt_term_t *t,
    vt_cell_flags_t f
) {
    t->attr = vt_cell_flags_add(t->attr, f);
}

void vt_term_flags_clear(
    vt_term_t *t,
    vt_cell_flags_t f
) {
    t->attr = vt_cell_flags_clear(t->attr, f);
}

void vt_term_set_fgci(
    vt_term_t *t,
    vt_cell_colour_t fgci
) {
    t->attr = vt_cell_fg_set(t->attr, fgci);
}

void vt_term_repeat(
    vt_term_t *t, // The terminal
    uint32_t n    // The number of characters to repeat
) {
    if (!t->c) return;
    vt_cell_t *rp = t->rp[t->r];
    vt_char_t ch = rp[t->c - 1];
    for (vt_coord_t i = 0; i < n; i++) {
        vt_term_putch(t, ch);
    }
}

void vt_term_reverse_nl(
    vt_term_t *t // The terminal
) {
    t->hang = VT_HANG_NONE;

    if (t->r == t->mt) {
        vt_term_scroll_down(t, t->mt, 1);
    }
    else if (t->r > 0) {
        t->r--;
    }
}

void vt_term_nl(
    vt_term_t *t // The terminal
) {
    if (t->hang != VT_HANG_NONE)
    {
        if (t->hang == VT_HANG_BOTTOM) {
            vt_term_scroll_up(t, t->mt, 1);
        }
        t->hang = VT_HANG_NONE;
        return;
    }

    if (t->r == t->mb) {
        vt_term_scroll_up(t, t->mt, 1);
    }
    else if (t->r < (t->h - 1)) {
        t->r++;
    }
}

void vt_term_cr(
    vt_term_t *t // The terminal
) {
    t->c = 0;
    if (t->hang == VT_HANG_LINE)
    {
        t->hang = VT_HANG_NONE;
        if (t->r > t->mt && t->r <= t->mb) {
            t->r--;
        }
    }
}

void vt_term_cursor_down(
    vt_term_t *t,
    vt_coord_t n
) {
    t->r = t->r + n >= t->h ? t->h - 1 : t->r + n;
}

void vt_term_cursor_up(
    vt_term_t *t,
    vt_coord_t n
) {
    t->r = t->r >= n ? t->r - n : 0;
}

void vt_term_cursor_left(
    vt_term_t *t,
    vt_coord_t n
) {
    t->c = t->c >= n ? t->c - n : 0;
}

void vt_term_cursor_right(
    vt_term_t *t,
    vt_coord_t n
) {
    t->c = t->c + n >= t->w ? t->w - 1 : t->c + n;
}

void vt_term_next_line_down(
    vt_term_t *t,
    vt_coord_t n
) {
    t->c = 0;
    t->r = t->r + n >= t->h ? t->h - 1 : t->r + n;
}

void vt_term_next_line_up(
    vt_term_t *t,
    vt_coord_t n
) {
    t->c = 0;
    t->r = t->r >= n ? t->r - n : 0;
}

#ifdef VT_BUILD_TEST
#include "vt_term_test.c"
#endif
