#include "hstx_dvi_sprite.h"

Sprite _sprites[MAX_SPRITES];

static SpriteCollisionMask _spriteCollisionMasks[MAX_SPRITES];
static SpriteIdRow _spriteIdRow; 
static SpriteCollisions _spriteCollisions;

void __not_in_flash_func(init_sprites)() {
	for(uint32_t i = 0; i < MAX_SPRITES; ++i) _sprites[i].f = 0;
}

void __not_in_flash_func(clear_sprite_id_row)() {
	for(uint32_t i = 0; i < SPRITE_ID_ROW_WORDS; ++i) _spriteIdRow.word[i] = 0;
}

void __not_in_flash_func(clear_sprite_collisions)() {
	for(uint32_t i = 0; i < MAX_SPRITES >> 2; ++i) _spriteCollisions.word[i] = 0;
}

void __not_in_flash_func(render_row_mono)(
	hstx_dvi_row_t* r,
	hstx_dvi_pixel_t p
) {
    hstx_dvi_fill_row(r, p);
}

static inline void render_sprite_pixel(
	hstx_dvi_row_t* r,
    hstx_dvi_pixel_t p,
	const SpriteId spriteId,
	const uint32_t j)
{
	const SpriteId ncid = _spriteIdRow.id[j];
	if (ncid)
	{
		const SpriteId cid = ncid - 1;
		_spriteCollisions.m[cid] |= _spriteCollisionMasks[spriteId];
		_spriteCollisions.m[spriteId] |= _spriteCollisionMasks[cid];
	}
	else
	{
        hstx_dvi_row_set_pixel(r, j, p);
		_spriteIdRow.id[j] = spriteId + 1;
	}
}

static inline void render_pixel(
	hstx_dvi_row_t* r,
    hstx_dvi_pixel_t p,
    const uint32_t j
) {
    hstx_dvi_row_set_pixel(r, j, p);
}

static inline void render_sprite_row_n_p1(
	uint32_t d,
	const Pallet1_t p1,
	hstx_dvi_row_t* r,
	const int32_t x,
	const SpriteId spriteId,
	const uint32_t w
) {
	if (d)
	{
		const uint32_t bm = 1 << (w-1);
        const hstx_dvi_pixel_t p = p1[0];
		if (((uint32_t)x) < (MODE_H_ACTIVE_PIXELS - w))
		{
			for (int32_t i = 0; i < w; i++)
			{
				const uint32_t j = (uint32_t)x + i;
				if (d & bm)
				{
					render_sprite_pixel(r, p, spriteId, j);
				}
				d <<= 1;
			}
		}
		else
		{
			for (int32_t i = 0; i < w; i++)
			{
				const uint32_t j = (uint32_t)x + i;
				if ((j < MODE_H_ACTIVE_PIXELS) && (d & bm))
				{
					render_sprite_pixel(r, p, spriteId, j);
				}
				d <<= 1;
			}
		}
	}
}

static inline void render_row_n_p1(
	uint32_t d,
	const Pallet1_t p1,
	hstx_dvi_row_t* r,
	const int32_t x,
	const uint32_t w
) {
	if (d)
	{
        const hstx_dvi_pixel_t p = p1[0];
		const uint32_t bm = 1 << (w-1);
		if (((uint32_t)x) < (MODE_H_ACTIVE_PIXELS - w))
		{
			for (int32_t i = 0; i < w; i++)
			{
				const uint32_t j = (uint32_t)x + i;
				if (d & bm)
				{
					render_pixel(r, p, j);
				}
				d <<= 1;
			}
		}
		else
		{
			for (int32_t i = 0; i < w; i++)
			{
				const uint32_t j = (uint32_t)x + i;
				if ((j < MODE_H_ACTIVE_PIXELS) && (d & bm))
				{
					render_pixel(r, p, j);
				}
				d <<= 1;
			}
		}
	}
}

