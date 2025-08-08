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
	for(int32_t i = 0; i < (MODE_H_ACTIVE_PIXELS>>2); i++) {
        hstx_dvi_row_set_pixel_quad(
            r, 
            i, 
            p,p,p,p);
	}
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


