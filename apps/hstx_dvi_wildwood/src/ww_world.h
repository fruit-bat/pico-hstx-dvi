#pragma once

#include "pico/stdlib.h"
#include "ww_tiles.h"
#include "game_fx.h"
#include "game_util.h"

#ifdef __cplusplus
extern "C" {
#endif

// The wood is a grid of rooms, joined up like a maze
#define WW_MAP_W 8
#define WW_MAP_H 6
#define WW_ROOMS (WW_MAP_W * WW_MAP_H)
#define WW_NO_ROOM 0xff

#define WW_DIR_N 0
#define WW_DIR_E 1
#define WW_DIR_S 2
#define WW_DIR_W 3

// The gaps in the edges of a room that lead to the next one
#define WW_GAP_C0 8
#define WW_GAP_R0 5
#define WW_GAP_N 4

// Makes a new wood, using game_rand
void ww_world_new(void);

uint32_t ww_world_start(void);
uint32_t ww_world_room(void);
void ww_world_enter(uint32_t room);

// The way out of a room in each direction, or WW_NO_ROOM
uint32_t ww_world_next(uint32_t room, uint32_t dir);
const uint8_t* ww_world_logic(uint32_t room);

// Pixel positions in the current room. Anywhere off the room is taken from
// the nearest tile on it, so the gaps carry on out.
bool ww_world_solid(int32_t x, int32_t y);
// Corners inclusive
bool ww_world_box_free(int32_t x0, int32_t y0, int32_t x1, int32_t y1);
// Moves a fixed point position by dx, dy, an axis at a time, stopping at
// anything solid. The box is hw either side of x and hh above y, in pixels.
// Slides round corners that are only just in the way. Returns the axes that
// were blocked, 1 for x and 2 for y.
uint32_t ww_world_move(int32_t* x, int32_t* y, int32_t dx, int32_t dy, int32_t hw, int32_t hh);
// True if nothing solid is in the way between two points
bool ww_world_line_clear(int32_t x0, int32_t y0, int32_t x1, int32_t y1);

// Somewhere a creature can stand, well away from the ways in. x and y are
// the pixel position of its feet. Returns false if there is nowhere.
bool ww_world_random_spot(uint32_t room, int32_t* x, int32_t* y);

// Rooms apart, going through the maze
uint32_t ww_world_distance(uint32_t a, uint32_t b);
// The direction to leave a room by to get towards another, or -1
int32_t ww_world_route(uint32_t from, uint32_t to);

// The direction to leave a room by to get to the nearest one not yet visited, or -1
int32_t ww_world_route_unvisited(uint32_t from);

// The next tile to go to on the way between two tiles in the current room.
// Returns false if there is no way.
bool ww_world_step(int32_t fc, int32_t fr, int32_t tc, int32_t tr, int32_t* nc, int32_t* nr);

uint32_t ww_world_visits(uint32_t room);
uint32_t ww_world_visited_count(void);

void ww_world_draw(uint32_t frame);

// Particles from a pixel position in the room
__force_inline void ww_world_burst(int32_t x, int32_t y, uint32_t colour, uint32_t count, int32_t speed) {
	game_fx_explode(GAME_PX(x), GAME_PX(y + WW_HUD_H), colour, count, speed);
}
// The rooms visited so far, in a block 64 by 30 pixels
void ww_world_draw_map(int32_t x, int32_t y, uint32_t frame);

#ifdef __cplusplus
}
#endif
