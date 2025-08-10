#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint16_t d[16];
} Tile16x16p2_t;

typedef struct {
	uint16_t d[8];
} Tile16x8p2_t;

typedef struct {
	uint32_t d[16];
} Tile32x16p2_t;

typedef struct {
	uint16_t w;
	uint8_t *s;
	uint8_t *font;
} TextGrid8_t;

typedef uint8_t SpriteId;
typedef uint8_t SpriteCollisionMask;

typedef void (*SpriteRenderer)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t y,
	const int32_t row,
	const SpriteId spriteId
);

enum SpriteFlags {
  SF_ENABLE = 1
};

typedef struct Sprite {
	int32_t x,y;
	uint32_t w,h,f;
	void *d1, *d2;
	SpriteRenderer r;
} Sprite;

#define MAX_SPRITES (1<<8)

extern Sprite _sprites[MAX_SPRITES];

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
	uint32_t word[MAX_SPRITES >> 2];
} SpriteCollisions;

// TODO probably should have accessor functions for these
extern SpriteCollisions _spriteCollisions;

void clear_sprite_id_row();

void clear_sprite_collisions();

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

void hstx_dvi_sprite_render_frame(uint32_t frame_index);

void hstx_dvi_sprite_set_sprite_collision_mask(
	const SpriteId spriteId,
	const SpriteCollisionMask mask
);

#ifdef __cplusplus
} 
#endif
