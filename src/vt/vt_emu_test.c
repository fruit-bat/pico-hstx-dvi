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
 * ./vt_emu_test.sh
 * 

Shell 1)

fifo=/tmp/myfifo
rm -f "$fifo"
mkfifo "$fifo"
pty=$(socat -d -d PTY,raw,echo=0 FILE:"$fifo",ignoreeof 2>&1 | \
awk -F' ' '/PTY is/ {print $NF; exit}')
echo "Run: tio $pty"

Shell 2)
a.out < /tmp/myfifo

Shell 3)
tio /tmp/myfifo

* 
 */
#include "vt_emu.h"
#include "vt_term_test_utils.h"
#include <stdio.h>
#include <assert.h>

// TODO
// ESC[6n	request cursor position (reports as ESC[#;#R)

// ESC[H	moves cursor to home position (0, 0)
// ESC[{line};{column}H moves cursor to line #, column #
// ESC[{line};{column}f	moves cursor to line #, column #
// ESC[#A	moves cursor up # lines
// ESC[#B	moves cursor down # lines
// ESC[#C	moves cursor right # columns
// ESC[#D	moves cursor left # columns
// ESC[#E	moves cursor to beginning of next line, # lines down
// ESC[#F	moves cursor to beginning of previous line, # lines up
// ESC[#G	moves cursor to column #
// ESC M	moves cursor one line up, scrolling if needed
// ESC 7	save cursor position (DEC)
// ESC 8	restores the cursor to the last saved position (DEC)
// ESC[s	save cursor position (SCO)
// ESC[u	restores the cursor to the last saved position (SCO)
void test_cursor(vt_emu_t* e) {
    vt_term_t* const t = &e->term;

    vt_emu_reset(e);
    assert(t->r == 0);
    assert(t->c == 0);
    // ESC[{line};{column}H moves cursor to line #, column #
    vt_emu_put_str(e, (vt_char_t*)"\033[4;5H");
    assert(t->r == 4);
    assert(t->c == 5);
    // ESC[{line};{column}f	moves cursor to line #, column #
    vt_emu_put_str(e, (vt_char_t*)"\033[7;13f");
    assert(t->r == 7);
    assert(t->c == 13);
    // ESC[H	moves cursor to home position (0, 0)
    vt_emu_put_str(e, (vt_char_t*)"\033[H");
    assert(t->r == 0);
    assert(t->c == 0);
    // ESC[#B	moves cursor down # lines
    vt_emu_put_str(e, (vt_char_t*)"\033[7B");
    assert(t->r == 7);
    assert(t->c == 0);
    // ESC[#C	moves cursor right # columns
    vt_emu_put_str(e, (vt_char_t*)"\033[13C");
    assert(t->r == 7);
    assert(t->c == 13);
    // ESC[#A	moves cursor up # lines
    vt_emu_put_str(e, (vt_char_t*)"\033[3A");
    assert(t->r == 4);
    assert(t->c == 13);
    // ESC[#D	moves cursor left # columns
    vt_emu_put_str(e, (vt_char_t*)"\033[8D");
    assert(t->r == 4);
    assert(t->c == 5);
    // ESC[#E	moves cursor to beginning of next line, # lines down
    vt_emu_put_str(e, (vt_char_t*)"\033[E");
    assert(t->r == 5);
    assert(t->c == 0);
    // ESC[#E	moves cursor to beginning of next line, # lines down
    vt_emu_put_str(e, (vt_char_t*)"\033[2E");
    assert(t->r == 7);
    assert(t->c == 0);
    // ESC[#F	moves cursor to beginning of previous line, # lines up
    vt_emu_put_str(e, (vt_char_t*)"\033[F");
    assert(t->r == 6);
    assert(t->c == 0);
    // ESC[#F	moves cursor to beginning of previous line, # lines up
    vt_emu_put_str(e, (vt_char_t*)"\033[2F");
    assert(t->r == 4);
    assert(t->c == 0);
    // ESC M	moves cursor one line up, scrolling if needed
    vt_emu_put_str(e, (vt_char_t*)"\033M");
    assert(t->r == 3);
    assert(t->c == 0);
    // Change horizontal attribute
    vt_emu_put_str(e, (vt_char_t*)"\033[11G");
    assert(t->r == 3);
    assert(t->c == 11);
    // Save & Restore ESC7, ESC8
    assert(t->r == 3);
    assert(t->c == 11);
    vt_emu_put_str(e, (vt_char_t*)"\0337");
    vt_emu_put_str(e, (vt_char_t*)"\033[4;5H");
    assert(t->r == 4);
    assert(t->c == 5);
    vt_emu_put_str(e, (vt_char_t*)"\0338");
    assert(t->r == 3);
    assert(t->c == 11);
    // Save & Restore ESC[s, ESC[u]
    assert(t->r == 3);
    assert(t->c == 11);
    vt_emu_put_str(e, (vt_char_t*)"\033[s");
    vt_emu_put_str(e, (vt_char_t*)"\033[4;5H");
    assert(t->r == 4);
    assert(t->c == 5);
    vt_emu_put_str(e, (vt_char_t*)"\033[u");
    assert(t->r == 3);
    assert(t->c == 11);
}

// ESC[J	erase in display (same as ESC[0J)
// ESC[0J	erase from cursor until end of screen
// ESC[1J	erase from cursor to beginning of screen
// ESC[2J	erase entire screen
// ESC[3J	erase saved lines
// ESC[K	erase in line (same as ESC[0K)
// ESC[0K	erase from cursor to end of line
// ESC[1K	erase start of line to the cursor
// ESC[2K	erase the entire line
void test_erase(vt_emu_t* e) {
    vt_term_t* const t = &e->term;
    vt_emu_reset(e);
    
    set_grid_cols(t);
    print_grid(t);
    check_grid_row(t, 4, "ABCDEFGHIJKLMNOPQRST");
    // ESC[{line};{column}H moves cursor to line #, column #
    // ESC[J	erase in display (same as ESC[0J)
    vt_emu_put_str(e, (vt_char_t*)"\033[4;5H\033[J");
    print_grid(t);
    for(vt_coord_t r = 0; r < t->h; ++r) {
        if (r <  4) check_grid_row(t, r, "ABCDEFGHIJKLMNOPQRST");
        if (r == 4) check_grid_row(t, r, "ABCDE               ");
        if (r >  4) check_grid_row(t, r, "                    ");
    }

    set_grid_cols(t);
    print_grid(t);
    check_grid_row(t, 4, "ABCDEFGHIJKLMNOPQRST");
    // ESC[{line};{column}H moves cursor to line #, column #
    // ESC[J	erase in display (same as ESC[0J)
    vt_emu_put_str(e, (vt_char_t*)"\033[4;5H\033[0J");
    print_grid(t);
    for(vt_coord_t r = 0; r < t->h; ++r) {
        if (r <  4) check_grid_row(t, r, "ABCDEFGHIJKLMNOPQRST");
        if (r == 4) check_grid_row(t, r, "ABCDE               ");
        if (r >  4) check_grid_row(t, r, "                    ");
    }

    set_grid_cols(t);
    print_grid(t);
    check_grid_row(t, 4, "ABCDEFGHIJKLMNOPQRST");
    // ESC[{line};{column}H moves cursor to line #, column #
    // ESC[1J	erase from cursor to beginning of screen
    vt_emu_put_str(e, (vt_char_t*)"\033[4;5H\033[1J");
    print_grid(t);
    for(vt_coord_t r = 0; r < t->h; ++r) {
        if (r <  4) check_grid_row(t, r, "                    ");
        if (r == 4) check_grid_row(t, r, "      GHIJKLMNOPQRST");
        if (r >  4) check_grid_row(t, r, "ABCDEFGHIJKLMNOPQRST");
    }

    set_grid_cols(t);
    print_grid(t);
    check_grid_row(t, 4, "ABCDEFGHIJKLMNOPQRST");
    // ESC[{line};{column}H moves cursor to line #, column #
    // ESC[2J	erase entire screen
    vt_emu_put_str(e, (vt_char_t*)"\033[4;5H\033[2J");
    print_grid(t);
    for(vt_coord_t r = 0; r < t->h; ++r) {
        check_grid_row(t, r, "                    ");
    }

    set_grid_cols(t);
    print_grid(t);
    check_grid_row(t, 4, "ABCDEFGHIJKLMNOPQRST");
    // ESC[{line};{column}H moves cursor to line #, column #
    // ESC[2J	erase entire screen
    vt_emu_put_str(e, (vt_char_t*)"\033[4;5H\033[3J");
    print_grid(t);
    for(vt_coord_t r = 0; r < t->h; ++r) {
        check_grid_row(t, r, "                    ");
    }



    set_grid_cols(t);
    print_grid(t);
    check_grid_row(t, 4, "ABCDEFGHIJKLMNOPQRST");
    // ESC[{line};{column}H moves cursor to line #, column #
    // ESC[K	erase in line (same as ESC[0K)
    vt_emu_put_str(e, (vt_char_t*)"\033[4;5H\033[K");
    print_grid(t);
    for(vt_coord_t r = 0; r < t->h; ++r) {
        if (r <  4) check_grid_row(t, r, "ABCDEFGHIJKLMNOPQRST");
        if (r == 4) check_grid_row(t, r, "ABCDE               ");
        if (r >  4) check_grid_row(t, r, "ABCDEFGHIJKLMNOPQRST");
    }

    set_grid_cols(t);
    print_grid(t);
    check_grid_row(t, 4, "ABCDEFGHIJKLMNOPQRST");
    // ESC[{line};{column}H moves cursor to line #, column #
    // ESC[0K	erase from cursor to end of line
    vt_emu_put_str(e, (vt_char_t*)"\033[4;5H\033[0K");
    print_grid(t);
    for(vt_coord_t r = 0; r < t->h; ++r) {
        if (r <  4) check_grid_row(t, r, "ABCDEFGHIJKLMNOPQRST");
        if (r == 4) check_grid_row(t, r, "ABCDE               ");
        if (r >  4) check_grid_row(t, r, "ABCDEFGHIJKLMNOPQRST");
    }

    set_grid_cols(t);
    print_grid(t);
    check_grid_row(t, 4, "ABCDEFGHIJKLMNOPQRST");
    // ESC[{line};{column}H moves cursor to line #, column #
    // ESC[1K	erase start of line to the cursor
    vt_emu_put_str(e, (vt_char_t*)"\033[4;5H\033[1K");
    print_grid(t);
    for(vt_coord_t r = 0; r < t->h; ++r) {
        if (r <  4) check_grid_row(t, r, "ABCDEFGHIJKLMNOPQRST");
        if (r == 4) check_grid_row(t, r, "      GHIJKLMNOPQRST");
        if (r >  4) check_grid_row(t, r, "ABCDEFGHIJKLMNOPQRST");
    }

    set_grid_cols(t);
    print_grid(t);
    check_grid_row(t, 4, "ABCDEFGHIJKLMNOPQRST");
    // ESC[{line};{column}H moves cursor to line #, column #
    // ESC[2K	erase the entire line
    vt_emu_put_str(e, (vt_char_t*)"\033[4;5H\033[2K");
    print_grid(t);
    for(vt_coord_t r = 0; r < t->h; ++r) {
        if (r <  4) check_grid_row(t, r, "ABCDEFGHIJKLMNOPQRST");
        if (r == 4) check_grid_row(t, r, "                    ");
        if (r >  4) check_grid_row(t, r, "ABCDEFGHIJKLMNOPQRST");
    }
}

// ESC[1;34;{...}m		Set graphics modes for cell, separated by semicolon (;).
// ESC[0m		reset all modes (styles and colors)
// ESC[1m	ESC[22m	set bold mode.
// ESC[2m	ESC[22m	set dim/faint mode.
// ESC[3m	ESC[23m	set italic mode.
// ESC[4m	ESC[24m	set underline mode.
// ESC[5m	ESC[25m	set blinking mode
// ESC[7m	ESC[27m	set inverse/reverse mode
// ESC[8m	ESC[28m	set hidden/invisible mode
// ESC[9m	ESC[29m	set strikethrough mode.
void test_cell_modes(vt_emu_t* e) {
    vt_term_t* const t = &e->term;
    vt_emu_reset(e);
    uint8_t r1[] = {
        0b00000000,
        0b00000001,
        0b00000010,
        0b00000110,
        0b00001110,
        0b00011110,
        0b00011110,
        0b00111110,
        0b01111110,
        0b11111110,
    };
    for(int i = 0; i <= 9; ++i) {
        vt_emu_put_ch(e, 27);
        vt_emu_put_ch(e, '[');
        vt_emu_put_ch(e, '0' + i);
        vt_emu_put_ch(e, 'm');
        vt_emu_put_ch(e, 'a' + i);
        printf("Attr set test %d, expecting attribute flags: %08b\n", i, r1[i]);
        print_grid(t);
        const vt_cell_attr_t attr = vt_cell_get_attr(t->rp[0][i]);
        const vt_cell_flags_t flags = vt_cell_flags_get(attr);
        assert(flags == r1[i]);
    }
    vt_emu_put_ch(e, 13);
    vt_emu_put_ch(e, 10);
    uint8_t r2[] = {
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000010,
        0b00000110,
        0b00001110,
        0b00011110,
        0b00011110,
        0b00111110,
        0b01111110,
    };
    for(int i = 9; i >= 0; --i) {
        vt_emu_put_ch(e, 27);
        vt_emu_put_ch(e, '[');
        vt_emu_put_ch(e, '2');
        vt_emu_put_ch(e, '0' + i);
        vt_emu_put_ch(e, 'm');
        vt_emu_put_ch(e, 'a' + i);
        printf("Attr clear test %d, expecting attribute flags: %08b\n", i, r2[i]);
        print_grid(t);
        const vt_cell_attr_t attr = vt_cell_get_attr(t->rp[1][9 - i]);
        const vt_cell_flags_t flags = vt_cell_flags_get(attr);
        assert(flags == r2[i]);
    }
}


void test_colours(vt_emu_t* e) {
    vt_term_t* const t = &e->term;
    vt_emu_reset(e);
    // Color Name	Foreground Color Code	Background Color Code
    // Black        30	                    40
    // Red	        31	                    41
    // Green	    32	                    42
    // Yellow	    33	                    43
    // Blue	        34	                    44
    // Magenta	    35	                    45
    // Cyan	        36	                    46
    // White	    37	                    47
    // Default	    39	                    49
    uint8_t r1[] = {0, 1, 2, 3, 4, 5, 6, 7};
    for(int i = 0; i <= 7; ++i) {
        vt_emu_put_ch(e, 27);
        vt_emu_put_ch(e, '[');
        vt_emu_put_ch(e, '3');
        vt_emu_put_ch(e, '0' + i);
        vt_emu_put_ch(e, 'm');
        vt_emu_put_ch(e, 'a' + i);
        printf("Foreground colour test %d, expecting colour: %08b\n", i, r1[i]);
        print_grid(t);
        const vt_cell_attr_t attr = vt_cell_get_attr(t->rp[0][i]);
        const vt_cell_colour_t ci = vt_cell_fg_get(attr);
        assert(ci == r1[i]);
    }
    vt_emu_put_ch(e, 13);
    vt_emu_put_ch(e, 10);
    for(int i = 0; i <= 7; ++i) {
        vt_emu_put_ch(e, 27);
        vt_emu_put_ch(e, '[');
        vt_emu_put_ch(e, '4');
        vt_emu_put_ch(e, '0' + i);
        vt_emu_put_ch(e, 'm');
        vt_emu_put_ch(e, 'a' + i);
        printf("Background colour test %d, expecting colour: %08b\n", i, r1[i]);
        print_grid(t);
        const vt_cell_attr_t attr = vt_cell_get_attr(t->rp[1][i]);
        const vt_cell_colour_t ci = vt_cell_bg_get(attr);
        assert(ci == r1[i]);
    }
    // Color Name	    Foreground Color Code	Background Color Code
    // Bright Black	    90	                    100
    // Bright Red	    91	                    101
    // Bright Green	    92	                    102
    // Bright Yellow	93	                    103
    // Bright Blue	    94	                    104
    // Bright Magenta	95	                    105
    // Bright Cyan	    96	                    106
    // Bright White	    97	                    107
    vt_emu_put_ch(e, 13);
    vt_emu_put_ch(e, 10);
    uint8_t r2[] = {8, 9, 10, 11, 12, 13, 14, 15};
    for(int i = 0; i <= 7; ++i) {
        vt_emu_put_ch(e, 27);
        vt_emu_put_ch(e, '[');
        vt_emu_put_ch(e, '9');
        vt_emu_put_ch(e, '0' + i);
        vt_emu_put_ch(e, 'm');
        vt_emu_put_ch(e, 'a' + i);
        printf("Foreground bright colour test %d, expecting colour: %08b\n", i, r2[i]);
        print_grid(t);
        const vt_cell_attr_t attr = vt_cell_get_attr(t->rp[2][i]);
        const vt_cell_colour_t ci = vt_cell_fg_get(attr);
        assert(ci == r2[i]);
    }
    vt_emu_put_ch(e, 13);
    vt_emu_put_ch(e, 10);
    for(int i = 0; i <= 7; ++i) {
        vt_emu_put_ch(e, 27);
        vt_emu_put_ch(e, '[');
        vt_emu_put_ch(e, '1');
        vt_emu_put_ch(e, '0');
        vt_emu_put_ch(e, '0' + i);
        vt_emu_put_ch(e, 'm');
        vt_emu_put_ch(e, 'a' + i);
        printf("Background bright colour test %d, expecting colour: %08b\n", i, r2[i]);
        print_grid(t);
        const vt_cell_attr_t attr = vt_cell_get_attr(t->rp[3][i]);
        const vt_cell_colour_t ci = vt_cell_bg_get(attr);
        assert(ci == r2[i]);
    }

    // ESC Code Sequence	Description
    // ESC[38;5;{ID}m	Set foreground color.
    // ESC[48;5;{ID}m	Set background color.
    vt_emu_reset(e);
    char buf[32];
    for(int i = 0; i <= 15; ++i) {
        for(int j = 0; j <= 15; ++j) {
            vt_cell_colour_t fg = i * 16 + j;
            vt_cell_colour_t bg = 255 - fg;
            snprintf(&buf, 32, "\033[38;5;%d;48;5;%dm%c", fg, bg, 'a' + i );
            vt_emu_put_str(e, (vt_char_t*)&buf);
            const vt_cell_attr_t attr = vt_cell_get_attr(t->rp[0][j]);
            const vt_cell_colour_t fgci = vt_cell_fg_get(attr);
            const vt_cell_colour_t bgci = vt_cell_bg_get(attr);
            assert(fg == fgci);
            assert(bg == bgci);
        }
        vt_emu_put_ch(e, 13);
    }
}

// ESC[@
void test_insert(vt_emu_t* e) {
    vt_term_t* const t = &e->term;
    vt_emu_reset(e);

    t->c = 5;
    set_grid_cols(t);
    vt_emu_put_str(e, (vt_char_t*)"\033[3@");
    print_grid( t);
    check_grid_row(t, 0, "ABCDE   FGHIJKLMNOPQ");
}

void test_scroll(vt_emu_t* e) {
    vt_term_t* const t = &e->term;
    vt_emu_reset(e);
}

void test_stdin(vt_emu_t* e) {
    vt_term_t* const t = &e->term;
    setvbuf(stdin, NULL, _IONBF, 0);

    char ch;
    while((ch = getchar()) != EOF) {
        printf("character %d\n", ch);
        vt_emu_put_ch(e, ch);
        print_grid(t);
    }
}

int main() {

    const vt_coord_t w = 20;
    const vt_coord_t h = 16;
    vt_cell_t grid[h][w];
    vt_emu_t e;
    vt_emu_init(&e, (vt_cell_t*)grid, w, h);

    // test_cursor(&e);
    // test_erase(&e);
    // test_cell_modes(&e);
    // test_colours(&e);
    test_insert(&e);
    // test_stdin(&e);

    printf("All OK\n");
}
