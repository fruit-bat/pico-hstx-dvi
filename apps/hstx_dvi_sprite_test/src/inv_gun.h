#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INV_GUN_W 24

SpriteId inv_gun_init(SpriteId start);

// input is the button state, from the player or the demo
void inv_gun_update(uint32_t frame, uint8_t input);

uint32_t inv_gun_get_lives(void);

void inv_gun_add_life(void);

// Takes all the remaining lives, for when the invaders have landed
void inv_gun_destroy(uint32_t frame);

bool inv_gun_is_alive(void);

int32_t inv_gun_get_x(void);

int32_t inv_gun_get_y(void);

#ifdef __cplusplus
}
#endif

