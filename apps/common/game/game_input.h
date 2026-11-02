#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Gamepad buttons for the pad map. A NES pad's A and B sit where a SNES pad's
// B and Y do, so SNES A and X are added to them.
#define GAME_PAD_A      0x0101 // NES A, SNES B and A
#define GAME_PAD_B      0x0202 // NES B, SNES Y and X
#define GAME_PAD_SELECT 0x0004
#define GAME_PAD_START  0x0008
#define GAME_PAD_UP     0x0010
#define GAME_PAD_DOWN   0x0020
#define GAME_PAD_LEFT   0x0040
#define GAME_PAD_RIGHT  0x0080
#define GAME_PAD_L      0x0400 // SNES shoulders
#define GAME_PAD_R      0x0800
// A SNES pad's A and X alone, to give them uses of their own
#define GAME_PAD_SNES_A 0x0100
#define GAME_PAD_SNES_X 0x0200

// Buttons on count consecutive GPIOs from first, each connecting the GPIO to
// ground, when built with GAME_BUTTONS. The first button is bit 0, and there
// can be up to 16 bits.
// When built with GAME_NESPAD, NES or SNES pads work too. pad_map has count
// entries of GAME_PAD_ buttons, the ones that press each bit.
void game_input_init(uint32_t first, uint32_t count, const uint16_t *pad_map);
// As game_input_init, but with more bits from a pad than there are GPIOs:
// pad_map has pad_count entries
void game_input_init_pad(uint32_t first, uint32_t count, const uint16_t *pad_map, uint32_t pad_count);

uint16_t game_input_get(void);

// Buttons that have gone down since the last call. Call it once a frame, as
// it also takes what a keyboard has sent.
uint16_t game_input_pressed(void);

// A PS/2 keyboard, when built with GAME_PS2KBD, on GPIO 0 for data and 1 for
// the clock. Keys are HID codes, as in ps2kbd_hid.h. Without one, no key is
// ever held or typed.
typedef struct {
	uint8_t key;   // HID_KEY_...
	uint16_t bits; // the input bits it presses, as the buttons do
} GameKey_t;
// Keys that press input bits, as buttons or a pad do; keys is kept
void game_input_set_keys(const GameKey_t *keys, uint32_t count);
// Whether there is a keyboard to use
bool game_keyboard(void);
// The input bits the keyboard presses now, or pressed in the frame
uint16_t game_input_keys(void);
// Whether a key is held, and whether it went down in the frame, as taken by
// game_input_pressed
bool game_key_held(uint8_t key);
bool game_key_pressed(uint8_t key);
// The modifiers held, KEYBOARD_MODIFIER_...
uint8_t game_key_modifiers(void);
// The next character typed, or 0 if none
char game_key_char(void);

#ifdef __cplusplus
}
#endif
