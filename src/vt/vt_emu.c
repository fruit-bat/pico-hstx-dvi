

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
        break;
    case VT_A_RESTORE_CUR: // VT100 Restore Cursor Position (ESC 8)
        DEBUG("VT_A_RESTORE_CUR\n");
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
    case VT_A_CHA:         // CHA - Cursor Horizontal Absolute 
        DEBUG("VT_A_CHA\n");
        break;
    case VT_A_CHT:         // CHT - Cursor Horizontal Tabulation
        DEBUG("VT_A_CHT\n");
        break;
    case VT_A_ED:          // ED  - Erase in Display
        DEBUG("VT_A_ED\n");
        vt_term_erase_in_display(t, vt_emu_get_p0(p, 0));
        break;
    case VT_A_EL:          // EL  - Erase in Line
        DEBUG("VT_A_EL\n");
        vt_term_erase_in_line(t, vt_emu_get_p0(p, 0));
        break;    
    case VT_A_SGR:         // Select Graphic Rendition
        DEBUG("VT_A_SGR\n");
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
