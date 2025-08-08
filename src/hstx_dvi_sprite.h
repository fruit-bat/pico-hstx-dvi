#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef hstx_dvi_pixel_t* Pallet1_t;

typedef hstx_dvi_pixel_t* Pallet2_t;

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
} TextGrid8_t;

typedef uint8_t SpriteId;
typedef uint8_t SpriteCollisionMask;

typedef void (*SpriteRenderer)(
	const void* d1,
	const void* d2,
	uint32_t * const dr,
	uint32_t * const dg,
	uint32_t * const db,
	const int32_t tdmsI,
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

#define MAX_SPRITES (1<<6)

extern Sprite _sprites[MAX_SPRITES];

void init_sprites();

void init_sprite(
	int i,
	int32_t x,
	int32_t y,
	uint32_t w,
	uint32_t h,
	uint32_t f,
	void *d1, 
	void *d2,
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


void clear_sprite_id_row();

void clear_sprite_collisions();

#ifdef __cplusplus
} 
#endif
