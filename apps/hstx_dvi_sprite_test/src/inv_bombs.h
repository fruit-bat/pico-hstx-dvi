#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

SpriteId inv_bombs_init(SpriteId start);

void inv_bombs_update(void);

void inv_bombs_fire(SpriteId invSpriteId);

void inv_bombs_clear(void);

// Bombs in flight
uint32_t inv_bombs_active(void);

// Looks for the lowest bomb between x0 and x1 that is below y0. Returns false
// if there are none, otherwise sets bx to the x position of the bomb.
bool inv_bombs_threat(int32_t x0, int32_t x1, int32_t y0, int32_t* bx);

#ifdef __cplusplus
}
#endif

