#include "inv_base.h"
#include "inv_pallet.h"
#include <memory.h>
#include "inv_collisions.h"

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

void inv_base_update() {
}

void inv_base_bomb_hit(SpriteId spriteId) {

}

void inv_base_bullet_hit(SpriteId spriteId) {

}
