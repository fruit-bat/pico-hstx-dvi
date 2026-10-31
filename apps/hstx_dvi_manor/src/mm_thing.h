#pragma once

#include "pico/stdlib.h"
#include "mm_body.h"

#ifdef __cplusplus
extern "C" {
#endif

// Everything in the house that moves about or can be picked up
enum {
	MM_T_NONE = 0,
	MM_T_GHOST,
	MM_T_BAT,
	MM_T_SPIDER,
	MM_T_PUMPKIN,
	MM_T_KITTEN,
	MM_T_SWEET,
	MM_T_HEART,
	MM_T_KEY,
	MM_T_FEATHER,
	MM_T_CRATE,
};

#define MM_KITTENS 5

// Creatures stay cheerful for less time on later nights
void mm_thing_reset(uint32_t night);
// Adds a thing from the map, see mm_map.c
void mm_thing_spawn(char ch, int32_t cx, int32_t cy, uint32_t group);

void mm_thing_update(uint32_t frame);
// Things behind Hazel. Creatures come after things to pick up.
void mm_thing_draw(uint32_t frame);

// For bumping into. Pixels x0 <= x < x1, y0 <= y < y1.
bool mm_thing_crate_box(int32_t x0, int32_t y0, int32_t x1, int32_t y1, const void* self);
// Whether a happy creature's top is pixel row y, between x0 and x1
bool mm_thing_top_at(int32_t x0, int32_t x1, int32_t y, const void* self);
// Pushes a crate touching the box by dx pixels. Returns false if it won't go.
bool mm_thing_push(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t dx);
// Whether a crate fills most of a tile, for the plates
bool mm_thing_crate_in(int32_t cx, int32_t cy);

// A spell, hitting whatever it touches first. Returns true if it hit something.
bool mm_thing_cast(int32_t x0, int32_t y0, int32_t x1, int32_t y1);

// Hazel picks things up and bumps into grumpy creatures
void mm_thing_touch(int32_t x0, int32_t y0, int32_t x1, int32_t y1);

uint32_t mm_thing_kittens_found(void);
uint32_t mm_thing_sweets_found(void);
uint32_t mm_thing_cheered(void);
// The colour of each kitten, for the HUD
uint32_t mm_thing_kitten_mask(void);

// The nearest thing of a type, for the demo player, with x its middle and y
// its bottom. Returns false if there are none left.
bool mm_thing_find(uint32_t type, int32_t near_x, int32_t near_y, int32_t* x, int32_t* y, bool* happy);

#ifdef __cplusplus
}
#endif
