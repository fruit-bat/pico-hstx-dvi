#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Buttons on count consecutive GPIOs from first, each connecting the GPIO to
// ground. The first button is bit 0.
void game_input_init(uint32_t first, uint32_t count);

uint8_t game_input_get(void);

// Buttons that have gone down since the last call
uint8_t game_input_pressed(void);

#ifdef __cplusplus
}
#endif
