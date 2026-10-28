#pragma once

#include "pico/stdlib.h"
#include "def_render.h"

#ifdef __cplusplus
extern "C" {
#endif

// World positions are fixed point, with this many fraction bits
#define DEF_FP 8
#define DEF_PX(v) ((v) << DEF_FP)
// The world wraps round after this many pixels
#define DEF_WORLD_W 4096
#define DEF_WORLD_W_FP DEF_PX(DEF_WORLD_W)

// Above here is the score and scanner
#define DEF_HUD_H 58
#define DEF_PLAY_TOP 64
#define DEF_PLAY_BOTTOM (DEF_SCREEN_H - 8)

// The mountains sit in a band at the bottom of the screen
#define DEF_TERRAIN_TOP 408
#define DEF_TERRAIN_ROWS (DEF_SCREEN_H - DEF_TERRAIN_TOP)

__force_inline int32_t def_wrap(const int32_t x) {
	return x & (DEF_WORLD_W_FP - 1);
}

// The signed distance from b to a, the short way round the world
__force_inline int32_t def_dx(const int32_t a, const int32_t b) {
	return (int32_t)(((uint32_t)(a - b) + (DEF_WORLD_W_FP >> 1)) & (DEF_WORLD_W_FP - 1)) - (DEF_WORLD_W_FP >> 1);
}

void def_world_init(void);

void def_world_set_camera(int32_t x);
int32_t def_world_camera(void);

// Where a world x position is on the screen, in pixels
int32_t def_world_screen_x(int32_t x);
bool def_world_on_screen(int32_t x, int32_t w);

// The screen y of the ground at a world x position
int32_t def_world_ground_y(int32_t x);

void def_world_destroy_planet(void);
void def_world_restore_planet(void);
bool def_world_planet_alive(void);

void def_world_draw_terrain(void);
// The stars scroll slower than the world
void def_world_draw_stars(int32_t scroll, uint32_t frame);

#ifdef __cplusplus
}
#endif
