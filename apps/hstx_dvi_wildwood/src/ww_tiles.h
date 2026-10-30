#pragma once

#include "pico/stdlib.h"
#include "game_render.h"

#ifdef __cplusplus
extern "C" {
#endif

// Each room is a screen of 32 pixel square tiles under the HUD
#define WW_TILE 32
#define WW_TILE_SHIFT 5
#define WW_HUD_H 32
#define WW_ROOM_COLS (GAME_SCREEN_W / WW_TILE)
#define WW_ROOM_ROWS ((GAME_SCREEN_H - WW_HUD_H) / WW_TILE)
#define WW_ROOM_W (WW_ROOM_COLS * WW_TILE)
#define WW_ROOM_H (WW_ROOM_ROWS * WW_TILE)
#define WW_ROOM_CELLS (WW_ROOM_COLS * WW_ROOM_ROWS)

// What is in each tile of a room. Bushes and anything after them are solid.
#define WW_L_GRASS 0
#define WW_L_FLOWERS 1
#define WW_L_MUSHROOM 2
#define WW_L_BUSH 3
#define WW_L_ROCK 4
#define WW_L_WATER 5
// A tree takes 4 tiles, top left, top right, bottom left, bottom right
#define WW_L_TREE 6
#define WW_L_TYPE 0x0f
// Set on tiles that can be reached from the way in to the room
#define WW_L_REACH 0x80

__force_inline bool ww_tile_solid(const uint8_t l) {
	return (l & WW_L_TYPE) >= WW_L_BUSH;
}

void ww_tiles_init(void);

// Picks the pictures for a room from what is in it. seed varies the grass.
void ww_tiles_build(uint8_t* gfx, const uint8_t* logic, uint32_t seed);

// Draws a room built by ww_tiles_build straight into the rows, so it has to go first
void ww_tiles_draw(const uint8_t* gfx, uint32_t frame);

#ifdef __cplusplus
}
#endif
