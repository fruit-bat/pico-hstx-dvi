#include "inv_bullets.h"
#include "inv_pallet.h"

#define INV_BULLET_COUNT 1
#define INV_BULLET_COLLISION_MASK ((SpriteCollisionMask)2)

static SpriteId _sprite_index = 0;

static Tile8x8p2_t tile8x8p2_bullets[] = {
	{{
		0b10000000,
		0b10000000,
		0b10000000,
		0b10000000,
		0b10000000,
		0b10000000,
		0b10000000,
		0b10000000,
	}}
};

SpriteId inv_bullets_init(SpriteId start) {
    _sprite_index = start;

	for(uint32_t x = 0; x < INV_BULLET_COUNT; ++x) {

	    SpriteId si = _sprite_index + x;

		init_sprite(
			si, 
			0,  // x
			0,  // y
			8,  // width
			8,  // height
			0,  // flags
			&tile8x8p2_bullets[0], 
			inv_pallet_red(), 
			sprite_renderer_sprite_8x8_p1);

		hstx_dvi_sprite_set_sprite_collision_mask(si, INV_BULLET_COLLISION_MASK);
	}

    return _sprite_index + INV_BULLET_COUNT;
}

void inv_bullets_update() {
    for (uint32_t i = 0; i < INV_BULLET_COUNT; ++i)
    {
        SpriteId si = _sprite_index + i;
        Sprite *sprite = hstx_dvi_sprite_get(si);
        if (sprite->f & SF_ENABLE) {
			if (_spriteCollisions.m[si] & ~INV_BULLET_COLLISION_MASK) {
				hstx_dvi_sprite_disable_1(sprite); // Disable the bullet if it was previously enabled
				_spriteCollisions.m[si] = 0;
			}
			else {
				sprite->y -= 3; // Move the bullet up
				if (sprite->y < -8) { // If the bullet goes off screen
					sprite->f &= ~SF_ENABLE; // Disable the bullet
				}
			}
		}
    }
}

void inv_bullets_fire(SpriteId gunSpriteId) {
	for (uint32_t i = 0; i < INV_BULLET_COUNT; ++i)
	{
		SpriteId si = _sprite_index + i;
		Sprite *sprite = hstx_dvi_sprite_get(si);
		if (!(sprite->f & SF_ENABLE)) { // If the bullet is not enabled
			sprite->x = 8 + hstx_dvi_sprite_get(gunSpriteId)->x; // Set bullet x to gun x
			sprite->y = hstx_dvi_sprite_get(gunSpriteId)->y - 8; // Set bullet y above the gun
			sprite->f |= SF_ENABLE; // Enable the bullet
			break; // Exit after firing one bullet
		}
	}
}
