#pragma once

#include "pico/stdlib.h"
#include "game_art.h"

#ifdef __cplusplus
extern "C" {
#endif

// The aliens, from the top row down
#define INV_ALIEN_COMMANDER 0
#define INV_ALIEN_CRAB 1
#define INV_ALIEN_JELLY 2
#define INV_ALIEN_TYPE_COUNT 3

#define INV_ALIEN_W 24
#define INV_ALIEN_LAYERS 3
#define INV_ALIEN_ANIM 4

// Where the eyes are looking
#define INV_ALIEN_LOOK_LEFT 0
#define INV_ALIEN_LOOK_AHEAD 1
#define INV_ALIEN_LOOK_RIGHT 2
#define INV_ALIEN_BLINK 3
#define INV_ALIEN_LOOKS 4

void inv_alien_init(void);

// anim goes round INV_ALIEN_ANIM frames
const GameArt_t* inv_alien_art(uint32_t type, uint32_t anim, uint32_t look);

uint32_t inv_alien_height(uint32_t type);
// The main colour, and one for the bits picked out
uint32_t inv_alien_colour(uint32_t type);
uint32_t inv_alien_detail(uint32_t type);

#ifdef __cplusplus
}
#endif
