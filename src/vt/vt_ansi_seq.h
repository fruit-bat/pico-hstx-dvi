#pragma once

#ifdef __cplusplus
extern "C" {
#endif 

typedef uint8_t vt_char_t;  // The character type

typedef int16_t vt_match_t; // Match symbol

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

    VT_A_NONE         // Do nothing!
} vt_a_t;

typedef enum {
    VT_F_NONE    = 0x00,
    VT_F_NEXT_CH = 0x01, // Another charater is needed to complete the sequence
    VT_F_FINAL   = 0x02, // Leaf node (n) now contains an action code VT_A_xxx
    VT_F_COL_P   = 0x04, // Collect parameter
    VT_F_NXT_P   = 0x08, // Next parameter
    VT_F_COL_CH  = 0x10, // Collect a character
} vt_f_t;   

typedef struct {
    vt_match_t m; // A symbol to match
    int16_t n; // The next state or the action to take
    uint8_t f; // Flags
} vt_state_t;

typedef struct {
    vt_state_t* state; // Current state
    uint32_t params[16]; // Parameters collected
    uint8_t n_params; // Number of parameters collected
    vt_char_t ch;
} vt_parser_t;

void vt_parser_init(vt_parser_t *p);

vt_state_t* vt_parser_put_ch(vt_parser_t *p, vt_char_t ch);

vt_state_t* vt_parser_put_str(vt_parser_t *p, vt_char_t* s);

vt_a_t vt_parser_action(vt_parser_t *p);

#ifdef __cplusplus
}
#endif 
