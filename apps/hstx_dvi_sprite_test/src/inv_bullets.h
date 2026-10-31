#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

SpriteId inv_bullets_init(SpriteId start);

void inv_bullets_update(void);

// x is where the bullet goes up, y the bottom of it
void inv_bullets_fire(int32_t x, int32_t y);

void inv_bullets_clear(void);

// Shots fired since inv_bullets_init
uint32_t inv_bullets_get_shots(void);

#ifdef __cplusplus
}
#endif

