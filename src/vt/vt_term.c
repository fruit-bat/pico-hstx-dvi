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
)
{
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
)
{
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
)
{
    vt_term_clear_lines(t, 0, t->w);
}

static void vt_term_putch(
    vt_term_t *t, // The terminal
    vt_char_t ch)
{
    t->rp[t->r][t->c] = vt_cell_combine(t->attr, ch);
}

void vt_term_init(
    vt_term_t *t,    // The terminal
    vt_cell_t *grid, // Cell grid for the display
    vt_coord_t w,    // Terminal width in characters
    vt_coord_t h     // Terminal height in characters
)
{
    // Set the terminal size
    t->w = w; // Terminal width
    t->h = h; // Terminal height

    // Cursor top left
    t->c = 0; // Cursor column
    t->r = 0; // Cursor row

    // No margins
    t->mt = 0; // Margin top
    t->mb = h - 1; // Margin bottom

    // The default attributes
    vt_term_reset_attr(t);

    // TODO perhaps these should be passed in?
    // Initialize the row pointers
    for (vt_coord_t r = 0; r < h; ++r)
    {
        t->rp[r] = grid;
        grid += w;
    }

    // Clear the screen
    vt_term_clear_screen(t);
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
    vt_term_t *t,  // The terminal
    uint32_t p0    // Parameter to specify the erase mode (0, 1, 2)
) {
    // Default range: clear the entire screen
    vt_coord_t b = 0;       // Start row (inclusive)
    vt_coord_t e = t->h;    // End row (exclusive)

    // Handle the different erase modes
    switch (p0) {
        case 0: 
            // Erase from the cursor to the end of the screen
            b = t->r + 1;  // Start clearing from the row after the cursor
            vt_term_clear_line(t, t->r, t->c, t->w);  // Clear from cursor column to the end of the current row
            break;
        case 1: 
            // Erase from the start of the screen to the cursor
            e = t->r;  // End clearing at the row before the cursor
            vt_term_clear_line(t, t->r, 0, t->c);  // Clear from the start of the current row to the cursor column
            break;
        case 2:  
            // Erase the entire screen (default range is already set)
            break;
        default: 
            // Invalid parameter, do nothing
            return;
    }

    // Clear the lines in the specified range
    vt_term_clear_lines(t, b, e - b);
}

void vt_term_cursor_down(
    vt_term_t *t)
{
}

void vt_term_cursor_up(
    vt_term_t *t)
{
}

#ifdef VT_BUILD_TEST
#include "vt_term_test.c"
#endif
