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

    VT_M_CSI = 0x9B, // Control Sequence Introducer
    VT_M_OSC = 0x9D, // Operating System Command
    VT_M_ST  = 0x9C, // String Terminator
    VT_M_DCS = 0x90, // Device Control String
    VT_M_SOS = 0x98, // Start of String
    VT_M_PM  = 0x9E, // Privacy Message
    VT_M_APC = 0x9F  // Application Program Command
} vt_m_t;

typedef enum {
    VT_A_CHAR,

    VT_A_C0_NULL,
    VT_A_C0_SOH,
    VT_A_C0_STX,
    VT_A_C0_ETX,
    VT_A_C0_EOT,
    VT_A_C0_ENQ,
    VT_A_C0_ACK,
    VT_A_C0_BEL,
    VT_A_C0_BS,
    VT_A_C0_HT,
    VT_A_C0_LF,
    VT_A_C0_VT,
    VT_A_C0_FF,
    VT_A_C0_CR,
    VT_A_C0_SO,
    VT_A_C0_SI,
    VT_A_C0_DLE,
    VT_A_C0_DC1,
    VT_A_C0_DC2,
    VT_A_C0_DC3,
    VT_A_C0_DC4,

    VT_A_IND, // ESC D, RC(8,4)
    VT_A_NEL,
    VT_A_HTS,
    VT_A_RI,
    VT_A_SS2,
    VT_A_SS3,
    VT_A_DCS,
    VT_A_SOS,
    VT_A_PM,
    VT_A_APC,


    VT_A_CTRL_C
} vt_a_t;

typedef enum {
    VT_F_NONE    = 0x00,
    VF_F_NEXT_CH = 0x01,
    VT_F_FINAL   = 0x02
} vt_f_t;   

typedef enum {
    VT_G_NONE = -1,
    VT_G_GROUND = 0,
    VT_G_C0,
    VT_G_ESC
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
    {VT_M_CHAR,   VT_A_CHAR, VT_F_FINAL}, // A normal character
};
vt_state_t vt_states_c0[] = {
    {VT_M_C0_ESC,  VT_G_ESC,     VF_F_NEXT_CH}, // ESC
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
    {'D',          VT_A_IND,     VT_F_FINAL}, // ESC D
    {'E',          VT_A_NEL,     VT_F_FINAL}, // ESC E
    {'H',          VT_A_HTS,     VT_F_FINAL}, // ESC H
    {'M',          VT_A_RI,      VT_F_FINAL}, // ESC M
    {'N',          VT_A_SS2,     VT_F_FINAL}, // ESC N
    {'O',          VT_A_SS3,     VT_F_FINAL}, // ESC O
    {'P',          VT_A_DCS,     VT_F_FINAL}, // ESC P
    {'X',          VT_A_SOS,     VT_F_FINAL}, // ESC X
    {'^',          VT_A_PM,      VT_F_FINAL}, // ESC ^
    {'_',          VT_A_APC,     VT_F_FINAL}, // ESC _

    // TODO More to be added here
};

#define STATE_PTR(X) ((vt_state_t*)X) 

vt_state_t* vt_state_grp[] = {
    STATE_PTR(vt_states_ground),
    STATE_PTR(vt_states_c0),
    STATE_PTR(vt_states_esc)
};

#define COUNT_ARR(X) ((sizeof (X))/(sizeof (X)[0]))

uint8_t vt_state_grp_len[] = {
    COUNT_ARR(vt_states_ground),
    COUNT_ARR(vt_states_c0),
    COUNT_ARR(vt_states_esc)
};

typedef struct {
    vt_g_t state; // Current state group
} vt_parser_t;

void vt_parser_init(vt_parser_t *p) {
    p->state = VT_G_GROUND;
}

bool vt_parser_match_ch(vt_match_t m, vt_char_t ch) {
    if (m < 0) {
        switch(m) {
            case VT_M_CHAR:     return ch >= 0x20 && ch != 0x7F;
            case VT_M_NP_LOW:   return ch < 0x20;
            default:            return 0;
        }
    }
    else {
        return m == ch;
    }
}

vt_state_t* vt_parser_put_ch(vt_parser_t *p, vt_char_t ch) {
        
    vt_state_t* gps = vt_state_grp[p->state];
    uint8_t gpl = vt_state_grp_len[p->state];

    for (uint8_t i = 0; i < gpl;) {
        vt_state_t* s = &gps[i];
        if (vt_parser_match_ch(s->m, ch)) {
            p->state = s->f & VT_F_FINAL ? VT_G_GROUND : (vt_g_t)s->n;
            if (s->f & (VF_F_NEXT_CH | VT_F_FINAL)) {
                // Need next char to determine action
                return s;
            }
            else {
                // Keep processing with the current char
                gps = vt_state_grp[p->state];
                gpl = vt_state_grp_len[p->state];
                i = 0;
                continue;
            }
        }
        ++i;
    }
    p->state = VT_G_GROUND;
    return NULL;
}

#include <assert.h>

int main() {
    vt_parser_t p;
    vt_parser_init(&p);

    vt_state_t* s0 = vt_parser_put_ch(&p, 0x1B); // ESC
    assert(s0 != NULL);
    assert(s0->m == VT_M_C0_ESC);
    assert(p.state == VT_G_ESC);

    vt_state_t* s1 = vt_parser_put_ch(&p, 'D'); // D
    assert(s1 != NULL);
    assert(s1->m == 'D');
    assert(p.state == VT_G_GROUND);

    return 0;
}   


