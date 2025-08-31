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
    for (vt_coord_t c = 0; c < w * 7 + 5; ++c) printf(c == (t->c + 1) ? "+" : "-");
    printf("\n");
    for (vt_coord_t r = 0; r < h; ++r) {
        printf(r == (t->r) ? "+" : "|");
        for (vt_coord_t c = 0; c < w; ++c) {
            vt_cell_t ct = t->rp[r][c];
            vt_char_t ch = vt_cell_get_char(ct);
            printf("%c", (uint8_t)ch);
        }
        printf(r == (t->r) ? "+" : "|");
        for (vt_coord_t c = 0; c < w; ++c) {
            vt_cell_t ct = t->rp[r][c];
            vt_cell_attr_t ca = vt_cell_get_attr(ct);
            vt_cell_colour_t ci = vt_cell_fg_get(ca);
            printf("%02X", (uint8_t)ci);
        }
        printf("|");
        for (vt_coord_t c = 0; c < w; ++c) {
            vt_cell_t ct = t->rp[r][c];
            vt_cell_attr_t ca = vt_cell_get_attr(ct);
            vt_cell_colour_t ci = vt_cell_bg_get(ca);
            printf("%02X", (uint8_t)ci);
        }
        printf("|");
        for (vt_coord_t c = 0; c < w; ++c) {
            vt_cell_t ct = t->rp[r][c];
            vt_cell_attr_t ca = vt_cell_get_attr(ct);
            vt_cell_flags_t fl = vt_cell_flags_get(ca);
            printf("%02X", (uint8_t)fl);
        }
        printf("|\n");
    }
    for (vt_coord_t c = 0; c < w * 7 + 5; ++c) printf(c == (t->c + 1) ? "+" : "-");
    printf("\n");
}

void check_grid_blank(vt_term_t *t) {
    // Create a cell with default attributes and a space character
    vt_cell_attr_t c1 = vt_cell_enc_attr(
        VT_TERM_DEFAULT_FG,
        VT_TERM_DEFAULT_BG,
        VT_TERM_DEFAULT_FLAGS
    );
    vt_cell_t c3 = vt_cell_combine(c1, 32);
    vt_coord_t w = t->w;
    vt_coord_t h = t->h;    
    for (vt_coord_t c = 0; c < w; ++c) {
        for (vt_coord_t r = 0; r < h; ++r) {
            vt_cell_t ct = t->rp[r][c];
            assert(ct == c3);
        }
    }
}

void set_grid_rows(vt_term_t *t) {
    // Create a cell with default attributes and a space character
    vt_cell_attr_t c1 = vt_cell_enc_attr(
        VT_TERM_DEFAULT_FG,
        VT_TERM_DEFAULT_BG,
        VT_TERM_DEFAULT_FLAGS
    );
    vt_coord_t w = t->w;
    vt_coord_t h = t->h;    

    // Put characters in the grid so we can test scroll up
    // AAAAA...
    // BBBBB...
    // etc.
    for (vt_coord_t c = 0; c < w; ++c) {
        for (vt_coord_t r = 0; r < h; ++r) {
            t->rp[r][c] = vt_cell_combine(c1, r + 'A');
        }
    } 
}

void set_grid_cols(vt_term_t *t) {
    // Create a cell with default attributes and a space character
    vt_cell_attr_t c1 = vt_cell_enc_attr(
        VT_TERM_DEFAULT_FG,
        VT_TERM_DEFAULT_BG,
        VT_TERM_DEFAULT_FLAGS
    );
    vt_coord_t w = t->w;
    vt_coord_t h = t->h;    

    // Put characters in the grid so we can test scroll up
    // AAAAA...
    // BBBBB...
    // etc.
    for (vt_coord_t c = 0; c < w; ++c) {
        for (vt_coord_t r = 0; r < h; ++r) {
            t->rp[r][c] = vt_cell_combine(c1, c + 'A');
        }
    } 
}

void check_grid_rows(vt_term_t *t, char* chs) {
    vt_coord_t w = t->w;
    vt_coord_t h = t->h;    
    for (vt_coord_t c = 0; c < w; ++c) {
        for (vt_coord_t r = 0; r < h; ++r) {
            vt_cell_t ct = t->rp[r][c];
            vt_char_t ch = vt_cell_get_char(ct);
            if (chs[r] != '?') assert(ch == (vt_char_t)(chs[r]));
        }
    }
}

void check_grid_row(vt_term_t *t, vt_coord_t r, char* chs) {
    vt_coord_t w = t->w;
    for (vt_coord_t c = 0; c < w; ++c) {
        vt_cell_t ct = t->rp[r][c];
        vt_char_t ch = vt_cell_get_char(ct);
        assert(ch == (vt_char_t)(chs[c]));
    }
}

void test_scroll(vt_term_t *t) {
    // Put characters in the grid so we can test scroll up
    // AAAAA...
    // BBBBB...
    // etc.
    set_grid_rows(t);
    print_grid(t);
    check_grid_rows(t, "ABCDEFGHIJKLMNOP"); 

    printf("\nScroll up 1 row\n");
    vt_term_scroll_up(t, 0, 1);
    print_grid(t);
    check_grid_rows(t, "BCDEFGHIJKLMNOP "); 

    printf("\nScroll up 2 rows\n");
    set_grid_rows(t);
    check_grid_rows(t, "ABCDEFGHIJKLMNOP"); 
    vt_term_scroll_up(t, 0, 2);
    print_grid(t);
    check_grid_rows(t, "CDEFGHIJKLMNOP  "); 

    printf("\nScroll up 100 rows\n");
    set_grid_rows(t);
    check_grid_rows(t, "ABCDEFGHIJKLMNOP"); 
    vt_term_scroll_up(t, 0, 100);
    print_grid(t);
    check_grid_blank(t);

    printf("\nScroll down 1 row\n");
    set_grid_rows(t);
    check_grid_rows(t, "ABCDEFGHIJKLMNOP"); 
    vt_term_scroll_down(t, 0, 1);
    print_grid(t);
    check_grid_rows(t, " ABCDEFGHIJKLMNO"); 

    printf("\nScroll down 2 rows\n");
    set_grid_rows(t);
    check_grid_rows(t, "ABCDEFGHIJKLMNOP"); 
    vt_term_scroll_down(t, 0, 2);
    print_grid(t);
    check_grid_rows(t, "  ABCDEFGHIJKLMN"); 

    printf("\nScroll down 100 rows\n");
    set_grid_rows(t);
    check_grid_rows(t, "ABCDEFGHIJKLMNOP"); 
    vt_term_scroll_down(t, 0, 100);
    print_grid(t);
    check_grid_blank(t);

    // Set top and bottom margins
    t->mt = 2;
    t->mb = t->h - 5;

    set_grid_rows(t);
    vt_term_scroll_up(t, t->mt, 1);
    print_grid(t);
    check_grid_rows(t, "ABDEFGHIJKL MNOP"); 

    set_grid_rows(t);
    vt_term_scroll_up(t, t->mt, 2);
    print_grid(t);
    check_grid_rows(t, "ABEFGHIJKL  MNOP"); 

    set_grid_rows(t);
    vt_term_scroll_up(t, t->mt, 10);
    print_grid(t);
    check_grid_rows(t, "AB          MNOP"); 

    set_grid_rows(t);
    vt_term_scroll_down(t, t->mt, 1);
    print_grid(t);
    check_grid_rows(t, "AB CDEFGHIJKMNOP"); 

    set_grid_rows(t);
    vt_term_scroll_down(t, t->mt, 2);
    print_grid(t);
    check_grid_rows(t, "AB  CDEFGHIJMNOP"); 

    set_grid_rows(t);
    vt_term_scroll_down(t, t->mt, 10);
    print_grid(t);
    check_grid_rows(t, "AB          MNOP"); 

    // Clear margins
    t->mt = 0;
    t->mb = t->h - 1;

}

void test_erase_in_display(vt_term_t *t) {
    // Erase in Display is a terminal control function (from ANSI escape codes, specifically ESC [ n J) that clears parts or all of the terminal screen, depending on the parameter n:
    //
    // n = 0: Erase from cursor to end of screen (including cursor position).
    // n = 1: Erase from start of screen to cursor (including cursor position).
    // n = 2: Erase entire screen.

    printf("\nErase in display [0]\n");
    set_grid_rows(t);
    t->r = 5; t->c = 7;
    check_grid_rows(t, "ABCDEFGHIJKLMNOP"); 
    vt_term_erase_in_display(t, 0);
    print_grid(t);
    check_grid_row(t, 0, "AAAAAAAAAAAAAAAAAAAA");
    check_grid_row(t, 1, "BBBBBBBBBBBBBBBBBBBB");
    check_grid_row(t, 2, "CCCCCCCCCCCCCCCCCCCC");
    check_grid_row(t, 3, "DDDDDDDDDDDDDDDDDDDD");
    check_grid_row(t, 4, "EEEEEEEEEEEEEEEEEEEE");
    check_grid_row(t, 5, "FFFFFFF             ");
    for(vt_coord_t r = 6; r < t->h; ++r)
        check_grid_row(t, r, "                    ");

    printf("\nErase in display [1]\n");
    set_grid_rows(t);
    t->r = 10; t->c = 7;
    check_grid_rows(t, "ABCDEFGHIJKLMNOP"); 
    vt_term_erase_in_display(t, 1);
    print_grid(t);
    for(vt_coord_t r = 0; r < 10; ++r) {
        check_grid_row(t, r, "                    ");
    }
    check_grid_row(t, 10, "       KKKKKKKKKKKKK");
    check_grid_row(t, 11, "LLLLLLLLLLLLLLLLLLLL");
    check_grid_row(t, 12, "MMMMMMMMMMMMMMMMMMMM");
    check_grid_row(t, 13, "NNNNNNNNNNNNNNNNNNNN");
    check_grid_row(t, 14, "OOOOOOOOOOOOOOOOOOOO");
    check_grid_row(t, 15, "PPPPPPPPPPPPPPPPPPPP");

    printf("\nErase in display [2]\n");
    set_grid_rows(t);
    check_grid_rows(t, "ABCDEFGHIJKLMNOP"); 
    vt_term_erase_in_display(t, 2);
    print_grid(t);
    check_grid_blank(t);
}

void test_insert_characters(vt_term_t *t) {
    t->r = 5;
    t->c = 0;
    set_grid_cols(t);
    vt_term_insert_characters(t, 3);
    print_grid(t);
    check_grid_row(t, 5, "   ABCDEFGHIJKLMNOPQ");

    t->c = 5;
    set_grid_cols(t);
    vt_term_insert_characters(t, 3);
    print_grid( t);
    check_grid_row(t, 5, "ABCDE   FGHIJKLMNOPQ");

    t->c = 5;
    set_grid_cols(t);
    vt_term_insert_characters(t, 300);
    print_grid( t);
    check_grid_row(t, 5, "ABCDE               ");

    t->c = t->w - 1;
    set_grid_cols(t);
    vt_term_insert_characters(t, 1);
    print_grid( t);
    check_grid_row(t, 5, "ABCDEFGHIJKLMNOPQRS ");
}

void test_delete_characters(vt_term_t *t) {
    t->r = 5;
    t->c = 0;
    set_grid_cols(t);
    vt_term_delete_characters(t, 3);
    print_grid(t);
    check_grid_row(t, 5, "DEFGHIJKLMNOPQRST   ");

    t->c = 5;
    set_grid_cols(t);
    vt_term_delete_characters(t, 3);
    print_grid( t);
    check_grid_row(t, 5, "ABCDEIJKLMNOPQRST   ");

    t->c = 5;
    set_grid_cols(t);
    vt_term_delete_characters(t, 300);
    print_grid( t);
    check_grid_row(t, 5, "ABCDE               ");

    t->c = t->w - 1;
    set_grid_cols(t);
    vt_term_delete_characters(t, 1);
    print_grid( t);
    check_grid_row(t, 5, "ABCDEFGHIJKLMNOPQRS ");

    // TODO Check attributes
    /* VT102 manual says the attribute for the newly empty characters
     * should be the same as the last character moved left, which isn't
     * what clearline() currently does.
     */
}

void test_erase_in_line(vt_term_t *t) {

    t->r = 5;
    t->c = 7;
    set_grid_cols(t);
    vt_term_erase_in_line(t, 0);
    print_grid(t);
    check_grid_row(t, 5, "ABCDEFG             ");

    set_grid_cols(t);
    vt_term_erase_in_line(t, 1);
    print_grid(t);
    check_grid_row(t, 5, "        IJKLMNOPQRST");

    set_grid_cols(t);
    vt_term_erase_in_line(t, 2);
    print_grid(t);
    check_grid_row(t, 5, "                    ");
}

void test_putch(vt_term_t *t) {
    printf("\nTesting putch no scroll\n");
    vt_term_clear_screen(t);
    t->r = 5;t->c = 7;
    vt_term_putch(t, 'A');
    vt_term_putch(t, 'B');
    vt_term_putch(t, 'C');
    print_grid(t);
    for(vt_coord_t r = 0; r < t->h; ++r) {
        if(r != 5) check_grid_row(t, r, "                    ");
    }
    check_grid_row(t, 5, "       ABC          ");

    printf("\nTesting putch wrap\n");
    vt_term_clear_screen(t);
    t->r = 5;t->c = t->w - 1;
    vt_term_putch(t, 'A');
    vt_term_putch(t, 'B');
    vt_term_putch(t, 'C');
    print_grid(t);
    for(vt_coord_t r = 0; r < t->h; ++r) {
        if(r != 5 && r != 6) check_grid_row(t, r, "                    ");
    }
    check_grid_row(t, 5, "                   A");
    check_grid_row(t, 6, "BC                  ");

    printf("\nTesting scroll hang\n");
    vt_term_clear_screen(t);
    t->r = t->h - 1;t->c = t->w - 1;
    vt_term_putch(t, 'A');
    print_grid(t);
    for(vt_coord_t r = 0; r < (t->h - 1); ++r) {
        check_grid_row(t, r, "                    ");
    }
    vt_term_putch(t, 'B');
    vt_term_putch(t, 'C');
    print_grid(t);


    printf("\nTesting scroll hang with margins\n");
    // Set top and bottom margins
    t->mt = 2;
    t->mb = t->h - 5;
    t->r = t->mb; 
    t->c = t->w - 1;

    set_grid_rows(t);
    vt_term_putch(t, 'x');
    print_grid(t);
    check_grid_rows(t, "ABCDEFGHIJK?MNOP");
    check_grid_row(t, 11, "LLLLLLLLLLLLLLLLLLLx");
    vt_term_putch(t, 'y');
    vt_term_putch(t, 'z');
    print_grid(t);
    check_grid_rows(t, "ABDEFGHIJK??MNOP"); 
    check_grid_row(t, 10, "LLLLLLLLLLLLLLLLLLLx");
    check_grid_row(t, 11, "yz                  ");

    // Clear margins
    t->mt = 0;
    t->mb = t->h - 1;
}

void test_reverse_nl(vt_term_t *t) {

    printf("\nTesting reverse nl margins\n");
    // Set top and bottom margins
    t->mt = 2;
    t->mb = t->h - 5;

    t->r = t->mt; 
    t->c = t->w - 1;
    set_grid_rows(t);
    vt_term_putch(t, 'x');
    printf("hang = %d\n", t->hang);
    vt_term_reverse_nl(t);
    print_grid(t);
    check_grid_rows(t, "AB?DEFGHIJKLMNOP");
    check_grid_row(t, 2, "CCCCCCCCCCCCCCCCCCCx");
    vt_term_putch(t, 'y');
    vt_term_putch(t, 'z');
    print_grid(t);
    check_grid_rows(t, "AB?DEFGHIJKLMNOP"); 
    check_grid_row(t, 2, "yzCCCCCCCCCCCCCCCCCx");

    t->r = t->mt; 
    t->c = 2;
    set_grid_rows(t);
    vt_term_putch(t, 'x');
    vt_term_reverse_nl(t);
    print_grid(t);
    vt_term_putch(t, 'y');
    vt_term_putch(t, 'z');
    print_grid(t);
    check_grid_rows(t, "AB??DEFGHIJKMNOP");
    check_grid_row(t, 2, "   yz               ");
    check_grid_row(t, 3, "CCxCCCCCCCCCCCCCCCCC");

    // Clear margins
    t->mt = 0;
    t->mb = t->h - 1;
}

void test_nl(vt_term_t *t) {
    printf("\nTesting nl margins\n");
    // Set top and bottom margins
    t->mt = 2;
    t->mb = t->h - 5;

    t->r = t->mb; 
    t->c = t->w - 1;
    set_grid_rows(t);
    vt_term_putch(t, 'x');
    printf("hang = %d\n", t->hang);
    vt_term_nl(t);
    print_grid(t);
    printf("hang = %d\n", t->hang);
    assert(t->hang == 0);
    check_grid_rows(t, "ABDEFGHIJK? MNOP");
    check_grid_row(t, 10, "LLLLLLLLLLLLLLLLLLLx");
    vt_term_putch(t, 'y');
    vt_term_putch(t, 'z');
    print_grid(t);
    check_grid_rows(t, "ABDEFGHIJK??MNOP");
    check_grid_row(t, 10, "LLLLLLLLLLLLLLLLLLLx");
    check_grid_row(t, 11, "yz                  ");

    t->r = t->mt; 
    t->c = 2;
    set_grid_rows(t);
    vt_term_putch(t, 'x');
    vt_term_nl(t);
    print_grid(t);
    vt_term_putch(t, 'y');
    vt_term_putch(t, 'z');
    print_grid(t);
    check_grid_rows(t, "AB??EFGHIJKLMNOP");
    check_grid_row(t, 2, "CCxCCCCCCCCCCCCCCCCC");
    check_grid_row(t, 3, "DDDyzDDDDDDDDDDDDDDD");

    // Clear margins
    t->mt = 0;
    t->mb = t->h - 1;
}

void test_cr(vt_term_t *t) {
    printf("\nTesting cr margins\n");
    // Set top and bottom margins
    t->mt = 2;
    t->mb = t->h - 5;

    t->r = t->mb; 
    t->c = t->w - 1;
    set_grid_rows(t);
    vt_term_putch(t, 'x');
    printf("hang = %d\n", t->hang);
    vt_term_cr(t);
    print_grid(t);
    assert(t->hang == VT_HANG_BOTTOM);
    check_grid_rows(t, "ABCDEFGHIJK?MNOP");
    check_grid_row(t, 11, "LLLLLLLLLLLLLLLLLLLx");
    vt_term_putch(t, 'y');
    vt_term_putch(t, 'z');
    print_grid(t);
    check_grid_rows(t, "ABDEFGHIJK??MNOP"); 
    check_grid_row(t, 10, "LLLLLLLLLLLLLLLLLLLx");
    check_grid_row(t, 11, "yz                  ");

    t->r = t->mb; 
    t->c = 0;
    set_grid_rows(t);
    vt_term_putch(t, 'x');
    vt_term_cr(t);
    print_grid(t);
    check_grid_rows(t, "ABCDEFGHIJK?MNOP");
    check_grid_row(t, 11, "xLLLLLLLLLLLLLLLLLLL");
    vt_term_putch(t, 'y');
    vt_term_putch(t, 'z');
    print_grid(t);
    check_grid_rows(t, "ABCDEFGHIJK?MNOP");
    check_grid_row(t, 11, "yzLLLLLLLLLLLLLLLLLL");

    // Clear margins
    t->mt = 0;
    t->mb = t->h - 1;
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

    // Check the screen is blank on start-up
    printf("Initial display check\n");
    print_grid(&t);
    check_grid_blank(&t);

    test_scroll(&t);
    test_erase_in_display(&t);
    test_insert_characters(&t);
    test_delete_characters(&t);
    test_erase_in_line(&t);
    test_putch(&t);
    test_reverse_nl(&t);
    test_nl(&t);
    test_cr(&t);

    printf("all ok\n");
    return 0;
}   

