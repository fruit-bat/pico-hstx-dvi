

#include "vt_emu.h"
#include <stdio.h>

#define DEBUG(...) printf(__VA_ARGS__)

void vt_emu_init(
    vt_emu_t* e,     // Emulator
    vt_cell_t* grid, // Cell grid for the display
    vt_coord_t w,    // Terminal width in characters
    vt_coord_t h     // Terminal height in characters
) {
    vt_parser_init(
        &e->parser
    );
    vt_term_init(
        &e->term,
        grid,
        w,
        h
    );
}

void vt_emu_reset(
    vt_emu_t *e
) {
    vt_parser_reset(
        &e->parser
    );
    vt_term_reset(
        &e->term
    );
}

uint32_t vt_emu_get_p0(vt_parser_t* p, uint32_t i) {
    return p->n_params <= i ? 0 : p->params[i];
}

uint32_t vt_emu_get_p1(vt_parser_t* p, uint32_t i) {
    return p->n_params <= i || p->params[i] == 0 ? 1 : p->params[i];
}
//
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
//
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
//
// Color Name	    Foreground Color Code	Background Color Code
// Bright Black	    90	                    100
// Bright Red	    91	                    101
// Bright Green	    92	                    102
// Bright Yellow	93	                    103
// Bright Blue	    94	                    104
// Bright Magenta	95	                    105
// Bright Cyan	    96	                    106
// Bright White	    97	                    107
//
static void vt_emu_sgr(vt_emu_t* const e) {
    vt_term_t* const t = &e->term;
    vt_parser_t* const p = &e->parser;

    for (uint32_t i = 0; i < p->n_params; ++i) {
        const uint32_t q = p->params[i];
        if (q == 0) {
            vt_term_reset_attr(t);
        }
        else if (q <= 9) {
            vt_term_flags_add(t, 1 << (q - 1));
        }
        else if (q == 10) {
            // Turn on ASC (??)
        }
        else if (q == 11) {
            // Turn on tacs (??)
        }
        else if (q <= 21) {
            // ??
        }
        else if (q <= 29) {
            // TODO 21, 22
            vt_term_flags_clear(t, 1 << (q - 22));
        }
        else if (q <= 37) {
            vt_term_set_fgci(t, q - 30);
        }
        else if (q == 38) {
            if ((p->n_params > i + 2) && (p->params[i + 1] == 5)) {
                const uint32_t x = p->params[i + 2];
                vt_term_set_fgci(t, x & 0xff);
            }
            i += 2;
        }
        else if (q == 39) {
            vt_term_set_fgci(t, VT_TERM_DEFAULT_FG);
        }
        else if (q <= 47) {
            vt_term_set_bgci(t, q - 40);
        }
        else if (q == 48) {
            if ((p->n_params > i + 2) && (p->params[i + 1] == 5)) {
                const uint32_t x = p->params[i + 2];
                vt_term_set_bgci(t, x & 0xff);
            }
            i += 2;
        }
        else if (q == 49) {
            vt_term_set_gci(t, VT_TERM_DEFAULT_BG);
        }
        else if (q <= 89) {
            // ??
        }
        else if (q <= 97) {
            vt_term_set_fgci(t, q - 90 + 8);
        }
        else if (q <= 99) {
            // ??
        }
        else if (q <= 107) {
            vt_term_set_bgci(t, q - 100 + 8);
        }
    }
}

void vt_emu_put_ch(
    vt_emu_t *e, 
    vt_char_t ch
) {
    const vt_state_t* s = vt_parser_put_ch(&e->parser, ch);
    const vt_a_t a = vt_parser_action_from_state(s);
    vt_term_t* const t = &e->term;
    vt_parser_t* const p = &e->parser;
    DEBUG("Action %d\n", a);
    switch(a) {
    case VT_A_CHAR:        // A character is ready
        DEBUG("VT_A_CHAR (%d)\n", p->ch);
        vt_term_putch(t, p->ch);
        break;
    case VT_A_C0_NULL:     // C0 control codes
        DEBUG("VT_A_C0_NULL\n");
        break;
    case VT_A_C0_SOH:      // Start of Heading
        DEBUG("VT_A_C0_SOH\n");
        break;
    case VT_A_C0_STX:      // Start of Text
        DEBUG("VT_A_C0_STX\n");
        break;
    case VT_A_C0_ETX:      // End of Text
        DEBUG("VT_A_C0_ETX\n");
        break;
    case VT_A_C0_EOT:      // End of Transmission
        DEBUG("VT_A_C0_EOT\n");
        break;
    case VT_A_C0_ENQ:      // Enquiry 
        DEBUG("VT_A_C0_ENQ\n");
        break;
    case VT_A_C0_ACK:      // Acknowledge
        DEBUG("VT_A_C0_ACK\n");
        break;
    case VT_A_C0_BEL:      // Bell
        DEBUG("VT_A_C0_BEL\n");
        break;
    case VT_A_C0_BS:       // Backspace
        DEBUG("VT_A_C0_BS\n");
        vt_term_cursor_left(t,1);
        break;
    case VT_A_C0_HT:       // Horizontal Tab  
        DEBUG("VT_A_C0_HT\n");
        break;
    case VT_A_C0_LF:       // Line Feed
        DEBUG("VT_A_C0_LF\n");
        vt_term_nl(t);
        break;
    case VT_A_C0_VT:       // Vertical Tab
        DEBUG("\n");
        break;
    case VT_A_C0_FF:       // Form Feed
        DEBUG("VT_A_C0_VT\n");
        break;
    case VT_A_C0_CR:       // Carriage Return
        DEBUG("VT_A_C0_CR\n");
        vt_term_cr(t);
        break;
    case VT_A_C0_SO:       // Shift Out  
        DEBUG("VT_A_C0_SO\n");
        break;
    case VT_A_C0_SI:       // Shift In
        DEBUG("VT_A_C0_SI\n");
        break;
    case VT_A_C0_DLE:      // Data Link Escape
        DEBUG("VT_A_C0_DLE\n");
        break;
    case VT_A_C0_DC1:      // Device Control 1 
        DEBUG("VT_A_C0_DC1\n");
        break;
    case VT_A_C0_DC2:      // Device Control 2
        DEBUG("VT_A_C0_DC2\n");
        break;
    case VT_A_C0_DC3:      // Device Control 3
        DEBUG("VT_A_C0_DC3\n");
        break;
    case VT_A_C0_DC4:      // Device Control 4
        DEBUG("VT_A_C0_DC4\n");
        break;
    case VT_A_IND:         // VT100 Index (ESC D)
        DEBUG("VT_A_IND\n");
        break;
    case VT_A_NEL: {        // VT100 Next Line (ESC E)
        DEBUG("VT_A_NEL");
        // TODO investigate behaviour
        break;
    }
    case VT_A_HTS:         // VT100 Horizontal Tab Set (ESC H)
        DEBUG("VT_A_HTS\n");
        break;
    case VT_A_RI:          // VT100 Reverse Index (ESC M)
        DEBUG("VT_A_RI\n");
        vt_term_reverse_nl(t);
        break;
    case VT_A_SS2:         // VT100 Single Shift 2 (ESC N)
        DEBUG("VT_A_SS2\n");
        break;
    case VT_A_SS3:         // VT100 Single Shift 3 (ESC O)
        DEBUG("VT_A_SS3\n");
        break;
    case VT_A_DCS:         // VT100 Device Control String (ESC P)
        DEBUG("VT_A_DCS\n");
        break;
    case VT_A_SOS:         // VT100 Start of String (ESC X)
        DEBUG("VT_A_SOS\n");
        break;
    case VT_A_PM:          // VT100 Privacy Message (ESC ^)
        DEBUG("VT_A_PM\n");
        break;
    case VT_A_APC:         // VT100 Application Program Command (ESC _)
        DEBUG("VT_A_APC\n");
        break;
    case VT_A_SAVE_CUR:    // VT100 Save Cursor Position (ESC 7)
        DEBUG("VT_A_SAVE_CUR\n");
        vt_term_save_cursor(t);
        break;
    case VT_A_RESTORE_CUR: // VT100 Restore Cursor Position (ESC 8)
        DEBUG("VT_A_RESTORE_CUR\n");
        vt_term_restore_cursor(t);
        break;
    case VT_A_CUU: {        // CUU - Cursor Up
        const uint32_t n = vt_emu_get_p1(p, 0);
        DEBUG("VT_A_CUU %lu\n", n);
        vt_term_cursor_up(t, n);
        break;
    }
    case VT_A_CUD: {       // CUD - Cursor Down
        const uint32_t n = vt_emu_get_p1(p, 0);
        DEBUG("VT_A_CUD %lu\n", n);
        vt_term_cursor_down(t, n);
        break;
    }
    case VT_A_CUF: {        // CUF - Cursor Forward
        const uint32_t n = vt_emu_get_p1(p, 0);
        DEBUG("VT_A_CUF %lu\n", n);
        vt_term_cursor_right(t, n);
        break;
    }
    case VT_A_CUB: {         // CUB - Cursor Back
        const uint32_t n = vt_emu_get_p1(p, 0);
        DEBUG("VT_A_CUB %lu\n", n);
        vt_term_cursor_left(t, n);
        break;
    }
    case VT_A_CNL: {       // CNL - Cursor Next Line
        const uint32_t n = vt_emu_get_p1(p, 0);        
        DEBUG("VT_A_CNL %lu\n", n);
        vt_term_next_line_down(t, n);    
        break;
    }
    case VT_A_CPL: {        // CPL - Cursor Previous Line
        const uint32_t n = vt_emu_get_p1(p, 0);        
        DEBUG("VT_A_CPL %lu\n", n);
        vt_term_next_line_up(t, n);
        break;
    }
    case VT_A_HVP:         // HVP - Horizontal Vertical Position
    case VT_A_CUP: {       // CUP - Cursor Position
        const uint32_t r = vt_emu_get_p0(p, 0);
        const uint32_t c = vt_emu_get_p0(p, 1);
        DEBUG("VT_A_CUP/VT_A_HVP %lu, %lu\n", r, c);
        vt_term_cursor_set(t, r, c);
        break;
    }
    case VT_A_CHA: {        // CHA - Cursor Horizontal Absolute 
        const uint32_t c = vt_emu_get_p0(p, 0);
        DEBUG("VT_A_CHA %lu\n", c);
        vt_term_cursor_set_col(t, c);
        break;
    }
    case VT_A_CHT:         // CHT - Cursor Horizontal Tabulation
        DEBUG("VT_A_CHT\n");
        break;
    case VT_A_ED: {         // ED  - Erase in Display
        const uint32_t n = vt_emu_get_p0(p, 0);
        DEBUG("VT_A_ED %lu\n", n);
        vt_term_erase_in_display(t, n);
        break;
    }
    case VT_A_EL: {         // EL  - Erase in Line
        const uint32_t n = vt_emu_get_p0(p, 0);
        DEBUG("VT_A_EL %lu\n", n);
        vt_term_erase_in_line(t, n);
        break;
    }
    case VT_A_SGR:         // Select Graphic Rendition
        DEBUG("VT_A_SGR\n");
        vt_emu_sgr(e);
        break;
    case VT_A_DECSTBM:     // Set Top and Bottom Margins
        DEBUG("VT_A_DECSTBM\n");
        break;
    case VT_A_DA:          // Device Attributes
        DEBUG("VT_A_DA\n");
        break;
    case VT_A_RM:          // Reset Mode
        DEBUG("VT_A_RM\n");
        break;
    case VT_A_SM:          // Set Mode
        DEBUG("VT_A_SM\n");
        break;
    case VT_A_SU:          // Scroll Up
        DEBUG("VT_A_SU\n");
        break;
    case VT_A_SD:          // Scroll Down
        DEBUG("VT_A_SD\n");
        break;
    case VT_A_XPALS:       // Linux set pallet 
        DEBUG("VT_A_XPALS\n");
        break;
    case VT_A_XPALR:       // Linux reset pallet
        DEBUG("VT_A_XPALR\n");
        break;
    case VT_A_OSC:         // General OSC string handling
        DEBUG("VT_A_OSC\n");
        break;
    case VT_A_NONE:         // Do nothing!
        DEBUG("VT_A_NONE\n");
        break;
    default:
        DEBUG("VT_A unrecognised\n");
        break;
    }
}

void vt_emu_put_str(
    vt_emu_t *e, 
    vt_char_t* s
) {
    while(*s) vt_emu_put_ch(e, *s++);
}
