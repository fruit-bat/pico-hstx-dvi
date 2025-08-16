#include "inv_base.h"
#include "inv_pallet.h"
#include <memory.h>
#include "inv_collisions.h"
#include <stdio.h>
#define INV_BASE_COUNT 4

static SpriteId _sprite_index = 0;

static Tile32x16p2_t tile32x16p2_base = {
	{
	//   0123456789012345 
		0b00000000011111111111111000000000,
		0b00000000111111111111111100000000,
		0b00000001111111111111111110000000,
		0b00000011111111111111111111000000,
		0b00000111111111111111111111100000,
		0b00000111111111111111111111100000,
		0b00000111111111111111111111100000,
		0b00000111111111111111111111100000,
		0b00000111111111111111111111100000,
		0b00000111111111111111111111100000,
		0b00000111111111111111111111100000,
		0b00000111111111111111111111100000,
		0b00000111111100000000111111100000,
		0b00000111111000000000011111100000,
		0b00000111110000000000001111100000,
		0b00000111110000000000001111100000,
	}
};

static Tile32x16p2_t tile32x16p2_bases[INV_BASE_COUNT];

SpriteId inv_base_init(SpriteId start) {
    _sprite_index = start;

	for(uint i = 0; i < INV_BASE_COUNT; ++i){

		SpriteId si = _sprite_index + i;

		memcpy(&tile32x16p2_bases[i], &tile32x16p2_base, sizeof(Tile32x16p2_t));

		uint w1 = MODE_H_ACTIVE_PIXELS / INV_BASE_COUNT;
		uint s = w1 * i;
		uint p = s + ((w1 - 32) / 2);
		init_sprite(
			si, 
			p, 
			MODE_V_ACTIVE_LINES - 88, 
			32, 
			16, 
			SF_ENABLE, 
			&tile32x16p2_bases[i], 
			inv_pallet_green(), 
			sprite_renderer_sprite_32x16_p1);

			hstx_dvi_sprite_set_sprite_collision_mask(si, INV_BASE_COLLISION_MASK << i);
	}

    return _sprite_index + INV_BASE_COUNT;
}

void __not_in_flash_func(inv_base_update)() {
}

static Tile8x8p2_t tile8x8p2_damage[] = {
	{{
		0b01011100,
		0b00011010,
		0b00001100,
		0b00011000,
		0b00001100,
		0b00000000,
		0b00000000,
		0b00000000,
	}}
};

void __not_in_flash_func(inv_base_bomb_hit)(SpriteId spriteId, SpriteCollisionMask m) {
	SpriteCollisionMask t = INV_BASE_COLLISION_MASK;
	for(uint i = 0; i < INV_BASE_COUNT; ++i) {
		if (m & t) {
			SpriteId si = _sprite_index + i;
			uint32_t* base_image = (uint32_t*)(&tile32x16p2_bases[i]);
			Tile8x8p2_t *d = &tile8x8p2_damage[0];
			Sprite *bomb_sprite = hstx_dvi_sprite_get(spriteId);
			int32_t bomb_x = bomb_sprite->x;
			int32_t bomb_y = bomb_sprite->y;
			int32_t base_x = hstx_dvi_sprite_get(si)->x;
			int32_t base_y = hstx_dvi_sprite_get(si)->y;
			int32_t dx = bomb_x - base_x;
			int32_t dy = (bomb_y - base_y); // Adjust for the bombs height
			for (uint32_t r = 0; r < 8; ++r)
			{
				uint32_t d1 = d->d[r];
				int32_t y = dy + r;
				if ((y < 0) || (y >= 16)) continue; // Skip
				base_image[y] &= ~(d1 << (24 -dx));
			}
		}
		t <<= 1;
	}
}

void __not_in_flash_func(inv_base_bullet_hit)(SpriteId spriteId) {
}

