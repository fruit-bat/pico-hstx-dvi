#include "inv_mot.h"
#include "inv_pallet.h"
#include "inv_score.h"
#include "inv_input.h"
#include "inv_bullets.h"
#include "inv_collisions.h"

static SpriteId _sprite_index = 0;

static Tile16x8p2_t tile16x8p2_gun[] = {
	{{
		0b0000000010000000,
		0b0000000111000000,
		0b0000000111000000,
		0b0000111111111000,
		0b0011111111111110,
		0b0011111111111110,
		0b0011111111111110,
		0b0011111111111110,
	}}
};

SpriteId inv_gun_init(SpriteId start) {
    _sprite_index = start;

	SpriteId si = _sprite_index;

	init_sprite(
		si, 
		20,  // x
		MODE_V_ACTIVE_LINES - 64, // y
		16, // width
		8,  // height
		SF_ENABLE,  // flags
		&tile16x8p2_gun[0], 
		inv_pallet_green(), 
		sprite_renderer_sprite_16x8_p1);

	hstx_dvi_sprite_set_sprite_collision_mask(si, INV_GUN_COLLISION_MASK);

    return _sprite_index + 1;
}

void __not_in_flash_func(inv_gun_update)() {
	SpriteId si = _sprite_index;
	Sprite *sprite = hstx_dvi_sprite_get(si);
	if (sprite->f & SF_ENABLE) {
		if (_spriteCollisions.m[si]) {
//			hstx_dvi_sprite_disable_1(sprite); // Disable the bullet if it was previously enabled
			_spriteCollisions.m[si] = 0;
            inv_score_clear();
			sprite->x = 0; // Reset position
		}
		else {
            // read the input
            const uint8_t input = get_inv_input();

            if (is_inv_input_left(input)) {
                sprite->x -= 2;
                if (sprite->x < 0) sprite->x = 0;
            }
            if (is_inv_input_right(input)) {
                sprite->x += 2;
                if (sprite->x + 16 > MODE_H_ACTIVE_PIXELS)
                    sprite->x = MODE_H_ACTIVE_PIXELS - 16;
            }
            if (is_inv_input_fire(input)) {
                // Fire a bullet
                inv_bullets_fire(si);
            }
		}
	}
}

