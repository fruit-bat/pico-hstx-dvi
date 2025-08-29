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

#define VT_TERM_DEFAULT_BG 0
#define VT_TERM_DEFAULT_FG 1

void vt_term_reset_attr(
    vt_term_t* t // The terminal
) {
    // The default attributes
    t->attr = vt_cell_enc_attr(
        VT_TERM_DEFAULT_FG,
        VT_TERM_DEFAULT_BG,
        VT_CELL_FLAGS_NORMAL
    );
}

static void vt_term_clearline(
    vt_term_t* t,  // The terminal
    vt_coord_t ri, // Row index
    vt_coord_t sc, // The start column
    vt_coord_t ec  // The end column
) {
    vt_cell_t c = vt_cell_combine(t->attr, (vt_char_t)' ');
    vt_cell_t* rp = t->rp[ri];
    if (ec > t->w) ec = t->w;
    for (vt_coord_t i = sc; i < ec; i++){
        rp[i] = c;
    }
}

static void vt_term_clearlines(
    vt_term_t* t,  // The terminal
    vt_coord_t rs, // Row start
    vt_coord_t rc  // Row count
) {
    if (rs >= t->h) return;
    if (rs + rc >= t->h) rc = t->h - rs;
    for (vt_coord_t ri = 0; ri < rc; ++ri) {
        vt_term_clearline(t, ri, 0, t->w);
    }
}

static void vt_term_putch(
    vt_term_t* t,  // The terminal
    vt_char_t ch
) {
    t->rp[t->r][t->c] = vt_cell_combine(t->attr, ch);

}

void vt_term_init(
    vt_term_t* t, // The terminal
    vt_cell_t* grid, // Cell grid for the display
    vt_coord_t w, // Terminal width in characters
    vt_coord_t h  // Terminal height in characters
) {
    // Set the terminal size
    t->w = w;  // Terminal width
    t->h = h;  // Terminal height

    // Cursor top left
    t->c = 0;  // Cursor column
    t->r = 0;  // Cursor row

    // No margins
    t->mt = 0; // Margin top
    t->mb = 0; // Margin bottom

    // The default attributes
    vt_term_reset_attr(t);

    // TODO perhaps these should be passed in?
    // Initialize the row pointers
    for(vt_coord_t r = 0; r < h; ++r) {
        t->rp[r] = grid;
        grid += w;
    }
}

void vt_term_cursor_up(
    vt_term_t* t
) {


}

void vt_term_cursor_down(
    vt_term_t* t
) {

    
}

void vt_term_scroll_up(
    vt_term_t* t
) {

    
}

void vt_term_scroll_down(
    vt_term_t* t
) {
}
    
