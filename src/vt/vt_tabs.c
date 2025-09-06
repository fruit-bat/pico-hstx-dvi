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
#include <stddef.h>
#include <stdio.h>
#include "vt_tabs.h"

inline static bool _vt_tabs_is_tab(
    vt_tabs_t *t,  // The tabs structure
    vt_coord_t c   // The column
) {
    return t->row[c >> 3] & (1 << (c & 7));
}

inline static void _vt_tabs_set_tab(
    vt_tabs_t *t,  // The tabs structure
    vt_coord_t c   // The column
) {
    t->row[c >> 3] |= 1 << (c & 7);
}

inline static void _vt_tabs_clear_tab(
    vt_tabs_t *t,  // The tabs structure
    vt_coord_t c   // The column
) {
    t->row[c >> 3] &= ~(1 << (c & 7));
}

void vt_tabs_init(
    vt_tabs_t *t,  // The tabs structure
    vt_coord_t w,  // The width of the screen
    vt_coord_t tw  // The width of tabs (default 8)
) {
    if (w > VT_SCREEN_MAX_COLS) w = VT_SCREEN_MAX_COLS;
    for (vt_coord_t i = 0; i < w; i++) {
        if (i % tw == 0) _vt_tabs_set_tab(t, i);
        else _vt_tabs_clear_tab(t, i);
    }
    _vt_tabs_set_tab(t, w - 1);
    t->w = w;
}

void vt_tabs_clear_all(
    vt_tabs_t *t  // The tabs structure
) {
    for (vt_coord_t i = 0; i < t->w; i++) _vt_tabs_clear_tab(t, i);
}

static vt_coord_t vt_tabs_next_one(
    vt_tabs_t *t, // The tabs structure
    vt_coord_t c  // The cursor column
) {
    const vt_coord_t s = c  + 1;
    if (s >= t->w) return c;
    for(vt_coord_t i = s; i < t->w; ++i) {
        if(_vt_tabs_is_tab(t, i)) return i;
    }
    return c;
}

vt_coord_t vt_tabs_next(
    vt_tabs_t *t, // The tabs structure
    vt_coord_t c, // The cursor column
    uint32_t n     // The number of tabs to move
) {
    while(n--) {
        vt_coord_t ct = vt_tabs_next_one(t, c);
        if (c == ct) return ct;
        c = ct;
    }
    return c;
}

static vt_coord_t vt_tabs_prev_one(
    vt_tabs_t *t, // The tabs structure
    vt_coord_t c  // The cursor column
) {
    for(vt_coord_t i = c; i > 0; --i) {
        const vt_coord_t s = i - 1;
        if(_vt_tabs_is_tab(t, s)) return s;
    }
    return c;
}

vt_coord_t vt_tabs_prev(
    vt_tabs_t *t, // The tabs structure
    vt_coord_t c, // The cursor column
    uint32_t n     // The number of tabs to move
) {
    while(n--) {
        vt_coord_t ct = vt_tabs_prev_one(t, c);
        if (c == ct) return ct;
        c = ct;
    }
    return c;
}

bool vt_tabs_is_tab(
    vt_tabs_t *t,  // The tabs structure
    vt_coord_t c   // The column
) {
    return _vt_tabs_is_tab(t, c);
}

void vt_tabs_set_tab(
    vt_tabs_t *t,  // The tabs structure
    vt_coord_t c   // The column
) {
    _vt_tabs_set_tab(t, c);
}

void vt_tabs_clear_tab(
    vt_tabs_t *t,  // The tabs structure
    vt_coord_t c   // The column
) {
    _vt_tabs_clear_tab(t, c);
}

