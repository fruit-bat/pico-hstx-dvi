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
 * cc -Wall vt_ansi_seq.c vt_ansi_seq_test.c 
 * 
 * ./a.out
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "vt_ansi_seq.h"
#include <string.h>
#include <assert.h>

int main() {
    vt_parser_t p;
    vt_parser_init(&p);

    assert(sizeof(vt_state_t) == 4);

    {
        const vt_state_t* s = vt_parser_put_ch(&p, 'D'); // D
        assert(s != NULL);
        assert(s->n == VT_A_CHAR);
        assert(s->f & VT_F_FINAL);
        assert(p.ch == 'D');
    }
    {
        const vt_state_t* s = vt_parser_put_ch(&p, 0x1B); // ESC
        assert(s != NULL);
        assert((s->f & VT_F_FINAL) == 0);
    }
    {
        const vt_state_t* s = vt_parser_put_ch(&p, 'D'); // D
        assert(s != NULL);
        assert(s->m == 'D');
        assert(s->f & VT_F_FINAL);
        assert(s->n == VT_A_IND);
    }
    {
        vt_parser_put_ch(&p, 0x1B); // ESC
        vt_parser_put_ch(&p, '[');  // [
        const vt_state_t* s = vt_parser_put_ch(&p, 's');  // s
        assert(s != NULL);
        assert(s->m == 's');
        assert(s->f & VT_F_FINAL);
        assert(s->n == VT_A_SAVE_CUR);
    }
    {
        const vt_state_t* s = vt_parser_put_str(&p, (vt_char_t*)"\033[s"); // ESC
        assert(s != NULL);
        assert(s->m == 's');
        assert(s->f & VT_F_FINAL);
        assert(s->n == VT_A_SAVE_CUR);
    }
    {
        const vt_state_t* s = vt_parser_put_str(&p, (vt_char_t*)"\033[123m"); // ESC
        assert(s != NULL);
        assert(s->m == 'm');
        assert(s->f & VT_F_FINAL);
        assert(s->n == VT_A_SGR);
        assert(p.n_params == 1);
        assert(p.params[0] == 123);
    }
    {
        const vt_state_t* s = vt_parser_put_str(&p, (vt_char_t*)"\033[456m"); // ESC
        assert(s != NULL);
        assert(s->m == 'm');
        assert(s->f & VT_F_FINAL);
        assert(s->n == VT_A_SGR);
        assert(p.n_params == 1);
        assert(p.params[0] == 456);
    }
    {
        const vt_state_t* s = vt_parser_put_str(&p, (vt_char_t*)"\033[1;2;3;4;5;0m"); // ESC
        assert(s != NULL);
        assert(s->m == 'm');
        assert(s->f & VT_F_FINAL);
        assert(s->n == VT_A_SGR);
        assert(p.n_params == 6);
        assert(p.params[0] == 1);
        assert(p.params[1] == 2);
        assert(p.params[2] == 3);
        assert(p.params[3] == 4);
        assert(p.params[4] == 5);
        assert(p.params[5] == 0);
    }
    {
        const vt_state_t* s = vt_parser_put_str(&p, (vt_char_t*)"\033[m"); // ESC
        assert(s != NULL);
        assert(s->m == 'm');
        assert(s->f & VT_F_FINAL);
        assert(s->n == VT_A_SGR);
        assert(p.n_params == 0);
    }
    {
        const vt_state_t* s = vt_parser_put_str(&p, (vt_char_t*)"\033[5;m"); // ESC
        assert(s != NULL);
        assert(s->m == 'm');
        assert(s->f & VT_F_FINAL);
        assert(s->n == VT_A_SGR);
        assert(p.n_params == 2);
        assert(p.params[0] == 5);
        assert(p.params[1] == 0);
    }
    {
        const vt_state_t* s = vt_parser_put_str(&p, (vt_char_t*)"\033[;m"); // ESC
        assert(s != NULL);
        assert(s->m == 'm');
        assert(s->f & VT_F_FINAL);
        assert(s->n == VT_A_SGR);
        assert(p.n_params == 2);
        assert(p.params[0] == 0);
        assert(p.params[1] == 0);
    }

    // Test a list of example sequences
    assert(vt_parser_put_ch(&p, (vt_char_t)'\x00')->n == VT_A_C0_NULL);
    
    // C0 codes
    //
    // See: https://en.wikipedia.org/wiki/ANSI_escape_code
    //
    // ^G	0x07	BEL	Bell	Makes an audible noise.
    // ^H	0x08	BS	Backspace	Moves the cursor left (but may "backwards wrap" if cursor is at start of line).
    // ^I	0x09	HT	Tab	Moves the cursor right to next tab stop.
    // ^J	0x0A	LF	Line Feed	Moves to next line, scrolls the display up if at bottom of the screen. Usually does not move horizontally, though programs should not rely on this.
    // ^L	0x0C	FF	Form Feed	Move a printer to top of next page. Usually does not move horizontally, though programs should not rely on this. Effect on video terminals varies.
    // ^M	0x0D	CR	Carriage Return	Moves the cursor to column zero.
    // ^[	0x1B	ESC	Escape	Starts all the escape sequences
    assert(vt_parser_put_ch(&p, (vt_char_t)'\x07')->n == VT_A_C0_BEL);
    assert(vt_parser_put_ch(&p, (vt_char_t)'\x08')->n == VT_A_C0_BS);
    assert(vt_parser_put_ch(&p, (vt_char_t)'\x09')->n == VT_A_C0_HT);
    assert(vt_parser_put_ch(&p, (vt_char_t)'\x0A')->n == VT_A_C0_LF);
    assert(vt_parser_put_ch(&p, (vt_char_t)'\x0C')->n == VT_A_C0_FF);
    assert(vt_parser_put_ch(&p, (vt_char_t)'\x0D')->n == VT_A_C0_CR);
    
    // C1 codes
    //
    // See: https://en.wikipedia.org/wiki/ANSI_escape_code
    //
    // ESC N	0x8E	SS2	Single Shift Two	Select a single character from one of the alternative character sets. SS2 selects the G2 character set, and SS3 selects the G3 character set.[17] In a 7-bit environment, this is followed by one or more GL bytes (0x20–0x7F) specifying a character from that set.[15]: 9.4  In an 8-bit environment, these may instead be GR bytes (0xA0–0xFF).[15]: 8.4 
    // ESC O	0x8F	SS3	Single Shift Three
    // ESC P	0x90	DCS	Device Control String	Terminated by ST.[16]: 5.6  Xterm's uses of this sequence include defining User-Defined Keys, and requesting or setting Termcap/Terminfo data.[17]
    // ESC X	0x98	SOS	Start of String	Takes an argument of a string of text, terminated by ST.[16]: 5.6  The uses for these string control sequences are defined by the application[16]: 8.3.2, 8.3.128  or privacy discipline.[16]: 8.3.94  These functions are rarely implemented and the arguments are ignored by xterm.[17] Some Kermit clients allow the server to automatically execute Kermit commands on the client by embedding them in APC sequences; this is a potential security risk if the server is untrusted.[18]
    // ESC ^	0x9E	PM	Privacy Message
    // ESC _	0x9F	APC	Application Program Command
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033N")->n == VT_A_SS2);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033O")->n == VT_A_SS3);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033P")->n == VT_A_DCS);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033X")->n == VT_A_SOS);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033^")->n == VT_A_PM);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033_")->n == VT_A_APC);

    // CSI codes
    //
    // See: https://en.wikipedia.org/wiki/ANSI_escape_code
    //
    // CSI n A	CUU	Cursor Up	Moves the cursor n (default 1) cells in the given direction. If the cursor is already at the edge of the screen, this has no effect.
    // CSI n B	CUD	Cursor Down
    // CSI n C	CUF	Cursor Forward
    // CSI n D	CUB	Cursor Back
    // CSI n E	CNL	Cursor Next Line	Moves cursor to beginning of the line n (default 1) lines down. (not ANSI.SYS)
    // CSI n F	CPL	Cursor Previous Line	Moves cursor to beginning of the line n (default 1) lines up. (not ANSI.SYS)
    // CSI n G	CHA	Cursor Horizontal Absolute	Moves the cursor to column n (default 1). (not ANSI.SYS)
    // CSI n ; m H	CUP	Cursor Position	Moves the cursor to row n, column m. The values are 1-based, and default to 1 (top left corner) if omitted. A sequence such as CSI ;5H is a synonym for CSI 1;5H as well as CSI 17;H is the same as CSI 17H and CSI 17;1H
    // CSI n J	ED	Erase in Display	Clears part of the screen. If n is 0 (or missing), clear from cursor to end of screen. If n is 1, clear from cursor to beginning of the screen. If n is 2, clear entire screen (and moves cursor to upper left on DOS ANSI.SYS). If n is 3, clear entire screen and delete all lines saved in the scrollback buffer (this feature was added for xterm and is supported by other terminal applications).
    // CSI n K	EL	Erase in Line	Erases part of the line. If n is 0 (or missing), clear from cursor to the end of the line. If n is 1, clear from cursor to beginning of the line. If n is 2, clear entire line. Cursor position does not change.
    // CSI n S	SU	Scroll Up	Scroll whole page up by n (default 1) lines. New lines are added at the bottom. (not ANSI.SYS)
    // CSI n T	SD	Scroll Down	Scroll whole page down by n (default 1) lines. New lines are added at the top. (not ANSI.SYS)
    // CSI n ; m f	HVP	Horizontal Vertical Position	Same as CUP, but counts as a format effector function (like CR or LF) rather than an editor function (like CUD or CNL). This can lead to different handling in certain terminal modes.[16]: Annex A 
    // CSI n m	SGR	Select Graphic Rendition	Sets colors and style of the characters following this code
    // CSI 5i		AUX Port On	Enable aux serial port usually for local serial printer
    // CSI 4i		AUX Port Off	Disable aux serial port usually for local serial printer
    // CSI 6n	DSR	Device Status Report	Reports the cursor position (CPR) by transmitting ESC[n;mR, where n is the row and m is the column.
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033[A")->n == VT_A_CUU);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033[B")->n == VT_A_CUD);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033[C")->n == VT_A_CUF);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033[D")->n == VT_A_CUB);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033[E")->n == VT_A_CNL);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033[F")->n == VT_A_CPL);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033[G")->n == VT_A_CHA);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033[H")->n == VT_A_CUP);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033[J")->n == VT_A_ED);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033[K")->n == VT_A_EL);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033[S")->n == VT_A_SU);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033[T")->n == VT_A_SD);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033[f")->n == VT_A_HVP);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033[m")->n == VT_A_SGR);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033[r")->n == VT_A_DECSTBM);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033[l")->n == VT_A_RM);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033[h")->n == VT_A_SM);
    // TODO CSI 5i		AUX Port On	Enable aux serial port usually for local serial printer
    // TODO CSI 4i		AUX Port Off	Disable aux serial port usually for local serial printer
    // TODO CSI 6n	DSR	Device Status Report	Reports the cursor position (CPR) by transmitting ESC[n;mR, where n is the row and m is the column.

    // Linux style osc
    // Set and reset pallet
    {
        vt_parser_put_str(&p, (vt_char_t*)"\033]P0123456");
        assert(p.state->n == VT_A_XPALS);
        assert(p.osc_param_len == 7);
        assert(strcmp((char*)(p.osc_param), "0123456") == 0);
    }
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033]R")->n == VT_A_XPALR);
    {
        vt_parser_put_str(&p, (vt_char_t*)"\033]8;;https://example.com\x9C");
        assert(p.state->n == VT_A_OSC);
        assert(p.n_params == 1);
        assert(p.params[0] == 8);
        assert(p.osc_param_len == 20);
        assert(strcmp((char*)(p.osc_param), ";https://example.com") == 0);
    }
    {
        vt_parser_put_str(&p, (vt_char_t*)"\033]8;;https://example.com\x07");
        assert(p.state->n == VT_A_OSC);
        assert(p.n_params == 1);
        assert(p.params[0] == 8);
        assert(p.osc_param_len == 20);
        assert(strcmp((char*)(p.osc_param), ";https://example.com") == 0);
    }
    printf("all ok\n");
    return 0;
}   


