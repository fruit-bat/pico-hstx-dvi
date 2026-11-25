// Copyright (c) 2024 fruit-bat.

// Big, bold pictures: pixel art of any colours, drawn a whole number of
// times the size, from rows of characters as game_art does. Each row is kept as runs of
// one colour, which the renderer draws as spans, so a picture costs about the
// same however big it is drawn.
#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RR_ART_MAX_H 64
// The most runs in a row of a picture
#define RR_ART_MAX_RUNS 40

typedef struct {
	uint8_t x0, x1; // x1 not included
	hstx_dvi_pixel_t p;
} RrRun_t;

typedef struct {
	uint8_t w, h;  // in the picture's own pixels
	uint8_t scale; // drawn this many times the size
	const uint16_t* row; // the first run of each row, h + 1 of them
	const RrRun_t* run;
} RrArt_t;

// The size on the screen
static inline int32_t rr_art_w(const RrArt_t* a) { return a->w * a->scale; }
static inline int32_t rr_art_h(const RrArt_t* a) { return a->h * a->scale; }

// Makes a picture from h rows of characters. Each character in key is drawn
// in the matching pixel; anything else is see through. Rows can be of
// different lengths. scale is 1 to 8, and flip turns it upside down.
void rr_art_make(RrArt_t* a, const char* const* rows, uint32_t h, const char* key,
	const hstx_dvi_pixel_t* pixels, uint32_t scale, bool flip);

// Draws it with its top left at sx, sy. With a flash, every pixel is that
// colour instead, for something hit.
void rr_art_draw(const RrArt_t* a, int32_t sx, int32_t sy);
void rr_art_draw_flash(const RrArt_t* a, int32_t sx, int32_t sy, hstx_dvi_pixel_t flash);

// A plain block of colour, w by h
void rr_spr_rect(int32_t x, int32_t y, uint32_t w, uint32_t h, hstx_dvi_pixel_t p);

#define RR_ROWS(rows) (sizeof(rows) / sizeof((rows)[0]))

#ifdef __cplusplus
}
#endif
