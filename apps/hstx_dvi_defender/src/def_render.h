#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEF_SCREEN_W MODE_H_ACTIVE_PIXELS
#define DEF_SCREEN_H MODE_V_ACTIVE_LINES
#define DEF_BITMAP_WORDS (DEF_SCREEN_W >> 5)
#define DEF_TEXT_MAX 40

// A line of text that can be drawn at 1 or more times the size of the font
typedef struct {
	TextGrid8_t grid;
	uint32_t scale;
	uint8_t s[DEF_TEXT_MAX];
} DefText_t;

// A block of pixels, one byte each, held as words
typedef struct {
	const uint32_t* data;
	uint32_t words; // per row
} DefPixels_t;

// The sprites are handed out afresh every frame, so only what is on the
// screen uses one. Lower ids take priority, so add the most important first.
void def_spr_begin(void);
Sprite* def_spr_add(int32_t x, int32_t y, uint32_t w, uint32_t h, void* d1, void* d2, SpriteRenderer r);
void def_spr_end(void);

// The following draw straight into the row, without taking part in sprite
// priority. Ordinary sprites draw over them, so add these first.

// Fills the whole screen
void def_spr_fill(hstx_dvi_pixel_t* p);
void def_spr_hline(int32_t x, int32_t y, uint32_t len, hstx_dvi_pixel_t* p);
// A laser beam, solid at the head and breaking up towards the tail
void def_spr_laser(int32_t x, int32_t y, uint32_t len, bool head_left, hstx_dvi_pixel_t* p);
// A screen wide 1 bit per pixel bitmap, DEF_BITMAP_WORDS per row with bit 31 leftmost
void def_spr_bitmap(int32_t y, uint32_t rows, const uint32_t* bits, hstx_dvi_pixel_t* p);
// x and the width must be multiples of 4, and it must be wholly on the screen
void def_spr_pixels(int32_t x, int32_t y, uint32_t w, uint32_t h, const DefPixels_t* pixels);

void def_text_set(DefText_t* t, const char* s, uint32_t scale);
// Only shows the first n characters, for typing text out
void def_text_set_n(DefText_t* t, const char* s, uint32_t n, uint32_t scale);
int32_t def_text_width(const DefText_t* t);
void def_spr_text(DefText_t* t, int32_t x, int32_t y, hstx_dvi_pixel_t* p);
void def_spr_text_centre(DefText_t* t, int32_t y, hstx_dvi_pixel_t* p);

#ifdef __cplusplus
}
#endif
