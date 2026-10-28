#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// The long range scanner at the top of the screen shows the whole world,
// centred on the ship

// Clears the scanner, ready for this frame's dots
void def_scanner_begin(int32_t centre);
void def_scanner_dot(int32_t x, int32_t y, uint32_t colour);
void def_scanner_draw(void);

#ifdef __cplusplus
}
#endif
