#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

// Invader types, from the top row down, then the mystery ship
#define INV_INVADER_TYPE_COUNT 3
#define INV_INVADER_TYPE_MOT 3

SpriteId inv_invaders_init(SpriteId start);
void inv_invader_update(uint32_t frame);

// Invaders still marching
uint32_t inv_invaders_alive(void);
// True once every invader has been shot and finished exploding
bool inv_invaders_cleared(void);
// True once the invaders have reached the gun
bool inv_invaders_landed(void);
// Where to shoot from to hit the invader nearest gx, allowing for the march
bool inv_invaders_aim(int32_t gx, int32_t* tx);

// The title, spelt out in invaders
SpriteId inv_invaders_init_title(SpriteId start);
void inv_invaders_update_title(uint32_t frame, uint32_t end);

// Graphics for showing the invaders off outside of the game
const Tile16x8p2_t* inv_invaders_tile(uint32_t type, uint32_t anim);
hstx_dvi_pixel_t* inv_invaders_pallet(uint32_t type);
uint32_t inv_invaders_type_score(uint32_t type);

#ifdef __cplusplus
}
#endif
