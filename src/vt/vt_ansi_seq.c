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
    VT_M_CHAR = -1,      // A normal character
    VT_M_NP_LOW = -2,    // A non printing char (below 0x20 ' ')
    VT_M_DIGIT = -3,     // A digit 0-9
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
    VT_A_CHAR,

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
    VT_A_CUP,         // CUP - Cursor Position
    VT_A_CHA,         // CHA - Cursor Horizontal Absolute 
    VT_A_CHT,         // CHT - Cursor Horizontal Tabulation
    VT_A_ED,          // ED  - Erase in Display
    VT_A_EL,          // EL  - Erase in Line
    VT_A_SGR,         // Select Graphic Rendition
    VT_A_DECSTBM,     // Set Top and Bottom Margins
    VT_A_DA,          // Device Attributes
    VT_A_RM,          // Reset Mode
    VT_A_SM,          // Set Mode


    VT_A_CTRL_C
} vt_a_t;

typedef enum {
    VT_F_NONE    = 0x00,
    VT_F_NEXT_CH = 0x01,
    VT_F_FINAL   = 0x02,
    VT_F_COL_P   = 0x04, // Collect parameter
    VT_F_NXT_P   = 0x08  // Next parameter
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
    {VT_M_CHAR,   VT_A_CHAR, VT_F_FINAL}, // A normal character
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
    {'D',          VT_A_IND,          VT_F_FINAL}, // ESC D
    {'E',          VT_A_NEL,          VT_F_FINAL}, // ESC E
    {'H',          VT_A_HTS,          VT_F_FINAL}, // ESC H
    {'M',          VT_A_RI,           VT_F_FINAL}, // ESC M
    {'N',          VT_A_SS2,          VT_F_FINAL}, // ESC N
    {'O',          VT_A_SS3,          VT_F_FINAL}, // ESC O
    {'P',          VT_A_DCS,          VT_F_FINAL}, // ESC P
    {'X',          VT_A_SOS,          VT_F_FINAL}, // ESC X
    {'^',          VT_A_PM,           VT_F_FINAL}, // ESC ^
    {'_',          VT_A_APC,          VT_F_FINAL}, // ESC _
    {'7',          VT_A_SAVE_CUR,     VT_F_FINAL}, // ESC 7
    {'8',          VT_A_RESTORE_CUR,  VT_F_FINAL}, // ESC 8
};
vt_state_t vt_states_csi[] = {
    {'s',          VT_A_SAVE_CUR,     VT_F_FINAL}, // ESC [s
    {'u',          VT_A_RESTORE_CUR,  VT_F_FINAL}, // ESC [u
    {VT_M_DIGIT,   VT_G_CSI_P,        VT_F_NONE}, // ESC [0-9
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
    {'H',          VT_A_CUP,          VT_F_FINAL}, // ESC [n;nH
    {'f',          VT_A_CUP,          VT_F_FINAL}, // ESC [n;nF
    {'J',          VT_A_ED,           VT_F_FINAL}, // ESC [nJ
    {'K',          VT_A_EL,           VT_F_FINAL}, // ESC [nK
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
    if (g == VT_G_GROUND) p->n_params = 0;
    vt_state_t* gps = vt_state_grp[g];
    uint8_t gpl = vt_state_grp_len[g];

    for (uint8_t i = 0; i < gpl;) {
        vt_state_t* s = &gps[i];
        if (vt_parser_match_ch(s->m, ch)) {
            p->state = s;
            if (s->f & VT_F_COL_P) {
                // Collect parameter digit
                uint8_t np = p->n_params;
                if (np < COUNT_ARR(p->params)) {
                    if (np) {
                        p->params[np-1] = p->params[np-1] * 10 + (ch - '0');
                    }
                    else {
                        p->params[0] = ch - '0';
                        p->n_params = 1;
                    }
                }
            }
            if (s->f & VT_F_NXT_P) {
                // Next parameter
                if (p->n_params < COUNT_ARR(p->params)) {
                    p->n_params++;
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
        vt_state_t* st = vt_parser_put_ch(p, *c);
        if (st) {
            last = st;
        }
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
    return 0;
}   


