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
/* Build instructions:
 *
 * cc -DVT_BUILD_TEST -Wall vt_term.c
 * 
 * ./a.out
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "vt_term.h"
#include <string.h>
#include <assert.h>

// Print out the characters in the grid so we can see what is going on
void print_grid(vt_term_t *t) {
    vt_coord_t w = t->w;
    vt_coord_t h = t->h;
    for (vt_coord_t c = 0; c < w + 2; ++c) printf("-");
    printf("\n");
    for (vt_coord_t r = 0; r < h; ++r) {
        printf("|");
        for (vt_coord_t c = 0; c < w; ++c) {
            vt_cell_t ct = t->rp[r][c];
            vt_char_t ch = vt_cell_get_char(ct);
            printf("%c", (uint8_t)ch);
        }
        printf("|\n");
    }
    for (vt_coord_t c = 0; c < w + 2; ++c) printf("-");
    printf("\n");
}

void check_grid_blank(vt_cell_t* grid, vt_coord_t w, vt_coord_t h, vt_cell_t cb) {
    for (vt_coord_t c = 0; c < w; ++c) {
        for (vt_coord_t r = 0; r < h; ++r) {
            vt_cell_t ct = grid[(r * w) + c];
            assert(ct == cb);
        }
    }
}

int main() {
    const vt_coord_t w = 20;
    const vt_coord_t h = 16;
    vt_cell_t grid[h][w];
    vt_term_t t;
    vt_term_init(&t, (vt_cell_t*)grid, w, h);
    assert(t.c == 0);
    assert(t.r == 0);
    assert(t.w == w);
    assert(t.h == h);

    // Create a cell with default attributes and a space character
    vt_cell_attr_t c1 = vt_cell_enc_attr(
        VT_TERM_DEFAULT_FG,
        VT_TERM_DEFAULT_BG,
        VT_TERM_DEFAULT_FLAGS
    );
    vt_cell_t c3 = vt_cell_combine(c1, 32);

    // Check the screen is blank on start-up
    print_grid(&t);
    check_grid_blank(&grid[0][0], w, h, c3);
    for (vt_coord_t c = 0; c < w; ++c) {
        for (vt_coord_t r = 0; r < h; ++r) {
            vt_cell_t ct = grid[r][c];
            assert(ct == c3);
        }
    }

    // Put characters in the grid so we can test scroll up
    // AAAAA...
    // BBBBB...
    // etc.
    for (vt_coord_t c = 0; c < w; ++c) {
        for (vt_coord_t r = 0; r < h; ++r) {
            grid[r][c] = vt_cell_combine(c1, r + 'A');
        }
    }    
    print_grid(&t);

    printf("\nScroll up 1 row\n");
    vt_term_scroll_up(&t, 0, 1);
    print_grid(&t);

    printf("\nScroll up 100 rows\n");
    vt_term_scroll_up(&t, 0, 100);
    print_grid(&t);
    check_grid_blank(&grid[0][0], w, h, c3);

    printf("all ok\n");
    return 0;
}   


