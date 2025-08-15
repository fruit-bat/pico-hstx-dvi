#include "inv_bullets.h"

#define INV_BULLET_COUNT 4
#define INV_BULLET_COLLISION_MASK ((SpriteCollisionMask)8)

static SpriteId _sprite_index = 0;

static const hstx_dvi_pixel_t  pallet1_Red[] = {
    HSTX_DVI_PIXEL_RGB(200, 0, 0), // Red
};

static Tile16x8p2_t tile16x8p2_bullets[] = {
	{{
		0b0000000010000000,
		0b0000000010000000,
		0b0000000010000000,
		0b0000000010000000,
		0b0000000010000000,
		0b0000000010000000,
		0b0000000010000000,
		0b0000000010000000,
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
			16, // width
			8,  // height
			0,  // flags
			&tile16x8p2_bullets[0], 
			(hstx_dvi_pixel_t*)pallet1_Red, 
			sprite_renderer_sprite_16x8_p1);

		hstx_dvi_sprite_set_sprite_collision_mask(si, INV_BULLET_COLLISION_MASK);
	}

    return _sprite_index + INV_BULLET_COUNT;
}

void inv_bullet_update() {
    for (uint32_t i = 0; i < INV_BULLET_COUNT; ++i)
    {
        SpriteId si = _sprite_index + i;
        Sprite *sprite = hstx_dvi_sprite_get(si);
        if (sprite->f & SF_ENABLE) {
			sprite->y -= 2; // Move the bullet up
			if (sprite->y < -8) { // If the bullet goes off screen
				sprite->f &= ~SF_ENABLE; // Disable the bullet
			}
		}
    }
}
