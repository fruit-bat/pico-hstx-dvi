// Copyright (c) 2024 fruit-bat.

// The game's pictures, made once at the start, all twice the size of their
// pixel art unless said otherwise
#pragma once

#include "rr_art.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	RrArt_t ship, ship_small, flame[2];
	RrArt_t drone[2], orb[2];
	RrArt_t shot, twin, laser, bomb;
	RrArt_t capsule[2];
	RrArt_t dart[2], carrier[2], saucer[2];
	RrArt_t rocket, rocket_flame[2], fuel;
	RrArt_t turret[3], turret_c[3]; // left, up and left, up; on the ceiling down
	RrArt_t meteor, rock;
	RrArt_t bullet[2];
	RrArt_t hull, core[3], wall;
} RrGfx_t;

extern RrGfx_t rr_gfx;

void rr_gfx_init(void);

#ifdef __cplusplus
}
#endif
