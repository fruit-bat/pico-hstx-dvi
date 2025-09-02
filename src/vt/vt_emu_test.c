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
// ESC[6n	request cursor position (reports as ESC[#;#R)
// ESC M	moves cursor one line up, scrolling if needed
// ESC 7	save cursor position (DEC)
// ESC 8	restores the cursor to the last saved position (DEC)
// ESC[s	save cursor position (SCO)
// ESC[u	restores the cursor to the last saved position (SCO)
void test_cursor(vt_emu_t* e) {
    vt_emu_reset(e);
    assert(e->term.r == 0);
    assert(e->term.c == 0);
    // ESC[{line};{column}H moves cursor to line #, column #
    vt_emu_put_str(e, (vt_char_t*)"\033[4;5H");
    assert(e->term.r == 4);
    assert(e->term.c == 5);
    // ESC[{line};{column}f	moves cursor to line #, column #
    vt_emu_put_str(e, (vt_char_t*)"\033[7;13f");
    assert(e->term.r == 7);
    assert(e->term.c == 13);
    // ESC[H	moves cursor to home position (0, 0)
    vt_emu_put_str(e, (vt_char_t*)"\033[H");
    assert(e->term.r == 0);
    assert(e->term.c == 0);
    // ESC[#B	moves cursor down # lines
    vt_emu_put_str(e, (vt_char_t*)"\033[7B");
    assert(e->term.r == 7);
    assert(e->term.c == 0);
    // ESC[#C	moves cursor right # columns
    vt_emu_put_str(e, (vt_char_t*)"\033[13C");
    assert(e->term.r == 7);
    assert(e->term.c == 13);
    // ESC[#A	moves cursor up # lines
    vt_emu_put_str(e, (vt_char_t*)"\033[3A");
    assert(e->term.r == 4);
    assert(e->term.c == 13);
    // ESC[#D	moves cursor left # columns
    vt_emu_put_str(e, (vt_char_t*)"\033[8D");
    assert(e->term.r == 4);
    assert(e->term.c == 5);
    // ESC[#E	moves cursor to beginning of next line, # lines down
    vt_emu_put_str(e, (vt_char_t*)"\033[E");
    assert(e->term.r == 5);
    assert(e->term.c == 0);
    // ESC[#E	moves cursor to beginning of next line, # lines down
    vt_emu_put_str(e, (vt_char_t*)"\033[2E");
    assert(e->term.r == 7);
    assert(e->term.c == 0);
    // ESC[#F	moves cursor to beginning of previous line, # lines up
    vt_emu_put_str(e, (vt_char_t*)"\033[F");
    assert(e->term.r == 6);
    assert(e->term.c == 0);
    // ESC[#F	moves cursor to beginning of previous line, # lines up
    vt_emu_put_str(e, (vt_char_t*)"\033[2F");
    assert(e->term.r == 4);
    assert(e->term.c == 0);
    // ESC M	moves cursor one line up, scrolling if needed
    vt_emu_put_str(e, (vt_char_t*)"\033M");
    assert(e->term.r == 3);
    assert(e->term.c == 0);
    // Change horizontal attribute
    vt_emu_put_str(e, (vt_char_t*)"\033[11G");
    assert(e->term.r == 3);
    assert(e->term.c == 11);
    // Save & Restore ESC7, ESC8
    assert(e->term.r == 3);
    assert(e->term.c == 11);
    vt_emu_put_str(e, (vt_char_t*)"\0337");
    vt_emu_put_str(e, (vt_char_t*)"\033[4;5H");
    assert(e->term.r == 4);
    assert(e->term.c == 5);
    vt_emu_put_str(e, (vt_char_t*)"\0338");
    assert(e->term.r == 3);
    assert(e->term.c == 11);
    // Save & Restore ESC[s, ESC[u]
    assert(e->term.r == 3);
    assert(e->term.c == 11);
    vt_emu_put_str(e, (vt_char_t*)"\033[s");
    vt_emu_put_str(e, (vt_char_t*)"\033[4;5H");
    assert(e->term.r == 4);
    assert(e->term.c == 5);
    vt_emu_put_str(e, (vt_char_t*)"\033[u");
    assert(e->term.r == 3);
    assert(e->term.c == 11);

}

void test_stdin(vt_emu_t* e) {
    setvbuf(stdin, NULL, _IONBF, 0);

    char ch;
    while((ch = getchar()) != EOF) {
        printf("character %d\n", ch);
        vt_emu_put_ch(e, ch);
        print_grid(&e->term);
    }
}

int main() {

    const vt_coord_t w = 20;
    const vt_coord_t h = 16;
    vt_cell_t grid[h][w];
    vt_emu_t e;
    vt_emu_init(&e, (vt_cell_t*)grid, w, h);

    test_cursor(&e);
    // test_stdin(&e);

    printf("All OK\n");
}
