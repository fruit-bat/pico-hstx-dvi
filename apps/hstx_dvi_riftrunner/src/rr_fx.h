// Copyright (c) 2024 fruit-bat.

// Big explosions: a ball of fire that swells, burns out from the middle and
// throws chunks about. Positions are in the world, fixed point, as rr_world.
#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	RR_BOOM_SMALL = 0,
	RR_BOOM_MEDIUM,
	RR_BOOM_BIG,
	RR_BOOM_HUGE,
} RrBoom_e;

void rr_fx_init(void);
// At the middle of what blew up
void rr_fx_explode(int32_t x, int32_t y, uint32_t size);
void rr_fx_update(void);
void rr_fx_draw(void);

// A disc of radius r, up to RR_FX_MAX_R, with a hole of radius inner, or 0
// for none, at screen x, y
#define RR_FX_MAX_R 63
void rr_fx_disc(int32_t x, int32_t y, uint32_t r, uint32_t inner, hstx_dvi_pixel_t p);

#ifdef __cplusplus
}
#endif
