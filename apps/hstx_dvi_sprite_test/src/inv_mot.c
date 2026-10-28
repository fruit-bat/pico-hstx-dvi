#include "inv_mot.h"
#include "inv_pallet.h"
#include "inv_score.h"
#include "inv_collisions.h"
#include "inv_invaders.h"
#include "inv_bullets.h"
#include "inv_gun.h"
#include "inv_level.h"
#include "inv_util.h"
#include "font_inv.h"
#include <stdio.h>

#define INV_MOT_Y 24
// The mystery ship only comes out while there are this many invaders left
#define INV_MOT_MIN_INVADERS 8
// Frames to show the score for after the ship is hit
#define INV_MOT_SCORE_FRAMES 60
#define INV_MOT_SCORE_TABLE_LEN 15

typedef enum {
	MOT_STATE_IDLE = 0,
	MOT_STATE_FLY,
	MOT_STATE_SCORE
} InvMotState_e;

static SpriteId _sprite_index = 0;
static int32_t _mot_v = 1;
static InvMotState_e _mot_state = MOT_STATE_IDLE;
static uint32_t _mot_timer = 0;

static uint8_t _score_text[4];
static TextGrid8_t _score_grid = {
	0,
	_score_text,
	(uint8_t*)font_8x8
};

// The score is not as mysterious as it looks. Like the arcade original it
// goes by the number of shots fired, so the 23rd shot, and every 15th after,
// is worth the most.
static const uint16_t _mot_score[INV_MOT_SCORE_TABLE_LEN] = {
	200, 100, 100, 200, 300, 200, 200, 100, 600, 200, 200, 200, 100, 300, 200
};

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
		0,  // x
		INV_MOT_Y, // y
		16, // width
		8,  // height
		0,  // flags
		&tile16x8p2_mot[0],
		inv_pallet_red(),
		sprite_renderer_sprite_16x8_p1);

	hstx_dvi_sprite_set_sprite_collision_mask(si, INV_MOT_COLLISION_MASK);

	// The score shown where the ship was hit
	init_sprite(
		si + 1,
		0,  // x
		INV_MOT_Y, // y
		32, // width
		8,  // height
		0,  // flags
		&_score_grid,
		inv_pallet_red(),
		text_renderer_8x8_p1);

	_mot_v = 1;
	_mot_state = MOT_STATE_IDLE;
	_mot_timer = inv_level_mot_interval();
    return _sprite_index + 2;
}

static void mot_show_score(const Sprite *sprite, const uint32_t score) {
	Sprite *text = hstx_dvi_sprite_get(_sprite_index + 1);
	_score_grid.w = snprintf((char*)_score_text, sizeof(_score_text), "%lu", (unsigned long)score);
	int32_t x = sprite->x - 4;
	if (x < 0) x = 0;
	if (x > MODE_H_ACTIVE_PIXELS - 32) x = MODE_H_ACTIVE_PIXELS - 32;
	text->x = x;
	text->w = _score_grid.w << 3;
	text->f |= SF_ENABLE;
}

void __not_in_flash_func(inv_mot_update)() {
	SpriteId si = _sprite_index;
	Sprite *sprite = hstx_dvi_sprite_get(si);
	switch (_mot_state) {
		case MOT_STATE_IDLE: {
			if (!inv_gun_is_alive() || inv_invaders_alive() < INV_MOT_MIN_INVADERS) break;
			if (_mot_timer) {
				--_mot_timer;
				break;
			}
			// Launch from one side or the other
			_mot_v = (inv_rand() & 1) ? 1 : -1;
			if (inv_level_get() >= 4) _mot_v <<= 1;
			sprite->x = _mot_v > 0 ? -16 : MODE_H_ACTIVE_PIXELS;
			sprite->f |= SF_ENABLE;
			_mot_state = MOT_STATE_FLY;
			break;
		}
		case MOT_STATE_FLY: {
			if (_spriteCollisions.m[si] & INV_BULLET_COLLISION_MASK) {
				const uint32_t score = _mot_score[inv_bullets_get_shots() % INV_MOT_SCORE_TABLE_LEN];
				inv_score_add(score);
				hstx_dvi_sprite_disable_1(sprite);
				mot_show_score(sprite, score);
				_mot_state = MOT_STATE_SCORE;
				_mot_timer = INV_MOT_SCORE_FRAMES;
			}
			else {
				sprite->x += _mot_v;
				if (sprite->x < -16 || sprite->x > MODE_H_ACTIVE_PIXELS) {
					hstx_dvi_sprite_disable_1(sprite);
					_mot_state = MOT_STATE_IDLE;
					_mot_timer = inv_level_mot_interval();
				}
			}
			break;
		}
		case MOT_STATE_SCORE: {
			if (--_mot_timer == 0) {
				hstx_dvi_sprite_disable(si + 1);
				_mot_state = MOT_STATE_IDLE;
				_mot_timer = inv_level_mot_interval();
			}
			break;
		}
		default: {
			break;
		}
	}
}
