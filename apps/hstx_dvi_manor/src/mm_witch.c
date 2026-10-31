#include "mm_witch.h"
#include "mm_input.h"
#include "mm_map.h"
#include "mm_thing.h"
#include "mm_camera.h"
#include "mm_art.h"
#include "mm_hud.h"
#include "game_fx.h"
#include "game_pallet.h"
#include "game_util.h"

#define MM_RUN_SPEED GAME_PX(3)
#define MM_PUSH_SPEED GAME_PX(1)
#define MM_GROUND_ACCEL (GAME_PX(1) / 2)
#define MM_AIR_ACCEL (GAME_PX(1) / 2)
// Up about 80 pixels, two and a half tiles
#define MM_JUMP_SPEED 1950
// Letting go of jump early cuts it short
#define MM_JUMP_CUT GAME_PX(2)
#define MM_FLOAT_FALL GAME_PX(1)
// Jumping still works for a few frames after walking off an edge, and a press
// is remembered for a few frames before landing
#define MM_COYOTE_FRAMES 6
#define MM_BUFFER_FRAMES 6
#define MM_DROP_FRAMES 12
#define MM_HURT_FRAMES 90
#define MM_STUN_FRAMES 16
#define MM_DIZZY_FRAMES 150
#define MM_CAST_FRAMES 12
#define MM_BOLT_SPEED GAME_PX(7)
#define MM_BOLT_FRAMES 96
#define MM_BOLT_R 6
// Where the wand is, from the middle bottom
#define MM_WAND_X 28
#define MM_WAND_Y 32

static MmBody_t _b;
static int32_t _facing;
static uint32_t _walk;
static uint32_t _hearts;
static uint32_t _keys;
static bool _float;
static uint32_t _hurt;
static uint32_t _stun;
static uint32_t _dizzy;
static bool _fainted;
static uint32_t _coyote;
static uint32_t _buffer;
static uint32_t _drop;
static uint32_t _cast;
static bool _cheer;
static bool _floating;
static int32_t _check_cx, _check_cy;
static uint32_t _locked_said;
static uint8_t _last_input;
static bool _bolt;
static int32_t _bx, _by, _bvx, _bvy;
static uint32_t _blife;

void mm_witch_init(void) {
	_b.hw = MM_WITCH_HW;
	_b.h = MM_WITCH_H;
}

static void place(int32_t cx, int32_t cy) {
	_b.x = GAME_PX(cx * MM_TILE + MM_TILE / 2);
	_b.y = GAME_PX((cy + 1) * MM_TILE);
	_b.vx = 0;
	_b.vy = 0;
	_b.ground = true;
	_bolt = false;
	_cast = 0;
	_coyote = _buffer = _drop = 0;
	_stun = 0;
	_dizzy = 0;
	_fainted = false;
	_cheer = false;
	_hearts = MM_WITCH_HEARTS;
}

void mm_witch_reset(int32_t cx, int32_t cy) {
	place(cx, cy);
	_check_cx = cx;
	_check_cy = cy;
	_facing = 1;
	_keys = 0;
	_float = false;
	_hurt = 0;
	_last_input = 0xff;
}

void mm_witch_respawn(void) {
	place(_check_cx, _check_cy);
	_hurt = MM_HURT_FRAMES;
	_last_input = 0xff;
}

int32_t mm_witch_x(void) {
	return _b.x >> GAME_FP;
}

int32_t mm_witch_y(void) {
	return _b.y >> GAME_FP;
}

int32_t mm_witch_facing(void) {
	return _facing;
}

bool mm_witch_grounded(void) {
	return _b.ground;
}

int32_t mm_witch_vy(void) {
	return _b.vy;
}

bool mm_witch_casting(void) {
	return _bolt;
}

void mm_witch_box(int32_t* x0, int32_t* y0, int32_t* x1, int32_t* y1) {
	*x0 = mm_witch_x() - _b.hw;
	*x1 = mm_witch_x() + _b.hw;
	*y1 = mm_witch_y();
	*y0 = *y1 - _b.h;
}

bool mm_witch_standing_on(int32_t x0, int32_t x1, int32_t y) {
	return _b.ground && mm_witch_y() == y && mm_witch_x() - _b.hw < x1 && x0 < mm_witch_x() + _b.hw;
}

void mm_witch_hurt(int32_t from_x) {
	if (_hurt || _dizzy || _cheer) return;
	if (_hearts) --_hearts;
	_hurt = MM_HURT_FRAMES;
	_stun = MM_STUN_FRAMES;
	const int32_t away = mm_witch_x() < from_x ? -1 : 1;
	_b.vx = away * GAME_PX(3);
	_b.vy = -GAME_PX(4);
	_bolt = false;
	game_fx_explode(_b.x, _b.y - GAME_PX(40), GAME_WHITE, 10, GAME_PX(2));
	if (!_hearts) {
		_dizzy = MM_DIZZY_FRAMES;
		mm_hud_say(mm_witch_x(), mm_witch_y() - 72, "OH DEAR...", GAME_WHITE);
	}
	else {
		mm_hud_say(mm_witch_x(), mm_witch_y() - 72, "EEK!", GAME_WHITE);
	}
}

bool mm_witch_heal(void) {
	if (_hearts >= MM_WITCH_MAX_HEARTS) return false;
	++_hearts;
	return true;
}

uint32_t mm_witch_hearts(void) {
	return _hearts;
}

bool mm_witch_fainted(void) {
	return _fainted;
}

void mm_witch_add_key(uint32_t group) {
	_keys |= 1u << group;
}

uint32_t mm_witch_keys(void) {
	return _keys;
}

void mm_witch_learn_float(void) {
	_float = true;
}

bool mm_witch_can_float(void) {
	return _float;
}

void mm_witch_set_cheer(bool cheer) {
	_cheer = cheer;
	_bolt = false;
}

static void cast(bool up) {
	_cast = MM_CAST_FRAMES;
	_bolt = true;
	_blife = MM_BOLT_FRAMES;
	if (up) {
		_bx = _b.x;
		_by = _b.y - GAME_PX(MM_WITCH_H + 8);
		_bvx = 0;
		_bvy = -MM_BOLT_SPEED;
	}
	else {
		_bx = _b.x + GAME_PX(_facing * MM_WAND_X);
		_by = _b.y - GAME_PX(MM_WAND_Y);
		_bvx = _facing * MM_BOLT_SPEED;
		_bvy = 0;
	}
}

static void bolt_end(uint32_t colour) {
	_bolt = false;
	game_fx_explode(_bx, _by, colour, 8, GAME_PX(2));
}

static void update_bolt(void) {
	if (!_bolt) return;
	_bx += _bvx;
	_by += _bvy;
	const int32_t x = _bx >> GAME_FP, y = _by >> GAME_FP;
	const int32_t x0 = x - MM_BOLT_R, y0 = y - MM_BOLT_R, x1 = x + MM_BOLT_R, y1 = y + MM_BOLT_R;
	// Levers and candles
	for (int32_t cy = y0 >> MM_TILE_SHIFT; cy <= (y1 - 1) >> MM_TILE_SHIFT; ++cy) {
		for (int32_t cx = x0 >> MM_TILE_SHIFT; cx <= (x1 - 1) >> MM_TILE_SHIFT; ++cx) {
			const int32_t tx = cx * MM_TILE + MM_TILE / 2, ty = cy * MM_TILE;
			switch (mm_map_cast(cx, cy)) {
			case MM_CAST_LEVER:
				mm_hud_say(tx, ty - 4, "CLUNK!", GAME_GREEN);
				mm_hud_score_at(tx, ty, 500);
				bolt_end(GAME_GREEN);
				return;
			case MM_CAST_CANDLE:
				mm_hud_score_at(tx, ty, 100);
				bolt_end(GAME_YELLOW);
				return;
			case MM_CAST_LAST_CANDLE:
				mm_hud_say(tx, ty - 4, "ALL LIT!", GAME_YELLOW);
				mm_hud_score_at(tx, ty, 500);
				bolt_end(GAME_YELLOW);
				return;
			default:
				break;
			}
		}
	}
	if (mm_thing_cast(x0, y0, x1, y1)) {
		bolt_end(GAME_PINK);
		return;
	}
	if (mm_map_box_solid(x - 2, y - 2, x + 2, y + 2) || --_blife == 0) {
		bolt_end(GAME_YELLOW);
		return;
	}
	if (_blife & 1) game_fx_explode(_bx, _by, GAME_YELLOW, 1, GAME_PX(1) / 2);
}

// Standing on a shelf or a creature that can be dropped through
static bool can_drop(void) {
	const int32_t x = mm_witch_x(), y = mm_witch_y();
	return _b.ground && !mm_body_floor(&_b, x, y, true, NULL);
}

// Bumping into a locked door
static void try_door(int32_t dir) {
	const int32_t cx = (mm_witch_x() + dir * (_b.hw + 1)) >> MM_TILE_SHIFT;
	const int32_t cy = (mm_witch_y() - MM_TILE / 2) >> MM_TILE_SHIFT;
	if (mm_map_kind(cx, cy) != MM_C_GATE) return;
	const uint32_t g = mm_map_group(cx, cy);
	if (mm_map_group_type(g) != MM_GROUP_KEY || mm_map_group_open(g)) return;
	if (_keys & (1u << g)) {
		_keys &= ~(1u << g);
		mm_map_unlock(g);
		mm_hud_say(mm_witch_x(), mm_witch_y() - 72, "UNLOCKED!", GAME_YELLOW);
		mm_hud_score_at(mm_witch_x(), mm_witch_y() - 64, 500);
	}
	else if (!_locked_said) {
		mm_hud_say(mm_witch_x(), mm_witch_y() - 72, "LOCKED! FIND THE KEY", GAME_WHITE);
		_locked_said = 120;
	}
}

static void light_lanterns(void) {
	int32_t x0, y0, x1, y1;
	mm_witch_box(&x0, &y0, &x1, &y1);
	for (int32_t cy = y0 >> MM_TILE_SHIFT; cy <= (y1 - 1) >> MM_TILE_SHIFT; ++cy) {
		for (int32_t cx = x0 >> MM_TILE_SHIFT; cx <= (x1 - 1) >> MM_TILE_SHIFT; ++cx) {
			if (!mm_map_light_lantern(cx, cy)) continue;
			_check_cx = cx;
			_check_cy = cy;
			mm_hud_say(cx * MM_TILE + MM_TILE / 2, cy * MM_TILE - 8, "LANTERN LIT", GAME_YELLOW);
			mm_hud_score_at(cx * MM_TILE + MM_TILE / 2, cy * MM_TILE, 100);
		}
	}
}

static void fall(int32_t max) {
	_b.vy += MM_GRAVITY;
	if (_b.vy > max) _b.vy = max;
}

void mm_witch_update(uint8_t input, uint32_t frame) {
	const uint8_t pressed = input & ~_last_input;
	_last_input = input;
	if (_locked_said) --_locked_said;
	_floating = false;

	if (_cheer || _dizzy) {
		// Just stands there
		_b.vx = _b.vx * 7 / 8;
		fall(MM_FALL_MAX);
		mm_body_move(&_b, false, NULL);
		if (_dizzy) {
			// Seeing stars
			if ((_dizzy & 15) == 0) game_fx_explode(_b.x, _b.y - GAME_PX(64), GAME_YELLOW, 3, GAME_PX(1));
			if (--_dizzy == 0) _fainted = true;
		}
		return;
	}
	if (_hurt) --_hurt;
	const bool control = _stun == 0;
	if (_stun) --_stun;

	int32_t want = 0;
	if (control) {
		if (input & MM_INP_BIT_L) want = -1;
		if (input & MM_INP_BIT_R) want = 1;
	}
	if (want) _facing = want;
	if (control) {
		const int32_t target = want * MM_RUN_SPEED;
		const int32_t accel = _b.ground ? MM_GROUND_ACCEL : MM_AIR_ACCEL;
		if (_b.vx < target) _b.vx = _b.vx + accel > target ? target : _b.vx + accel;
		else if (_b.vx > target) _b.vx = _b.vx - accel < target ? target : _b.vx - accel;
	}

	// Jumping, or dropping through a shelf with down held
	if (_b.ground) _coyote = MM_COYOTE_FRAMES;
	else if (_coyote) --_coyote;
	if (control && (pressed & MM_INP_BIT_J)) _buffer = MM_BUFFER_FRAMES;
	else if (_buffer) --_buffer;
	if (_drop) --_drop;
	if (_buffer && _coyote) {
		if ((input & MM_INP_BIT_D) && can_drop()) _drop = MM_DROP_FRAMES;
		else _b.vy = -MM_JUMP_SPEED;
		_buffer = 0;
		_coyote = 0;
	}
	if (!(input & MM_INP_BIT_J) && _b.vy < -MM_JUMP_CUT) _b.vy = -MM_JUMP_CUT;
	_floating = _float && control && (input & MM_INP_BIT_J) && _b.vy > 0;
	fall(_floating ? MM_FLOAT_FALL : MM_FALL_MAX);

	// Pushing a crate along, slowly
	if (_b.ground && want) {
		int32_t x0, y0, x1, y1;
		mm_witch_box(&x0, &y0, &x1, &y1);
		if (mm_thing_crate_box(x0 + want, y0, x1 + want, y1, NULL)) {
			_b.vx = mm_thing_push(x0 + want, y0, x1 + want, y1, want) ? want * MM_PUSH_SPEED : 0;
		}
	}

	mm_body_move(&_b, _drop > 0, NULL);
	if (_b.wall && want) try_door(want);
	if (_b.ground && _b.vx) _walk += game_abs(_b.vx);

	if (control && (pressed & MM_INP_BIT_F) && !_bolt) cast((input & MM_INP_BIT_U) != 0);
	if (_cast) --_cast;
	update_bolt();

	light_lanterns();
	int32_t x0, y0, x1, y1;
	mm_witch_box(&x0, &y0, &x1, &y1);
	mm_thing_touch(x0, y0, x1, y1);

	if (_floating && (frame & 3) == 0) {
		game_fx_explode(_b.x + GAME_PX((int32_t)(game_rand() % 32) - 16), _b.y, GAME_CYAN, 1, GAME_PX(1) / 2);
	}
}

void mm_witch_draw(uint32_t frame) {
	if (_bolt) {
		mm_gfx_draw(&mm_art_star[(frame >> 2) & 1], mm_pal_star, mm_screen_x(_bx >> GAME_FP) - 8, mm_screen_y(_by >> GAME_FP) - 8, 2, 0);
	}
	// Flashes after being bumped
	if (_hurt && !_dizzy && (frame & 4)) return;
	uint32_t f = MM_HAZEL_STAND;
	if (_cheer) f = (frame & 16) ? MM_HAZEL_CHEER : MM_HAZEL_FRONT;
	else if (_dizzy) f = MM_HAZEL_DIZZY;
	else if (_cast) f = MM_HAZEL_CAST;
	else if (!_b.ground) f = _b.vy < 0 ? MM_HAZEL_JUMP : MM_HAZEL_FALL;
	else if (_b.vx) {
		static const uint8_t steps[4] = { MM_HAZEL_WALK1, MM_HAZEL_STAND, MM_HAZEL_WALK2, MM_HAZEL_STAND };
		f = steps[(_walk >> (GAME_FP + 3)) & 3];
	}
	const uint32_t flip = (_facing < 0 && !_cheer) ? MM_GFX_FLIP : 0;
	// Hops up and down when cheering
	const int32_t hop = (_cheer && (frame & 16)) ? 6 : 0;
	mm_gfx_draw(&mm_art_hazel[f], mm_pal_hazel, mm_screen_x(mm_witch_x()) - 24, mm_screen_y(mm_witch_y()) - 64 - hop, 2, flip);
}
