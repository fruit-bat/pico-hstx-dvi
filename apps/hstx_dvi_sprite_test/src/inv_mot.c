#include "inv_mot.h"
#include "game_pallet.h"
#include "inv_score.h"
#include "inv_collisions.h"
#include "inv_invaders.h"
#include "inv_bullets.h"
#include "inv_gun.h"
#include "inv_level.h"
#include "game_util.h"
#include "game_art.h"
#include "game_fx.h"

#define INV_MOT_Y 24
// The mystery ship only comes out while there are this many invaders left
#define INV_MOT_MIN_INVADERS 8
// Frames to show the score for after the ship is hit
#define INV_MOT_SCORE_FRAMES 60
#define INV_MOT_SCORE_TABLE_LEN 15

#define INV_MOT_W 32
#define INV_MOT_LAYERS 3
#define INV_MOT_FRAMES 4

typedef enum {
	MOT_STATE_IDLE = 0,
	MOT_STATE_FLY,
	MOT_STATE_SCORE
} InvMotState_e;

static SpriteId _sprite_index = 0;
static int32_t _mot_v = 1;
static int32_t _mot_x = 0;
static InvMotState_e _mot_state = MOT_STATE_IDLE;
static uint32_t _mot_timer = 0;

// The score is not as mysterious as it looks. Like the arcade original it
// goes by the number of shots fired, so the 23rd shot, and every 15th after,
// is worth the most.
static const uint16_t _mot_score[INV_MOT_SCORE_TABLE_LEN] = {
	200, 100, 100, 200, 300, 200, 200, 100, 600, 200, 200, 200, 100, 300, 200
};

static const char* const _mot_top[] = {
	"............RRRRRRRR............",
	".........RRRRRRRRRRRRRR.........",
	"......RRRRRRRRRRRRRRRRRRRR......",
};

static const char* const _mot_bottom[] = {
	"RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR",
	"..RRRRRRRRRRRRRRRRRRRRRRRRRRRR..",
	".....RRRRRRRRRRRRRRRRRRRRRR.....",
};

static const uint8_t _mot_colours[INV_MOT_LAYERS] = { GAME_RED, GAME_WHITE, GAME_YELLOW };

// The windows light up in turn and the lights underneath wink
static GameArt_t _art_mot[INV_MOT_FRAMES];
static bool _art_made = false;

static void make_art(void) {
	GameArtCanvas_t c;
	for (uint32_t f = 0; f < INV_MOT_FRAMES; ++f) {
		game_art_canvas_clear(&c, INV_MOT_W, 8);
		game_art_canvas_stamp(&c, _mot_top, GAME_ART_ROWS(_mot_top), 0, 0);
		static const char* const band[] = { "...RRRRRRRRRRRRRRRRRRRRRRRRRR..." };
		game_art_canvas_stamp(&c, band, 1, 0, 3);
		for (uint32_t k = 0; k < 6; ++k) {
			const char w = (k & 3) == f ? 'Y' : 'W';
			game_art_canvas_set(&c, 6 + k * 4, 3, w);
			game_art_canvas_set(&c, 7 + k * 4, 3, w);
		}
		game_art_canvas_stamp(&c, _mot_bottom, GAME_ART_ROWS(_mot_bottom), 0, 4);
		for (uint32_t x = 6 + (f & 1) * 2; x < 26; x += 4) game_art_canvas_set(&c, x, 7, 'Y');
		game_art_canvas_make(&_art_mot[f], &c, "RWY", _mot_colours);
	}
	_art_made = true;
}

void inv_mot_draw_icon(int32_t x, int32_t y, uint32_t frame) {
	if (!_art_made) make_art();
	game_art_draw(&_art_mot[(frame >> 3) % INV_MOT_FRAMES], x, y, NULL);
}

SpriteId inv_mot_init(SpriteId start) {
	if (!_art_made) make_art();
	_sprite_index = start;
	for (uint32_t l = 0; l < INV_MOT_LAYERS; ++l) {
		init_sprite(start + l, 0, INV_MOT_Y, 0, 0, 0, NULL, NULL, NULL);
		hstx_dvi_sprite_set_sprite_collision_mask(start + l, INV_MOT_COLLISION_MASK);
	}

	_mot_v = 1;
	_mot_state = MOT_STATE_IDLE;
	_mot_timer = inv_level_mot_interval();
	return start + INV_MOT_LAYERS;
}

void __not_in_flash_func(inv_mot_update)(uint32_t frame) {
	SpriteId si = _sprite_index;
	switch (_mot_state) {
		case MOT_STATE_IDLE: {
			if (!inv_gun_is_alive() || inv_invaders_alive() < INV_MOT_MIN_INVADERS) break;
			if (_mot_timer) {
				--_mot_timer;
				break;
			}
			// Launch from one side or the other
			_mot_v = (game_rand() & 1) ? 1 : -1;
			if (inv_level_get() >= 4) _mot_v <<= 1;
			_mot_x = _mot_v > 0 ? -INV_MOT_W : MODE_H_ACTIVE_PIXELS;
			_mot_state = MOT_STATE_FLY;
			break;
		}
		case MOT_STATE_FLY: {
			SpriteCollisionMask m = 0;
			for (uint32_t l = 0; l < INV_MOT_LAYERS; ++l) m |= _spriteCollisions.m[si + l];
			if (m & INV_BULLET_COLLISION_MASK) {
				const uint32_t score = _mot_score[inv_bullets_get_shots() % INV_MOT_SCORE_TABLE_LEN];
				inv_score_add(score);
				game_art_sprites_hide(si, INV_MOT_LAYERS);
				const int32_t cx = GAME_PX(_mot_x + (INV_MOT_W >> 1));
				game_fx_explode(cx, GAME_PX(INV_MOT_Y + 4), GAME_RED, 24, GAME_PX(3));
				game_fx_explode(cx, GAME_PX(INV_MOT_Y + 4), GAME_YELLOW, 12, GAME_PX(1));
				game_fx_popup(cx - GAME_PX(12), INV_MOT_Y, score);
				_mot_state = MOT_STATE_SCORE;
				_mot_timer = INV_MOT_SCORE_FRAMES;
				break;
			}
			_mot_x += _mot_v;
			if (_mot_x < -INV_MOT_W || _mot_x > MODE_H_ACTIVE_PIXELS) {
				game_art_sprites_hide(si, INV_MOT_LAYERS);
				_mot_state = MOT_STATE_IDLE;
				_mot_timer = inv_level_mot_interval();
			}
			else {
				game_art_sprites(si, INV_MOT_LAYERS, &_art_mot[(frame >> 3) % INV_MOT_FRAMES], _mot_x, INV_MOT_Y, NULL);
			}
			break;
		}
		case MOT_STATE_SCORE: {
			if (--_mot_timer == 0) {
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
