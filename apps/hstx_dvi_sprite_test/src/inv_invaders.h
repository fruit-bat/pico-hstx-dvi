#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"
#include "inv_alien.h"

#ifdef __cplusplus
extern "C" {
#endif

// The alien types, see inv_alien.h, then the mystery ship
#define INV_INVADER_TYPE_COUNT INV_ALIEN_TYPE_COUNT
#define INV_INVADER_TYPE_MOT INV_ALIEN_TYPE_COUNT

SpriteId inv_invaders_init(SpriteId start);
void inv_invader_update(uint32_t frame);

// Invaders still about, in the formation or not
uint32_t inv_invaders_alive(void);
// True once every invader has been shot and finished exploding
bool inv_invaders_cleared(void);
// True once the formation has reached the gun
bool inv_invaders_landed(void);
// Where to shoot from to hit the invader nearest gx, allowing for the march
bool inv_invaders_aim(int32_t gx, int32_t* tx);
// Looks for a diving invader between x0 and x1 that is below y0. Returns
// false if there are none, otherwise sets dx to the middle of it.
bool inv_invaders_diver_threat(int32_t x0, int32_t x1, int32_t y0, int32_t* dx);

// For showing the invaders off outside of the game, on sprites handed out each frame
void inv_invaders_draw_icon(uint32_t type, int32_t x, int32_t y, uint32_t frame);
uint32_t inv_invaders_type_score(uint32_t type);
const char* inv_invaders_type_name(uint32_t type);

#ifdef __cplusplus
}
#endif
