#include "inv_mot.h"
#include "inv_pallet.h"
#include "inv_score.h"

#define INV_MOT_COLLISION_MASK ((SpriteCollisionMask)4)
#define INV_MOT_MAX_X 1000
#define INV_MOT_MIN_X -1000

static SpriteId _sprite_index = 0;
static int32_t _mot_v = 1;

static Tile16x8p2_t tile16x8p2_mot[] = {
	{{
		0b0000000000000000,
		0b0000001111000000,
		0b0001111111111000,
		0b0011111111111100,
		0b0110110110110110,
		0b1111111111111111,
		0b0011100110011100,
		0b0001000000001000,
	}}
};

SpriteId inv_mot_init(SpriteId start) {
    _sprite_index = start;

	SpriteId si = _sprite_index;

	init_sprite(
		si, 
		INV_MOT_MIN_X,  // x
		16, // y
		16, // width
		8,  // height
		SF_ENABLE,  // flags
		&tile16x8p2_mot[0], 
		inv_pallet_green(), 
		sprite_renderer_sprite_16x8_p1);

	hstx_dvi_sprite_set_sprite_collision_mask(si, INV_MOT_COLLISION_MASK);
	_mot_v = 1;
    return _sprite_index + 1;
}

void inv_mot_update() {
	SpriteId si = _sprite_index;
	Sprite *sprite = hstx_dvi_sprite_get(si);
	if (sprite->f & SF_ENABLE) {
		if (_spriteCollisions.m[si]) {
//			hstx_dvi_sprite_disable_1(sprite); // Disable the bullet if it was previously enabled
			_spriteCollisions.m[si] = 0;
            inv_score_add(1000);
			sprite->x = INV_MOT_MIN_X; // Reset position
		}
		else {
			sprite->x += _mot_v; 
			if (sprite->x > INV_MOT_MAX_X) { 
				sprite->x = INV_MOT_MAX_X;
				_mot_v = -_mot_v; // Reverse direction
			}
			else if (sprite->x < INV_MOT_MIN_X) { 
				sprite->x = INV_MOT_MIN_X;
				_mot_v = -_mot_v; // Reverse direction
			}
		}
	}
}

