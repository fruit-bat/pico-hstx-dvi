#pragma once
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
#include "vt_types.h"
#include "vt_cell.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define VT_SCREEN_MAX_COLS 80
#define VT_SCREEN_MAX_ROWS 60
#define VT_TERM_DEFAULT_BG 0
#define VT_TERM_DEFAULT_FG 1
#define VT_TERM_DEFAULT_FLAGS VT_CELL_FLAGS_NORMAL

// VT100-derived terminals have a wrap behavior where the cursor "sticks"
// at the end of a line instead of immediately wrapping.  This allows you
// to use the last column without getting extra blank lines or
// unintentionally scrolling the screen.  The logic we implement for it
// is not exactly like that of a real VT100, but it seems to be
// sufficient for things to work as expected in the use cases and with
// the terminfo files I've tested with.  Specifically, I call the case
// where the cursor has advanced exactly one position past the rightmost
// column "hanging".  A rough description of the current algorithm is
// that there are two cases which each have two sub-cases:
// 1. You're hanging onto the next line below.  That is, you're not at
//    the bottom of the screen/scrolling region.
//    1a. If you receive a newline, hanging mode is canceled and nothing
//        else happens.  In particular, you do *not* advanced to the next
//        line.  You're already *at* the start of the "next" line.
//    2b. If you receive a printable character, just cancel hanging mode.
// 2. You're hanging past the bottom of the screen/scrolling region.
//    2a. If you receive a newline or printable character, scroll the
//        screen up one line and cancel hanging.
//    2b. If you receive a cursor reposition or whatever, cancel hanging.
// Below, hang is 0 if not hanging, or 1 or 2 as described above.
typedef enum {
    VT_HANG_NONE   = 0,
    VT_HANG_LINE   = 1,
    VT_HANG_BOTTOM = 2
} vt_term_hang_t;

typedef struct 
{
    vt_coord_t mt; // Margin top row
    vt_coord_t mb; // Margin bottom row
    vt_coord_t r; // Cursor row
    vt_coord_t c; // Cursor column
    vt_coord_t w; // Terminal width in characters
    vt_coord_t h; // Terminal height in characters
    vt_cell_t* rp[VT_SCREEN_MAX_ROWS]; // Row pointers
    vt_cell_attr_t attr; // The current attributes
    uint32_t flags;
    vt_term_hang_t hang; // Delayed scroll handling

} vt_term_t;

void vt_term_init(
    vt_term_t* t, // The terminal
    vt_cell_t* grid, // Cell grid for the display
    vt_coord_t w, // Terminal width in characters
    vt_coord_t h  // Terminal height in characters
);

void vt_term_reset(
    vt_term_t* t  // The terminal
);

void vt_term_scroll_up(
    vt_term_t *t,  // The terminal
    vt_coord_t rs, // The start row
    vt_coord_t n   // Number of rows to scroll
);

void vt_term_putch(
    vt_term_t *t, // The terminal
    vt_char_t ch
);

void vt_term_nl(
    vt_term_t *t // The terminal
);

void vt_term_cr(
    vt_term_t *t // The terminal
);

void vt_term_cursor_down(
    vt_term_t *t
);

void vt_term_cursor_up(
    vt_term_t *t
);

void vt_term_cursor_left(
    vt_term_t *t
);

void vt_term_cursor_right(
    vt_term_t *t
);

void vt_term_cursor_set(
    vt_term_t *t,
    vt_coord_t r,
    vt_coord_t c
);

// Erase in Display is a terminal control function (from ANSI escape codes, specifically ESC [ n J) that clears parts or all of the terminal screen, depending on the parameter n:
//
// n = 0: Erase from cursor to end of screen (including cursor position).
// n = 1: Erase from start of screen to cursor (including cursor position).
// n = 2: Erase entire screen.
void vt_term_erase_in_display(
    vt_term_t *t, // The terminal
    uint32_t p0   // Parameter to specify the erase mode (0, 1, 2)
);

// Erase in Display is a terminal control function (from ANSI escape codes, specifically ESC [ n J) that clears parts or all of the terminal screen, depending on the parameter n:
//
// n = 0: Erase from cursor to end of screen (including cursor position).
// n = 1: Erase from start of screen to cursor (including cursor position).
// n = 2: Erase entire screen.
void vt_term_erase_in_line(
    vt_term_t *t, // The terminal
    uint32_t p0   // Parameter to specify the erase mode (0, 1, 2)
);

#ifdef __cplusplus
}
#endif
