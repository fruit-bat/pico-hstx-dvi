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

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "vt_parser.h"

// See :
//  https://vt100.net/docs/vt510-rm/chapter4.html#:~:text=A%20DCS%20control%20string%20is,or%20a%20soft%20character%20set.&text=Ignored.&text=Makes%20the%20terminal%20send%20its,the%20ANSI%20DA%20sequence%20instead.
//  https://en.wikipedia.org/wiki/ANSI_escape_code

#define RC(R,C) ((R<<4)+(C))
typedef enum {
    VT_M_CHAR    = -1,   // A normal character
    VT_M_NP_LOW  = -2,   // A non printing char (below 0x20 ' ')
    VT_M_DIGIT   = -3,   // A digit 0-9
    VT_M_ANY     = -4,   // Any char
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
    VT_G_NONE = -1,
    VT_G_GROUND = 0,
    VT_G_C0,
    VT_G_ESC,
    VT_G_CSI,
    VT_G_CSI_P,
    VT_G_CSI_F,
    VT_G_OSC,
    VT_G_OSC_P1,
    VT_G_OSC_P2,
    VT_G_OSC_P3,
    VT_G_OSC_P4,
    VT_G_OSC_P5,
    VT_G_OSC_P6,
    VT_G_OSC_P7,
    VT_G_OSC_T,
    VT_G_OSC_C,
    VT_G_OSC_D,
} vt_g_t;

const vt_state_t vt_states_ground[] = {
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
const vt_state_t vt_states_c0[] = {
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
const vt_state_t vt_states_esc[] = {
    {'[',          VT_G_CSI,          VT_F_NEXT_CH}, // ESC [
    {']',          VT_G_OSC,          VT_F_NEXT_CH}, // ESC ]
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
const vt_state_t vt_states_csi[] = {
    {'s',          VT_A_SAVE_CUR,     VT_F_FINAL},   // ESC [s
    {'u',          VT_A_RESTORE_CUR,  VT_F_FINAL},   // ESC [u
    {VT_M_DIGIT,   VT_G_CSI_P,        VT_F_NXT_P|VT_F_COL_P|VT_F_NEXT_CH}, // ESC [0-9
    {';',          VT_G_CSI_P,        VT_F_NXT_P},
    {VT_M_CHAR,    VT_G_CSI_F,        VT_F_NONE},    // ESC [c
};
const vt_state_t vt_states_csi_p[] = {
    {VT_M_DIGIT,   VT_G_CSI_P,        VT_F_COL_P|VT_F_NEXT_CH}, // Collect param digits
    {';',          VT_G_CSI_P,        VT_F_NXT_P|VT_F_NEXT_CH}, // Next param
    {VT_M_CHAR,    VT_G_CSI_F,        VT_F_NONE}, // ESC [0-9;s
};
const vt_state_t vt_states_csi_f[] = {
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
const vt_state_t vt_states_osc[] = {
    {'P',          VT_G_OSC_P1,       VT_F_NEXT_CH},  // Linux set pallet ESC ] P n r r g g b b
    {'R',          VT_A_XPALR,        VT_F_FINAL},    // Linux reset pallet ESC ] R
    // TODO at least parse out the command - up to the first ;
    // The command can go into the numeric parameters
    {VT_M_CHAR,    VT_G_OSC_T,        VT_F_NONE},     // ESC ]
};
const vt_state_t vt_states_osc_p1[] = {{VT_M_CHAR, VT_G_OSC_P2, VT_F_COL_OSC|VT_F_NEXT_CH}};
const vt_state_t vt_states_osc_p2[] = {{VT_M_CHAR, VT_G_OSC_P3, VT_F_COL_OSC|VT_F_NEXT_CH}};
const vt_state_t vt_states_osc_p3[] = {{VT_M_CHAR, VT_G_OSC_P4, VT_F_COL_OSC|VT_F_NEXT_CH}};
const vt_state_t vt_states_osc_p4[] = {{VT_M_CHAR, VT_G_OSC_P5, VT_F_COL_OSC|VT_F_NEXT_CH}};
const vt_state_t vt_states_osc_p5[] = {{VT_M_CHAR, VT_G_OSC_P6, VT_F_COL_OSC|VT_F_NEXT_CH}};
const vt_state_t vt_states_osc_p6[] = {{VT_M_CHAR, VT_G_OSC_P7, VT_F_COL_OSC|VT_F_NEXT_CH}};
const vt_state_t vt_states_osc_p7[] = {{VT_M_CHAR, VT_A_XPALS,  VT_F_COL_OSC|VT_F_FINAL}};
const vt_state_t vt_states_osc_t[] = {
    {VT_M_C0_BEL,  VT_A_OSC,          VT_F_FINAL}, // OSC terminator
    {VT_M_ST,      VT_A_OSC,          VT_F_FINAL}, // OSC terminator
    {VT_M_DIGIT,   VT_G_OSC_C,        VT_F_NXT_P|VT_F_COL_P|VT_F_NEXT_CH}, // ESC [0-9
};
const vt_state_t vt_states_osc_c[] = {
    {VT_M_C0_BEL,  VT_A_OSC,          VT_F_FINAL}, // OSC terminator
    {VT_M_ST,      VT_A_OSC,          VT_F_FINAL}, // OSC terminator
    {';',          VT_G_OSC_D,        VT_F_NEXT_CH},
    {VT_M_ANY,     VT_G_OSC_C,        VT_F_COL_OSC|VT_F_NEXT_CH} // Collect OSC character
};
const vt_state_t vt_states_osc_d[] = {
    {VT_M_C0_BEL,  VT_A_OSC,          VT_F_FINAL}, // OSC terminator
    {VT_M_ST,      VT_A_OSC,          VT_F_FINAL}, // OSC terminator
    {VT_M_ANY,     VT_G_OSC_D,        VT_F_COL_OSC|VT_F_NEXT_CH} // Collect OSC character  
};

#define STATE_PTR(X) ((vt_state_t*)X) 

const vt_state_t* vt_state_grp[] = {
    STATE_PTR(vt_states_ground),
    STATE_PTR(vt_states_c0),
    STATE_PTR(vt_states_esc),
    STATE_PTR(vt_states_csi),
    STATE_PTR(vt_states_csi_p),
    STATE_PTR(vt_states_csi_f),
    STATE_PTR(vt_states_osc),
    STATE_PTR(vt_states_osc_p1),
    STATE_PTR(vt_states_osc_p2),
    STATE_PTR(vt_states_osc_p3),
    STATE_PTR(vt_states_osc_p4),
    STATE_PTR(vt_states_osc_p5),
    STATE_PTR(vt_states_osc_p6),
    STATE_PTR(vt_states_osc_p7),
    STATE_PTR(vt_states_osc_t),
    STATE_PTR(vt_states_osc_c),
    STATE_PTR(vt_states_osc_d),
};

#define COUNT_ARR(X) ((sizeof (X))/(sizeof (X)[0]))

const uint8_t vt_state_grp_len[] = {
    COUNT_ARR(vt_states_ground),
    COUNT_ARR(vt_states_c0),
    COUNT_ARR(vt_states_esc),
    COUNT_ARR(vt_states_csi),
    COUNT_ARR(vt_states_csi_p),
    COUNT_ARR(vt_states_csi_f),
    COUNT_ARR(vt_states_osc),
    COUNT_ARR(vt_states_osc_p1),
    COUNT_ARR(vt_states_osc_p2),
    COUNT_ARR(vt_states_osc_p3),
    COUNT_ARR(vt_states_osc_p4),
    COUNT_ARR(vt_states_osc_p5),
    COUNT_ARR(vt_states_osc_p6),
    COUNT_ARR(vt_states_osc_p7),
    COUNT_ARR(vt_states_osc_t),
    COUNT_ARR(vt_states_osc_c),
    COUNT_ARR(vt_states_osc_d),
};

void vt_parser_init(vt_parser_t *p) {
    p->state = NULL;
    p->n_params = 0;
    p->osc_param_len = 0;
}

bool vt_parser_match_ch(vt_match_t m, vt_char_t ch) {
    if (m < 0) {
        switch(m) {
            case VT_M_ANY:      return true;
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

const vt_state_t* vt_parser_put_ch(vt_parser_t *p, vt_char_t ch) {
        
    const vt_state_t* ps = p->state;
    vt_g_t g = ps && !(ps->f & VT_F_FINAL) ? (vt_g_t)ps->n : VT_G_GROUND;
    if (g == VT_G_GROUND) {
        p->n_params = 0;
        p->osc_param_len = 0;
        p->osc_param[0] = 0;
    }
    const vt_state_t* gps = vt_state_grp[g];
    uint8_t gpl = vt_state_grp_len[g];

    for (uint8_t i = 0; i < gpl;) {
        const vt_state_t* s = &gps[i];
        if (vt_parser_match_ch(s->m, ch)) {
            p->state = s;
            if (s->f & VT_F_COL_CH) {
                p->ch = ch;
            }
            if (s->f & VT_F_COL_OSC) {
                if (p->osc_param_len + 2 < COUNT_ARR(p->osc_param)) {
                    p->osc_param[p->osc_param_len++] = ch;
                    p->osc_param[p->osc_param_len] = 0;
                }
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

const vt_state_t* vt_parser_put_str(vt_parser_t *p, vt_char_t* s) {
    const vt_state_t* last = NULL;
    for (vt_char_t* c = s; *c; ++c) {
        last = vt_parser_put_ch(p, *c);
    }
    return last;
}

vt_a_t vt_parser_action(vt_parser_t *p) {
    return (p == NULL) || (p->state == NULL) || ((p->state->f & VT_F_FINAL) ==0)
        ? VT_A_NONE
        : p->state->n;
}

