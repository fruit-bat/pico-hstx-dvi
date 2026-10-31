#pragma once

#include "pico/stdlib.h"
#include "game_util.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MM_GRAVITY 90
#define MM_FALL_MAX GAME_PX(8)

// Something that falls and bumps into the house. x is the middle and y the
// bottom, in fixed point.
typedef struct {
	int32_t x, y;
	int32_t vx, vy;
	int16_t hw; // half the width, in pixels
	int16_t h;
	bool ground;
	bool wall; // bumped into something going across
	bool head; // bumped into something going up
} MmBody_t;

// Moves by vx, vy, stopping at walls, crates and things to stand on. drop
// falls through shelves. self is left out when looking for crates and things
// to stand on, so a crate doesn't bump into itself.
void mm_body_move(MmBody_t* b, bool drop, const void* self);

// Whether there is room for the body with its middle and bottom at x, y, in pixels
bool mm_body_fits(const MmBody_t* b, int32_t x, int32_t y, const void* self);

// Whether there is something to stand on at x, y, in pixels
bool mm_body_floor(const MmBody_t* b, int32_t x, int32_t y, bool drop, const void* self);

#ifdef __cplusplus
}
#endif
