#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void ww_items_init(void);

// Plants the magic flowers and leaves the bear's piece of the charm by it
void ww_items_new_night(uint32_t bear_room, int32_t bear_x, int32_t bear_y);

// Checks for the witch picking things up in the current room
void ww_items_update(uint32_t frame);

// A piece of the charm flies from x, y in the room to the HUD
void ww_items_award_piece(uint32_t piece, int32_t x, int32_t y);
bool ww_items_flying(void);

// What is lying about in the current room, for the demo player
bool ww_items_piece_here(int32_t* x, int32_t* y);
bool ww_items_flower_here(int32_t* x, int32_t* y, uint32_t* effect);

// On the ground, so drawn behind everything
void ww_items_draw(uint32_t frame);
void ww_items_draw_flying(uint32_t frame);
// For outside of the game, sx and sy being the top left on the screen
void ww_items_draw_flower(uint32_t effect, int32_t sx, int32_t sy, uint32_t frame);

#ifdef __cplusplus
}
#endif
