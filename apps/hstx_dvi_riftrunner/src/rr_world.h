// Copyright (c) 2024 fruit-bat.

// The world: the land scrolling by, made up as it comes into view, in
// sections, each its own kind of land, ending with a warship to fight.
// Positions are in the world, fixed point (RR_FP), with x going on for ever
// and y on the screen. The camera is the world x at the left of the screen.
#pragma once

#include "pico/stdlib.h"
#include "game_render.h"
#include "game_util.h"
#include "game_fx.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RR_FP GAME_FP
#define RR_PX(v) GAME_PX(v)

// The score and fuel along the top, the power bar along the bottom
#define RR_PLAY_TOP 40
#define RR_PLAY_BOTTOM 444
#define RR_HUD_BOTTOM 448

// How fast the world goes by, a frame
#define RR_SCROLL 384

#define RR_RGB(r, g, b) HSTX_DVI_PIXEL_RGB(r, g, b)

typedef enum {
	RR_SEC_RIDGES = 0,
	RR_SEC_CAVERNS,
	RR_SEC_DEBRIS,
	RR_SEC_CITY,
	RR_SEC_FORTRESS,
	RR_SEC_WARSHIP,
	RR_SECTIONS
} RrSection_e;

void rr_world_init(void);

// Moves the camera on, making up more land as needed
void rr_world_scroll(int32_t dx);
int32_t rr_world_camera(void);

// Where a world x is on the screen, in pixels
int32_t rr_world_sx(int32_t x);
const GameFxView_t* rr_world_fx_view(void);

// The land at a world x in pixels: the top of the floor, and the bottom of
// the ceiling, as screen y. With no floor it is below the play area, and with
// no ceiling above it.
int32_t rr_world_floor(int32_t px);
int32_t rr_world_ceil(int32_t px);
// The highest floor and lowest ceiling from px0 to px1
int32_t rr_world_floor_min(int32_t px0, int32_t px1);
int32_t rr_world_ceil_max(int32_t px0, int32_t px1);

// The section the land at a world x in pixels belongs to
uint32_t rr_world_section(int32_t px);
const char* rr_world_section_name(uint32_t section);
// Times round all the sections, from 0, in the land being made
uint32_t rr_world_loop(void);
// The land stays clear while the warship is fought; this lets it go on
void rr_world_release_boss(void);
// Makes the land from the start of a section from now on, for trying things out
void rr_world_jump(uint32_t section);

// The land, then the hills and stars behind, added last as they are furthest back
void rr_world_draw_terrain(void);
void rr_world_draw_back(void);
// Just the stars, for the screens between games, scrolled by
void rr_world_draw_stars(uint32_t scroll);

#ifdef __cplusplus
}
#endif
