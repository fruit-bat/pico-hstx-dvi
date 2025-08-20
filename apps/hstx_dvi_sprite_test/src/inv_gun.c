#include "inv_mot.h"
#include "inv_pallet.h"
#include "inv_score.h"
#include "inv_input.h"
#include "inv_bullets.h"
#include "inv_collisions.h"
#include "inv_gun.h"

#define INV_GUN_COOLDOWN 20

typedef enum  {
	GUN_STATE_ALIVE = 0,
	GUN_STATE_EXPLODE,
	GUN_STATE_WAIT,
	GUN_STATE_DEAD
 } InvGunState_e;

typedef struct {
	InvGunState_e state;
	uint32_t end;
} InvGunState_t;

static SpriteId _sprite_index = 0;
static uint32_t _cooldown = 0;
static uint32_t _lives = 3;
static InvGunState_t _gun_state;

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

static Tile16x8p2_t tile16x8p2_gun_explosion[] = {
	{{
		0b0000001000000000,
		0b0000000000010000,
		0b0000001010100000,
		0b0001001000000000,
		0b0000000110110000,
		0b0100010110101000,
		0b0001111111100100,
		0b0011111111110101,
	}},
	{{
		0b0000000000000100,
		0b1000001000011001,
		0b0001000011000000,
		0b0000001000000010,
		0b0100101100110001,
		0b0010000111000100,
		0b0001111111110000,
		0b0011011111110010,
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

	_cooldown = 0;
	_lives = 3;
	_gun_state.state = GUN_STATE_ALIVE;
	_gun_state.end = 0;

    return _sprite_index + 1;
}

static void inv_gun_explode(const uint32_t frame) {
	const SpriteId spriteId = _sprite_index;
	Sprite *sprite = hstx_dvi_sprite_get(spriteId);
	if (sprite->f & SF_ENABLE) {
		// Set the sprite to explode
		sprite->d1 = &tile16x8p2_gun_explosion[0];
		sprite->d2 = inv_pallet_yellow();
		_gun_state.state = GUN_STATE_EXPLODE;
		_gun_state.end = frame + 30; 
	}
}

void __not_in_flash_func(inv_gun_update)(uint32_t frame) {
	SpriteId si = _sprite_index;
	Sprite *sprite = hstx_dvi_sprite_get(si);
	switch(_gun_state.state) {
		case GUN_STATE_ALIVE: {
			if (sprite->f & SF_ENABLE) {
				if (_spriteCollisions.m[si]) {
					inv_gun_explode(frame);
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
					if (_cooldown) {
						--_cooldown;
					}
					else if (is_inv_input_fire(input)) {
						// Fire a bullet
						inv_bullets_fire(si);
						// Set the cooldown
						_cooldown = INV_GUN_COOLDOWN;
					}
				}
			}
			break;
		}
		case GUN_STATE_EXPLODE: {
			const int32_t r = (int32_t)(_gun_state.end - frame);
			if (r < 0) {
				hstx_dvi_sprite_disable_1(sprite);
				if (--_lives > 0) {
					_gun_state.state = GUN_STATE_WAIT;
					_gun_state.end = frame + 60; // Wait for 60 frames
				
				}
				else {
					// Gun is dead
					_gun_state.state = GUN_STATE_DEAD;
				}
			}
			else {
				sprite->d1 = &tile16x8p2_gun_explosion[(r >> 2) & 1];
			}
			break;
		}
		case GUN_STATE_WAIT: {
			const int32_t r = (int32_t)(_gun_state.end - frame);
			if (r < 0) {
				// Reset the gun
				sprite->x = 20;				
				_gun_state.state = GUN_STATE_ALIVE;
				sprite->f |= SF_ENABLE; // Enable the sprite
				sprite->d1 = &tile16x8p2_gun[0];
				sprite->d2 = inv_pallet_green();					
			}
			break;
		}
		case GUN_STATE_DEAD: {
			// Do nothing, the sprite is already disabled
			inv_score_clear(); // Clear the score when the gun is destroyed
			break;
		}
		default: {
			break;
		}
	}
}

uint32_t __not_in_flash_func(inv_gun_get_lives)(void) {
	return _lives;
}	