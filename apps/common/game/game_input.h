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

// Buttons on count consecutive GPIOs from first, each connecting the GPIO to
// ground. The first button is bit 0.
// When built with GAME_NESPAD, NES or SNES pads work too. pad_map has count
// entries of GAME_PAD_ buttons, the ones that press each bit.
void game_input_init(uint32_t first, uint32_t count, const uint16_t *pad_map);

uint8_t game_input_get(void);

// Buttons that have gone down since the last call
uint8_t game_input_pressed(void);

#ifdef __cplusplus
}
#endif
