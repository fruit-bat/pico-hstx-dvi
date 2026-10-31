#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

// Pictures with up to 15 colours, 4 bits a pixel, drawn 1, 2 or 4 times
// their size straight into the row. They draw over whatever is there, so
// they are added back to front.
#define MM_IMG_MAX_W 32
#define MM_IMG_MAX_H 32
#define MM_IMG_WORDS (MM_IMG_MAX_W / 8)
#define MM_PAL_SIZE 16

typedef struct {
	uint8_t w;
	uint8_t h;
	// Words used in each row, 8 pixels to a word with the leftmost in the bottom bits
	uint8_t words;
	uint32_t d[MM_IMG_MAX_H * MM_IMG_WORDS];
} MmImg_t;

// The colours for a picture. Entry 0 is see through.
typedef hstx_dvi_pixel_t MmPal_t[MM_PAL_SIZE];

// Draw flags
#define MM_GFX_FLIP 1 // Left to right
#define MM_GFX_GLASS 2 // Half see through

// For putting a picture together from parts. '.' is see through.
typedef struct {
	uint32_t w;
	uint32_t h;
	char c[MM_IMG_MAX_H][MM_IMG_MAX_W + 1];
} MmCanvas_t;

void mm_canvas_clear(MmCanvas_t* c, uint32_t w, uint32_t h);
// '.' in the rows leaves the canvas as it was and ' ' makes it see through
void mm_canvas_stamp(MmCanvas_t* c, const char* const* rows, uint32_t n, int32_t x, int32_t y);
void mm_canvas_set(MmCanvas_t* c, int32_t x, int32_t y, char ch);

// Each character of key is a colour, the first being colour 1 of the pallet.
// Anything else is see through.
void mm_img_make(MmImg_t* img, const char* const* rows, uint32_t h, const char* key);
void mm_img_from_canvas(MmImg_t* img, const MmCanvas_t* c, const char* key);

// sx, sy is the top left on the screen. scale is 1, 2 or 4. The pallet has
// to stay put until the frame is drawn, so it can't be on the stack.
void mm_gfx_draw(const MmImg_t* img, const hstx_dvi_pixel_t* pal, int32_t sx, int32_t sy, uint32_t scale, uint32_t flags);

// A colour moved towards another by t out of 256
uint32_t mm_gfx_mix(uint32_t a, uint32_t b, uint32_t t);

// A block of one colour, colour[0], which has to stay put like a pallet
void mm_gfx_rect(int32_t x, int32_t y, uint32_t w, uint32_t h, const hstx_dvi_pixel_t* colour);

#ifdef __cplusplus
}
#endif
