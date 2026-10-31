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

__force_inline void hstx_dvi_sprite_disable_all() {
	for (uint32_t i = 0; i < MAX_SPRITES; ++i) {
		hstx_dvi_sprite_disable(i);
	}
}

#ifdef __cplusplus
} 
#endif
