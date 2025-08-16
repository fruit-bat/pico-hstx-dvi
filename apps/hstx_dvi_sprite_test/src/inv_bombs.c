#include "inv_bombs.h"
#include "inv_pallet.h"
#include "inv_collisions.h"
#include "inv_base.h"

#define INV_BOMB_COUNT 8

static SpriteId _sprite_index = 0;

static Tile8x8p2_t tile8x8p2_bombs[] = {
	{{
		0b00010000,
		0b00100000,
		0b00010000,
		0b00001000,
		0b00010000,
		0b00100000,
		0b00010000,
		0b00010000,
	}},
	{{
		0b00010000,
		0b00001000,
		0b00010000,
		0b00100000,
		0b00010000,
		0b00001000,
		0b00010000,
		0b00010000,
	}},	
};

SpriteId inv_bombs_init(SpriteId start) {
    _sprite_index = start;

	for(uint32_t x = 0; x < INV_BOMB_COUNT; ++x) {

	    SpriteId si = _sprite_index + x;

		init_sprite(
			si, 
			0,  // x
			0,  // y
			8,  // width
			8,  // height
			0,  // flags
			&tile8x8p2_bombs[0], 
			inv_pallet_yellow(), 
			sprite_renderer_sprite_8x8_p1);

		hstx_dvi_sprite_set_sprite_collision_mask(si, INV_BOMB_COLLISION_MASK);
	}

    return _sprite_index + INV_BOMB_COUNT;
}

void inv_bombs_update() {
    for (uint32_t i = 0; i < INV_BOMB_COUNT; ++i)
    {
        SpriteId si = _sprite_index + i;
        Sprite *sprite = hstx_dvi_sprite_get(si);
        if (sprite->f & SF_ENABLE) {
			SpriteCollisionMask m = _spriteCollisions.m[si];
			if (m & ~INV_BOMB_COLLISION_MASK) {
				inv_base_bomb_hit(si, m); // Notify the base of the bomb hit
				hstx_dvi_sprite_disable_1(sprite); // Disable the bullet if it was previously enabled
				_spriteCollisions.m[si] = 0;
			}
			else {
				sprite->y += 4; // Move the bullet down
				if (sprite->y > MODE_V_ACTIVE_LINES + 8) { // If the bullet goes off screen
					sprite->f &= ~SF_ENABLE; // Disable the bullet
				}
			}
		}
    }
}

void inv_bombs_fire(SpriteId invSpriteId) {
	for (uint32_t i = 0; i < INV_BOMB_COUNT; ++i)
	{
		SpriteId si = _sprite_index + i;
		Sprite *sprite = hstx_dvi_sprite_get(si);
		if (!(sprite->f & SF_ENABLE)) { // If the bullet is not enabled
			sprite->x = 4 + hstx_dvi_sprite_get(invSpriteId)->x; // Set bullet x to gun x
			sprite->y = hstx_dvi_sprite_get(invSpriteId)->y + 8; // Set bullet y above the gun
			sprite->f |= SF_ENABLE; // Enable the bullet
			break; // Exit after firing one bullet
		}
	}
}
