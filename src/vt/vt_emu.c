

#include "vt_emu.h"

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

void vt_emu_put_ch(
    vt_emu_t *e, 
    vt_char_t ch
) {
    const vt_state_t* s = vt_parser_put_ch(&e->parser, ch);
    const vt_a_t a = vt_parser_action_from_state(s);
    vt_term_t* const t = &e->term;
    vt_parser_t* const p = &e->parser;
    switch(a) {
    case VT_A_CHAR:        // A character is ready
        vt_term_putch(t, p->ch);
        break;
    case VT_A_C0_NULL:     // C0 control codes
    case VT_A_C0_SOH:      // Start of Heading
    case VT_A_C0_STX:      // Start of Text
    case VT_A_C0_ETX:      // End of Text
    case VT_A_C0_EOT:      // End of Transmission
    case VT_A_C0_ENQ:      // Enquiry 
    case VT_A_C0_ACK:      // Acknowledge
    case VT_A_C0_BEL:      // Bell
    case VT_A_C0_BS:       // Backspace
    case VT_A_C0_HT:       // Horizontal Tab  
    case VT_A_C0_LF:       // Line Feed
        vt_term_nl(t);
        break;
    case VT_A_C0_VT:       // Vertical Tab
    case VT_A_C0_FF:       // Form Feed
    case VT_A_C0_CR:       // Carriage Return
        vt_term_cr(t);
        break;
    case VT_A_C0_SO:       // Shift Out  
    case VT_A_C0_SI:       // Shift In
    case VT_A_C0_DLE:      // Data Link Escape
    case VT_A_C0_DC1:      // Device Control 1 
    case VT_A_C0_DC2:      // Device Control 2
    case VT_A_C0_DC3:      // Device Control 3
    case VT_A_C0_DC4:      // Device Control 4
    case VT_A_IND:         // VT100 Index (ESC D)
    case VT_A_NEL:         // VT100 Next Line (ESC E)
    case VT_A_HTS:         // VT100 Horizontal Tab Set (ESC H)
    case VT_A_RI:          // VT100 Reverse Index (ESC M)
    case VT_A_SS2:         // VT100 Single Shift 2 (ESC N)
    case VT_A_SS3:         // VT100 Single Shift 3 (ESC O)
    case VT_A_DCS:         // VT100 Device Control String (ESC P)
    case VT_A_SOS:         // VT100 Start of String (ESC X)
    case VT_A_PM:          // VT100 Privacy Message (ESC ^)
    case VT_A_APC:         // VT100 Application Program Command (ESC _)
    case VT_A_SAVE_CUR:    // VT100 Save Cursor Position (ESC 7)
    case VT_A_RESTORE_CUR: // VT100 Restore Cursor Position (ESC 8)
    case VT_A_CUU:         // CUU - Cursor Up
    case VT_A_CUD:         // CUD - Cursor Down 
    case VT_A_CUF:         // CUF - Cursor Forward
    case VT_A_CUB:         // CUB - Cursor Back
    case VT_A_CNL:         // CNL - Cursor Next Line
    case VT_A_CPL:         // CPL - Cursor Previous Line
    case VT_A_CUP:         // CUP - Cursor Position
    case VT_A_CHA:         // CHA - Cursor Horizontal Absolute 
    case VT_A_CHT:         // CHT - Cursor Horizontal Tabulation
    case VT_A_ED:          // ED  - Erase in Display
    case VT_A_EL:          // EL  - Erase in Line
    case VT_A_HVP:         // HVP - Horizontal Vertical Position
    case VT_A_SGR:         // Select Graphic Rendition
    case VT_A_DECSTBM:     // Set Top and Bottom Margins
    case VT_A_DA:          // Device Attributes
    case VT_A_RM:          // Reset Mode
    case VT_A_SM:          // Set Mode
    case VT_A_SU:          // Scroll Up
    case VT_A_SD:          // Scroll Down
    case VT_A_XPALS:       // Linux set pallet 
    case VT_A_XPALR:       // Linux reset pallet
    case VT_A_OSC:         // General OSC string handling
    case VT_A_NONE:         // Do nothing!
    default:
        break;
    }
}

void vt_emu_put_str(
    vt_emu_t *e, 
    vt_char_t* s
) {
    while(*s) vt_emu_put_ch(e, *s++);
}
