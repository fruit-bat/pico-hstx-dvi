#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint8_t d[8];
} Tile8x8p2_t;

typedef struct {
	uint16_t d[16];
} Tile16x16p2_t;

typedef struct {
	uint16_t d[8];
} Tile16x8p2_t;

typedef struct {
	uint32_t d[16];
} Tile32x16p2_t;

// Range of characters covered by a TextGrid8_t font (8 bytes per glyph)
#define TEXT_GRID8_FIRST_CHAR 32
#define TEXT_GRID8_LAST_CHAR 126

typedef struct {
	uint16_t w;
	uint8_t *s;
	uint8_t *font;
} TextGrid8_t;

typedef uint8_t SpriteId;
typedef uint32_t SpriteCollisionMask;

typedef void (*SpriteRenderer)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t y,
	const int32_t row,
	const SpriteId spriteId
);

enum SpriteFlags {
  SF_NONE = 0,
  SF_ENABLE = 1
};

typedef struct Sprite {
	int32_t x,y;
	uint16_t w,h,f; // w and h in pixels, used to skip sprites that are off screen
	void *d1, *d2;
	SpriteRenderer r;
} Sprite;

#define MAX_SPRITES ((1<<8)-1)

extern Sprite _sprites[MAX_SPRITES];

// Returns NULL if spriteId is out of range
__force_inline Sprite* hstx_dvi_sprite_get(const SpriteId spriteId) {
	return spriteId < MAX_SPRITES ? &_sprites[spriteId] : NULL;
}

__force_inline void hstx_dvi_sprite_disable_1(Sprite* sprite) {
	sprite->f &= ~SF_ENABLE;
}
__force_inline void hstx_dvi_sprite_disable(const SpriteId spriteId) {
	if (spriteId < MAX_SPRITES) {
		hstx_dvi_sprite_disable_1(&_sprites[spriteId]);
	}
}

void hstx_dvi_sprite_init_all();

__force_inline void init_sprite(
	const int i,
	const int32_t x,
	const int32_t y,
	const uint32_t w,
	const uint32_t h,
	const uint32_t f,
	void * const d1, 
	void * const d2,
	SpriteRenderer r
) {
	if ((uint32_t)i >= MAX_SPRITES) return;
	Sprite *s = &_sprites[i];
	s->x = x;
	s->y = y;
	s->w = w;
	s->h = h;
	s->f = f;
	s->d1 = d1;
	s->d2 = d2;
	s->r = r;
}

// ----------------------------------------------------------------------------
// Sprite collisions
// 
// Collide with the drawn sprite only 
// ----------------------------------------------------------------------------
#define SPRITE_ID_ROW_WORDS ((MODE_H_ACTIVE_PIXELS + 3) >> 2)

typedef union {
	SpriteId id[MODE_H_ACTIVE_PIXELS];
	uint32_t word[SPRITE_ID_ROW_WORDS];
} SpriteIdRow;

typedef union {
	SpriteCollisionMask m[MAX_SPRITES];
} SpriteCollisions;

extern SpriteCollisions _spriteCollisions;

void clear_sprite_id_row();

void sprite_renderer_sprite_8x8_p1(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
);

void sprite_renderer_sprite_16x8_p1(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
);

void sprite_renderer_sprite_16x16_p1(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
);

void sprite_renderer_sprite_32x16_p1(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
);

// 32 pixels wide and as tall as the sprite, with d1 pointing at a uint32_t
// for each row, bit 31 leftmost
void sprite_renderer_sprite_32xn_p1(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
);

// For renderers of your own: fills the pixels from x0 up to but not including x1
// in one colour, clipped to the row, taking part in sprite priority and collisions
// as the sprite spriteId
void sprite_render_span(
	hstx_dvi_row_t* r,
	int32_t x0,
	int32_t x1,
	const hstx_dvi_pixel_t p,
	const SpriteId spriteId
);

// A run of pixels in one colour, for sprite_render_spans
typedef struct {
	int16_t x0, x1; // x1 is not included
	hstx_dvi_pixel_t p;
} SpriteSpan_t;

// As sprite_render_span for n spans, in order, so the earlier ones keep
// priority over the later ones. A sprite that draws many small runs on a row
// can draw them all with one call.
void sprite_render_spans(
	hstx_dvi_row_t* r,
	const SpriteSpan_t* spans,
	uint32_t n,
	const SpriteId spriteId
);

// ----------------------------------------------------------------------------
// Parts: a sprite made of lines and convex shapes, such as a ship in 3D
//
// The parts are drawn in order, so the earlier ones keep priority over the
// later ones, as a pixel can only be drawn once. Each part works out where it
// is on a row from the row alone, so nothing has to be kept between rows.
// The x of the lines and shapes is fixed point, with SPRITE_PARTS_FX bits
// after the point, and absolute on the screen.

#define SPRITE_PARTS_FX 16

// A band of a shape, filling the pixels whose middles are between its sides.
// The sides are half a pixel to the left, for the middle of the band's first row.
typedef struct {
	int32_t xl, dxl;
	int32_t xr, dxr;
	int32_t start; // the band's first row, from the top of the shape
} SpriteBand_t;

typedef struct {
	int16_t top, rows; // the rows of the screen it is on
	hstx_dvi_pixel_t colour;
	uint8_t is_shape;  // a shape rather than a line
	union {
		// A line's run on a row covers it from half a row above the middle of
		// the row to half a row below, so shallow lines join up, and stops at its ends
		struct {
			int32_t x0;   // where it starts, on its first row
			int32_t x1;   // where it ends, on its last row
			int32_t dxdy; // how far x moves for each row
			int32_t half; // half of dxdy
		} line;
		struct {
			SpriteBand_t* band;
			uint16_t count;
			uint16_t cur; // the band drawn last; the renderer keeps this up to date
		} shape;
	};
} SpritePart_t;

typedef struct {
	SpritePart_t* part;
	uint32_t count;
	int32_t top; // the y of the sprite
} SpriteParts_t;

// d1 points at a SpriteParts_t, and d2 is not used
void sprite_renderer_parts(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
);

void text_renderer_8x8_p1(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
);

void render_Tile16x16p2(
	Tile16x16p2_t *t,
	const hstx_dvi_pixel_t* p2,
	hstx_dvi_row_t* r,
	int32_t x,
	int32_t row
);

void hstx_dvi_sprite_render_frame();

// Hands the current sprite state to the renderer and blocks until it has been
// taken at the end of a frame. Sprite updates only become visible (and
// _spriteCollisions only changes) when this is called; between calls the
// sprites and collision masks can be changed freely.
void hstx_dvi_sprite_wait_for_frame();

void hstx_dvi_sprite_set_sprite_collision_mask(
	const SpriteId spriteId,
	const SpriteCollisionMask mask
);

void hstx_dvi_sprite_render_loop();

// With HSTX_DVI_SPRITE_TIMING defined, how hard the renderer worked on the
// last frame it drew. The times are in system clock cycles, and are only
// measured on Arm, being left at 0 otherwise.
typedef struct {
	uint32_t line_max;   // the slowest row
	uint32_t window_max; // the slowest run of rows that fits in the row FIFO
	uint32_t frame;      // all the rows
	// The least time the rows were ready ahead of the display, from the row
	// FIFO being full at the top of the frame. Below 0 a row was late and the
	// frame was dropped.
	int32_t ahead_min;
	uint32_t calls_max;  // the most sprite renderer calls in a run of rows that fits in the row FIFO
	uint32_t pixels_max; // the most pixels filled by sprite_render_span in such a run
} HstxDviSpriteTiming_t;

extern volatile HstxDviSpriteTiming_t hstx_dvi_sprite_timing;

__force_inline void hstx_dvi_sprite_disable_all() {
	for (uint32_t i = 0; i < MAX_SPRITES; ++i) {
		hstx_dvi_sprite_disable(i);
	}
}

#ifdef __cplusplus
} 
#endif
