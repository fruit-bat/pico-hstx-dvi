#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "vt_ansi_seq.h"

// See https://vt100.net/docs/vt510-rm/chapter4.html#:~:text=A%20DCS%20control%20string%20is,or%20a%20soft%20character%20set.&text=Ignored.&text=Makes%20the%20terminal%20send%20its,the%20ANSI%20DA%20sequence%20instead.

typedef uint8_t vt_char_t;
typedef int16_t vt_match_t; // Match symbol

#define RC(R,C) ((R<<4)+(C))
typedef enum {
    VT_M_CHAR    = -1,   // A normal character
    VT_M_NP_LOW  = -2,   // A non printing char (below 0x20 ' ')
    VT_M_DIGIT   = -3,   // A digit 0-9
    VT_M_C0_NULL = 0x00, // Null
    VT_M_C0_ENQ  = 0x05, // Enquiry
    VT_M_C0_BEL  = 0x07, // Bell
    VT_M_C0_BS   = 0x08, // Backspace
    VT_M_C0_HT   = 0x09, // Horizontal Tab
    VT_M_C0_LF   = 0x0A, // Line Feed
    VT_M_C0_VT   = 0x0B, // Vertical Tab
    VT_M_C0_FF   = 0x0C, // Form Feed
    VT_M_C0_CR   = 0x0D, // Carriage Return
    VT_M_C0_SO   = 0x0E, // Shift Out
    VT_M_C0_SI   = 0x0F, // Shift In
    VT_M_C0_DC1  = 0x10, // Device Control 1
    VT_M_C0_DC2  = 0x11, // Device Control 2
    VT_M_C0_DC3  = 0x12, // Device Control 3
    VT_M_C0_CAN  = 0x18, // Cancel
    VT_M_C0_SUB  = 0x1A, // Substitute
    VT_M_C0_ESC  = 0x1B, // Escape
    VT_M_C0_DEL  = 0x7F, // Delete

    VT_M_C1_IND = 0x84, // Index
    VT_M_C1_NEL = 0x85, // Next Line
    VT_M_C1_HTS = 0x88, // Horizontal Tab Set
    VT_M_C1_RI  = 0x8D, // Reverse Index
    VT_M_C1_SS2 = 0x8E, // Single Shift 2
    VT_M_C1_SS3 = 0x8F, // Single Shift 3
    VT_M_C1_DCS = 0x90, // Device Control String
    VT_M_C1_SOS = 0x98, // Start of String
    VT_M_C1_PM  = 0x9E, // Privacy Message
    VT_M_C1_APC = 0x9F, // Application Program Command

    VT_M_CSI = 0x9B,    // Control Sequence Introducer
    VT_M_OSC = 0x9D,    // Operating System Command
    VT_M_ST  = 0x9C,    // String Terminator
    VT_M_DCS = 0x90,    // Device Control String
    VT_M_SOS = 0x98,    // Start of String
    VT_M_PM  = 0x9E,    // Privacy Message
    VT_M_APC = 0x9F     // Application Program Command
} vt_m_t;

typedef enum {
    VT_A_CHAR,        // A character is ready

    VT_A_C0_NULL,     // C0 control codes
    VT_A_C0_SOH,      // Start of Heading
    VT_A_C0_STX,      // Start of Text
    VT_A_C0_ETX,      // End of Text
    VT_A_C0_EOT,      // End of Transmission
    VT_A_C0_ENQ,      // Enquiry 
    VT_A_C0_ACK,      // Acknowledge
    VT_A_C0_BEL,      // Bell
    VT_A_C0_BS,       // Backspace
    VT_A_C0_HT,       // Horizontal Tab  
    VT_A_C0_LF,       // Line Feed
    VT_A_C0_VT,       // Vertical Tab
    VT_A_C0_FF,       // Form Feed
    VT_A_C0_CR,       // Carriage Return
    VT_A_C0_SO,       // Shift Out  
    VT_A_C0_SI,       // Shift In
    VT_A_C0_DLE,      // Data Link Escape
    VT_A_C0_DC1,      // Device Control 1 
    VT_A_C0_DC2,      // Device Control 2
    VT_A_C0_DC3,      // Device Control 3
    VT_A_C0_DC4,      // Device Control 4

    VT_A_IND,         // VT100 Index (ESC D)
    VT_A_NEL,         // VT100 Next Line (ESC E)
    VT_A_HTS,         // VT100 Horizontal Tab Set (ESC H)
    VT_A_RI,          // VT100 Reverse Index (ESC M)
    VT_A_SS2,         // VT100 Single Shift 2 (ESC N)
    VT_A_SS3,         // VT100 Single Shift 3 (ESC O)
    VT_A_DCS,         // VT100 Device Control String (ESC P)
    VT_A_SOS,         // VT100 Start of String (ESC X)
    VT_A_PM,          // VT100 Privacy Message (ESC ^)
    VT_A_APC,         // VT100 Application Program Command (ESC _)
    VT_A_SAVE_CUR,    // VT100 Save Cursor Position (ESC 7)
    VT_A_RESTORE_CUR, // VT100 Restore Cursor Position (ESC 8)

    VT_A_CUU,         // CUU - Cursor Up
    VT_A_CUD,         // CUD - Cursor Down 
    VT_A_CUF,         // CUF - Cursor Forward
    VT_A_CUB,         // CUB - Cursor Back
    VT_A_CNL,         // CNL - Cursor Next Line
    VT_A_CPL,         // CPL - Cursor Previous Line
    VT_A_CUP,         // CUP - Cursor Position
    VT_A_CHA,         // CHA - Cursor Horizontal Absolute 
    VT_A_CHT,         // CHT - Cursor Horizontal Tabulation
    VT_A_ED,          // ED  - Erase in Display
    VT_A_EL,          // EL  - Erase in Line
    VT_A_HVP,         // HVP - Horizontal Vertical Position
    VT_A_SGR,         // Select Graphic Rendition
    VT_A_DECSTBM,     // Set Top and Bottom Margins
    VT_A_DA,          // Device Attributes
    VT_A_RM,          // Reset Mode
    VT_A_SM,          // Set Mode
    VT_A_SU,          // Scroll Up
    VT_A_SD,          // Scroll Down

} vt_a_t;

typedef enum {
    VT_F_NONE    = 0x00,
    VT_F_NEXT_CH = 0x01, // Another charater is needed to complete the sequence
    VT_F_FINAL   = 0x02, // Leaf node (n) now contains an action code VT_A_xxx
    VT_F_COL_P   = 0x04, // Collect parameter
    VT_F_NXT_P   = 0x08, // Next parameter
    VT_F_COL_CH  = 0x10, // Collect a character
} vt_f_t;   

typedef enum {
    VT_G_NONE = -1,
    VT_G_GROUND = 0,
    VT_G_C0,
    VT_G_ESC,
    VT_G_CSI,
    VT_G_CSI_P,
    VT_G_CSI_F
} vt_g_t;

typedef struct {
    vt_match_t m; // A symbol to match
    int16_t n; // The next state or the action to take
    uint8_t f; // Flags
} vt_state_t;

vt_state_t vt_states_ground[] = {
    {VT_M_NP_LOW, VT_G_C0,   VT_F_NONE},
    {VT_M_C1_IND, VT_A_IND,  VT_F_FINAL},
    {VT_M_C1_NEL, VT_A_NEL,  VT_F_FINAL},
    {VT_M_C1_HTS, VT_A_HTS,  VT_F_FINAL},
    {VT_M_C1_RI,  VT_A_RI,   VT_F_FINAL},
    {VT_M_C1_SS2, VT_A_SS2,  VT_F_FINAL},
    {VT_M_C1_SS3, VT_A_SS3,  VT_F_FINAL},
    {VT_M_C1_DCS, VT_A_DCS,  VT_F_FINAL},
    {VT_M_C1_SOS, VT_A_SOS,  VT_F_FINAL},
    {VT_M_C1_PM,  VT_A_PM,   VT_F_FINAL},
    {VT_M_C1_APC, VT_A_APC,  VT_F_FINAL},
    {VT_M_CSI,    VT_G_CSI,  VT_F_NEXT_CH}, // CSI
    {VT_M_CHAR,   VT_A_CHAR, VT_F_COL_CH|VT_F_FINAL}, // A normal character
};
vt_state_t vt_states_c0[] = {
    {VT_M_C0_ESC,  VT_G_ESC,     VT_F_NEXT_CH}, // ESC
    {VT_M_C0_NULL, VT_A_C0_NULL, VT_F_FINAL},
    {VT_M_C0_ENQ,  VT_A_C0_ENQ,  VT_F_FINAL},
    {VT_M_C0_BEL,  VT_A_C0_BEL,  VT_F_FINAL},
    {VT_M_C0_BS,   VT_A_C0_BS,   VT_F_FINAL},
    {VT_M_C0_HT,   VT_A_C0_HT,   VT_F_FINAL},
    {VT_M_C0_LF,   VT_A_C0_LF,   VT_F_FINAL},
    {VT_M_C0_VT,   VT_A_C0_VT,   VT_F_FINAL},
    {VT_M_C0_FF,   VT_A_C0_FF,   VT_F_FINAL},
    {VT_M_C0_CR,   VT_A_C0_CR,   VT_F_FINAL},
    {VT_M_C0_SO,   VT_A_C0_SO,   VT_F_FINAL},
    {VT_M_C0_SI,   VT_A_C0_SI,   VT_F_FINAL},
    {VT_M_C0_DC1,  VT_A_C0_DC1,  VT_F_FINAL},
    {VT_M_C0_DC2,  VT_A_C0_DC2,  VT_F_FINAL},
    {VT_M_C0_DC3,  VT_A_C0_DC3,  VT_F_FINAL},
};
vt_state_t vt_states_esc[] = {
    {'[',          VT_G_CSI,          VT_F_NEXT_CH}, // ESC [
    {'D',          VT_A_IND,          VT_F_FINAL},   // ESC D
    {'E',          VT_A_NEL,          VT_F_FINAL},   // ESC E
    {'H',          VT_A_HTS,          VT_F_FINAL},   // ESC H
    {'M',          VT_A_RI,           VT_F_FINAL},   // ESC M
    {'N',          VT_A_SS2,          VT_F_FINAL},   // ESC N
    {'O',          VT_A_SS3,          VT_F_FINAL},   // ESC O
    {'P',          VT_A_DCS,          VT_F_FINAL},   // ESC P
    {'X',          VT_A_SOS,          VT_F_FINAL},   // ESC X
    {'^',          VT_A_PM,           VT_F_FINAL},   // ESC ^
    {'_',          VT_A_APC,          VT_F_FINAL},   // ESC _
    {'7',          VT_A_SAVE_CUR,     VT_F_FINAL},   // ESC 7
    {'8',          VT_A_RESTORE_CUR,  VT_F_FINAL},   // ESC 8
};
vt_state_t vt_states_csi[] = {
    {'s',          VT_A_SAVE_CUR,     VT_F_FINAL},   // ESC [s
    {'u',          VT_A_RESTORE_CUR,  VT_F_FINAL},   // ESC [u
    {VT_M_DIGIT,   VT_G_CSI_P,        VT_F_NXT_P|VT_F_COL_P|VT_F_NEXT_CH}, // ESC [0-9
    {';',          VT_G_CSI_P,        VT_F_NXT_P},
    {VT_M_CHAR,    VT_G_CSI_F,        VT_F_NONE},    // ESC [0-9;s
};
vt_state_t vt_states_csi_p[] = {
    {VT_M_DIGIT,   VT_G_CSI_P,        VT_F_COL_P|VT_F_NEXT_CH}, // Collect param digits
    {';',          VT_G_CSI_P,        VT_F_NXT_P|VT_F_NEXT_CH}, // Next param
    {VT_M_CHAR,    VT_G_CSI_F,        VT_F_NONE}, // ESC [0-9;s
};
vt_state_t vt_states_csi_f[] = {
    {'A',          VT_A_CUU,          VT_F_FINAL}, // ESC [nA
    {'B',          VT_A_CUD,          VT_F_FINAL}, // ESC [nB
    {'C',          VT_A_CUF,          VT_F_FINAL}, // ESC [nC
    {'D',          VT_A_CUB,          VT_F_FINAL}, // ESC [nD
    {'E',          VT_A_CNL,          VT_F_FINAL}, // ESC [nE          
    {'F',          VT_A_CPL,          VT_F_FINAL}, // ESC [nF
    {'f',          VT_A_HVP,          VT_F_FINAL}, // ESC [nf
    {'G',          VT_A_CHA,          VT_F_FINAL}, // ESC [nG          
    {'H',          VT_A_CUP,          VT_F_FINAL}, // ESC [n;nH
    {'J',          VT_A_ED,           VT_F_FINAL}, // ESC [nJ
    {'K',          VT_A_EL,           VT_F_FINAL}, // ESC [nK
    {'S',          VT_A_SU,           VT_F_FINAL}, // ESC [nS
    {'T',          VT_A_SD,           VT_F_FINAL}, // ESC [nT
    {'m',          VT_A_SGR,          VT_F_FINAL}, // ESC [n;n;...m
    {'r',          VT_A_DECSTBM,      VT_F_FINAL}, // ESC [t;r
    {'l',          VT_A_RM,           VT_F_FINAL}, // ESC [?n;l
    {'h',          VT_A_SM,           VT_F_FINAL}, // ESC [?n;h
};

#define STATE_PTR(X) ((vt_state_t*)X) 

vt_state_t* vt_state_grp[] = {
    STATE_PTR(vt_states_ground),
    STATE_PTR(vt_states_c0),
    STATE_PTR(vt_states_esc),
    STATE_PTR(vt_states_csi),
    STATE_PTR(vt_states_csi_p),
    STATE_PTR(vt_states_csi_f)
};

#define COUNT_ARR(X) ((sizeof (X))/(sizeof (X)[0]))

uint8_t vt_state_grp_len[] = {
    COUNT_ARR(vt_states_ground),
    COUNT_ARR(vt_states_c0),
    COUNT_ARR(vt_states_esc),
    COUNT_ARR(vt_states_csi),
    COUNT_ARR(vt_states_csi_p),
    COUNT_ARR(vt_states_csi_f)
};

typedef struct {
    vt_state_t* state; // Current state
    uint32_t params[16]; // Parameters collected
    uint8_t n_params; // Number of parameters collected
    vt_char_t ch;
} vt_parser_t;

void vt_parser_init(vt_parser_t *p) {
    p->state = NULL;
    p->n_params = 0;
}

bool vt_parser_match_ch(vt_match_t m, vt_char_t ch) {
    if (m < 0) {
        switch(m) {
            case VT_M_CHAR:     return ch >= 0x20 && ch != 0x7F;
            case VT_M_NP_LOW:   return ch < 0x20;
            case VT_M_DIGIT:    return ch >= '0' && ch <= '9';
            default:            return 0;
        }
    }
    else {
        return m == ch;
    }
}

vt_state_t* vt_parser_put_ch(vt_parser_t *p, vt_char_t ch) {
        
    vt_state_t* ps = p->state;
    vt_g_t g = ps && !(ps->f & VT_F_FINAL) ? (vt_g_t)ps->n : VT_G_GROUND;
    if (g == VT_G_GROUND) {
        p->n_params = 0;
        for (uint8_t i = 0; i < COUNT_ARR(p->params); ++i) {
            p->params[i] = 0;
        }
    }
    vt_state_t* gps = vt_state_grp[g];
    uint8_t gpl = vt_state_grp_len[g];

    for (uint8_t i = 0; i < gpl;) {
        vt_state_t* s = &gps[i];
        if (vt_parser_match_ch(s->m, ch)) {
            p->state = s;
            if (s->f & VT_F_COL_CH) {
                p->ch = ch;
            }
            if (s->f & VT_F_NXT_P) {
                // Next parameter
                if (p->n_params < COUNT_ARR(p->params)) {
                    p->params[p->n_params] = 0;
                    p->n_params++;
                }
                else {
                    // Too many parameters, ignore
                    // TODO consider failure mode
                }
            }
            if (s->f & VT_F_COL_P)  {
                // Collect parameter digit
                uint8_t pi = p->n_params - 1;
                if (pi < COUNT_ARR(p->params)) {
                    p->params[pi] = p->params[pi] * 10 + (ch - '0');
                }
            }
            if (s->f & (VT_F_NEXT_CH | VT_F_FINAL)) {
                // Need next char to determine action
                return s;
            }
            else {
                // Keep processing with the current char
                gps = vt_state_grp[s->n];
                gpl = vt_state_grp_len[s->n];
                i = 0;
                continue;
            }
        }
        ++i;
    }
    vt_parser_init(p);
    return NULL;
}

vt_state_t* vt_parser_put_str(vt_parser_t *p, vt_char_t* s) {
    vt_state_t* last = NULL;
    for (vt_char_t* c = s; *c; ++c) {
        last = vt_parser_put_ch(p, *c);
    }
    return last;
}

#include <assert.h>

int main() {
    vt_parser_t p;
    vt_parser_init(&p);

    {
        vt_state_t* s = vt_parser_put_ch(&p, 'D'); // D
        assert(s != NULL);
        assert(s->m == VT_M_CHAR);
        assert(s->n == VT_A_CHAR);
        assert(s->f & VT_F_FINAL);
        assert(p.ch == 'D');
    }
    {
        vt_state_t* s = vt_parser_put_ch(&p, 0x1B); // ESC
        assert(s != NULL);
        assert(s->m == VT_M_C0_ESC);
        assert((s->f & VT_F_FINAL) == 0);
    }
    {
        vt_state_t* s = vt_parser_put_ch(&p, 'D'); // D
        assert(s != NULL);
        assert(s->m == 'D');
        assert(s->f & VT_F_FINAL);
        assert(s->n == VT_A_IND);
    }
    {
        vt_parser_put_ch(&p, 0x1B); // ESC
        vt_parser_put_ch(&p, '[');  // [
        vt_state_t* s = vt_parser_put_ch(&p, 's');  // s
        assert(s != NULL);
        assert(s->m == 's');
        assert(s->f & VT_F_FINAL);
        assert(s->n == VT_A_SAVE_CUR);
    }
    {
        vt_state_t* s = vt_parser_put_str(&p, (vt_char_t*)"\033[s"); // ESC
        assert(s != NULL);
        assert(s->m == 's');
        assert(s->f & VT_F_FINAL);
        assert(s->n == VT_A_SAVE_CUR);
    }
    {
        vt_state_t* s = vt_parser_put_str(&p, (vt_char_t*)"\033[123m"); // ESC
        assert(s != NULL);
        assert(s->m == 'm');
        assert(s->f & VT_F_FINAL);
        assert(s->n == VT_A_SGR);
        assert(p.n_params == 1);
        assert(p.params[0] == 123);
    }
    {
        vt_state_t* s = vt_parser_put_str(&p, (vt_char_t*)"\033[456m"); // ESC
        assert(s != NULL);
        assert(s->m == 'm');
        assert(s->f & VT_F_FINAL);
        assert(s->n == VT_A_SGR);
        assert(p.n_params == 1);
        assert(p.params[0] == 456);
    }
    {
        vt_state_t* s = vt_parser_put_str(&p, (vt_char_t*)"\033[1;2;3;4;5;0m"); // ESC
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
        vt_state_t* s = vt_parser_put_str(&p, (vt_char_t*)"\033[m"); // ESC
        assert(s != NULL);
        assert(s->m == 'm');
        assert(s->f & VT_F_FINAL);
        assert(s->n == VT_A_SGR);
        assert(p.n_params == 0);
    }
    {
        vt_state_t* s = vt_parser_put_str(&p, (vt_char_t*)"\033[5;m"); // ESC
        assert(s != NULL);
        assert(s->m == 'm');
        assert(s->f & VT_F_FINAL);
        assert(s->n == VT_A_SGR);
        assert(p.n_params == 2);
        assert(p.params[0] == 5);
        assert(p.params[1] == 0);
    }
    {
        vt_state_t* s = vt_parser_put_str(&p, (vt_char_t*)"\033[;m"); // ESC
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
    // ESC \	0x9C	ST	String Terminator	Terminates strings in other controls.[16]: 8.3.143 
    // ESC ]	0x9D	OSC	Operating System Command	Starts a control string for the operating system to use, terminated by ST.[16]: 8.3.89 
    // ESC X	0x98	SOS	Start of String	Takes an argument of a string of text, terminated by ST.[16]: 5.6  The uses for these string control sequences are defined by the application[16]: 8.3.2, 8.3.128  or privacy discipline.[16]: 8.3.94  These functions are rarely implemented and the arguments are ignored by xterm.[17] Some Kermit clients allow the server to automatically execute Kermit commands on the client by embedding them in APC sequences; this is a potential security risk if the server is untrusted.[18]
    // ESC ^	0x9E	PM	Privacy Message
    // ESC _	0x9F	APC	Application Program Command
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033N")->n == VT_A_SS2);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033O")->n == VT_A_SS3);
    assert(vt_parser_put_str(&p, (vt_char_t*)"\033P")->n == VT_A_DCS);
    // TODO ST
    // TODO OSC
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

    return 0;
}   


