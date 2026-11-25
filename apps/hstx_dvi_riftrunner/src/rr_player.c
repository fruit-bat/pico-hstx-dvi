// Copyright (c) 2024 fruit-bat.

// See rr_player.h.
//
// The ship and its shots are kept in screen positions, fixed point, as they
// move with the screen; they are turned into the world's to hit things.

#include "rr_player.h"
#include "rr_world.h"
#include "rr_enemy.h"
#include "rr_input.h"
#include "rr_hud.h"
#include "rr_fx.h"
#include "rr_gfx.h"
#include "rr_sound.h"
#include "game_util.h"
#include "game_render.h"

#define RR_SHIP_SPEED 640
#define RR_SHIP_SPEED_STEP 192
#define RR_SHIP_START_X 96
#define RR_DEAD_FRAMES 150
#define RR_SAFE_FRAMES 150

#define RR_SHOTS 24
// Shots in flight from each of the ship and drones
#define RR_SHOTS_EACH 3
#define RR_SHOT_SPEED RR_PX(10)
#define RR_LASER_SPEED RR_PX(14)
#define RR_TWIN_VX RR_PX(7)
#define RR_TWIN_VY (-RR_PX(4))
#define RR_BOMB_VX RR_PX(1)
#define RR_BOMB_GRAVITY 40
#define RR_BOMB_MAX_VY RR_PX(5)
#define RR_BOMB_BLAST 30
// Held this long, fire starts charging the beam rather than having fired
#define RR_HOLD_CHARGE 10
#define RR_BEAM_FRAMES 24

// The drones follow where the ship has been
#define RR_TRAIL 64
#define RR_TRAIL_GAP 16
#define RR_DRONE_SIZE 16
#define RR_ORB_SIZE 24
// How often the orb hurts what it touches
#define RR_ORB_BITE 6

#define RR_MAX_SPEED 4

typedef enum {
	RR_SHOT_NONE = 0,
	RR_SHOT_NORMAL,
	RR_SHOT_TWIN,
	RR_SHOT_LASER,
	RR_SHOT_BOMB,
	RR_SHOT_BOMB_UP,
} RrShot_e;

typedef enum {
	RR_PLAYER_ALIVE = 0,
	RR_PLAYER_DEAD,
	RR_PLAYER_OVER,
} RrPlayerState_e;

typedef struct {
	uint8_t type;
	uint8_t owner;
	uint32_t pierce;
	int32_t x, y, vx, vy; // on the screen
} RrShot_t;

static int32_t _x, _y; // the top left of the ship on the screen
static RrPlayerState_e _state = RR_PLAYER_ALIVE;
static uint32_t _timer = 0;
static uint32_t _safe = 0;
static int32_t _fuel = RR_FUEL_MAX;
static bool _burning = true;
static int32_t _cursor = -1;
static uint8_t _level[RR_POWERS];
static uint32_t _held = 0;
static uint32_t _charge = 0;
static uint32_t _pierce = 1;
static RrShot_t _shots[RR_SHOTS];
static bool _moving = false;

static struct {
	uint32_t frames;
	int32_t x, y; // the head of the beam and its middle, on the screen
	int32_t h;
	int32_t damage;
	uint32_t pierce;
} _beam;

static int16_t _trail_x[RR_TRAIL], _trail_y[RR_TRAIL];
static uint32_t _trail_i = 0;

static uint32_t next_pierce(void) {
	if (++_pierce == 0) _pierce = 1;
	return _pierce;
}

static int32_t cam_px(void) {
	return rr_world_camera() >> RR_FP;
}

static void reset_trail(void) {
	for (uint32_t i = 0; i < RR_TRAIL; ++i) {
		_trail_x[i] = _x >> RR_FP;
		_trail_y[i] = _y >> RR_FP;
	}
}

void rr_player_clear_shots(void) {
	for (uint32_t i = 0; i < RR_SHOTS; ++i) _shots[i].type = RR_SHOT_NONE;
	_beam.frames = 0;
}

static void clear_powers(void) {
	for (uint32_t i = 0; i < RR_POWERS; ++i) _level[i] = 0;
	_cursor = -1;
}

void rr_player_init(void) {
	_x = RR_PX(RR_SHIP_START_X);
	_y = RR_PX(240);
	_state = RR_PLAYER_ALIVE;
	_timer = 0;
	_safe = RR_SAFE_FRAMES;
	_fuel = RR_FUEL_MAX;
	_burning = true;
	_held = 0;
	_charge = 0;
	clear_powers();
	rr_player_clear_shots();
	reset_trail();
}

// Where drone i, from 0, is, its top left on the screen in pixels
static void drone_pos(uint32_t i, int32_t* x, int32_t* y) {
	const uint32_t t = (_trail_i - (i + 1) * RR_TRAIL_GAP) & (RR_TRAIL - 1);
	*x = _trail_x[t] + 16;
	*y = _trail_y[t] + (RR_SHIP_H - RR_DRONE_SIZE) / 2;
}

static uint32_t shots_of(uint32_t owner, uint32_t type) {
	uint32_t n = 0;
	for (uint32_t i = 0; i < RR_SHOTS; ++i) {
		if (_shots[i].type == type && _shots[i].owner == owner) ++n;
	}
	return n;
}

static bool add_shot(uint32_t type, uint32_t owner, int32_t x, int32_t y, int32_t vx, int32_t vy, uint32_t pierce) {
	for (uint32_t i = 0; i < RR_SHOTS; ++i) {
		RrShot_t* s = &_shots[i];
		if (s->type != RR_SHOT_NONE) continue;
		s->type = type;
		s->owner = owner;
		s->x = x;
		s->y = y;
		s->vx = vx;
		s->vy = vy;
		s->pierce = pierce;
		return true;
	}
	return false;
}

// From a nose on the screen in pixels
static bool fire_from(int32_t nx, int32_t ny, uint32_t owner) {
	bool fired = false;
	if (_level[RR_POW_LASER]) {
		if (shots_of(owner, RR_SHOT_LASER) < 2) {
			fired = add_shot(RR_SHOT_LASER, owner, RR_PX(nx - 16), RR_PX(ny - 2), RR_LASER_SPEED, 0, next_pierce());
		}
	}
	else if (shots_of(owner, RR_SHOT_NORMAL) < RR_SHOTS_EACH) {
		fired = add_shot(RR_SHOT_NORMAL, owner, RR_PX(nx - 12), RR_PX(ny - 3), RR_SHOT_SPEED, 0, 0);
	}
	if (_level[RR_POW_TWIN] && shots_of(owner, RR_SHOT_TWIN) < 2) {
		fired |= add_shot(RR_SHOT_TWIN, owner, RR_PX(nx - 8), RR_PX(ny - 12), RR_TWIN_VX, RR_TWIN_VY, 0);
	}
	return fired;
}

static void fire(void) {
	const int32_t x = _x >> RR_FP, y = _y >> RR_FP;
	bool fired = fire_from(x + RR_SHIP_NOSE_X, y + RR_SHIP_NOSE_Y, 0);
	for (uint32_t i = 0; i < _level[RR_POW_DRONE]; ++i) {
		int32_t dx, dy;
		drone_pos(i, &dx, &dy);
		fired |= fire_from(dx + RR_DRONE_SIZE, dy + RR_DRONE_SIZE / 2, i + 1);
	}
	if (fired) game_sound_play(_level[RR_POW_LASER] ? RR_SFX_LASER : RR_SFX_SHOT);
}

static bool bomb_from(int32_t x, int32_t y, uint32_t owner) {
	bool dropped = false;
	if (shots_of(owner, RR_SHOT_BOMB) == 0) {
		dropped = add_shot(RR_SHOT_BOMB, owner, RR_PX(x), RR_PX(y), RR_BOMB_VX, RR_PX(1), 0);
	}
	if (_level[RR_POW_BOMB] && shots_of(owner, RR_SHOT_BOMB_UP) == 0) {
		dropped |= add_shot(RR_SHOT_BOMB_UP, owner, RR_PX(x), RR_PX(y - 12), RR_BOMB_VX, -RR_PX(1), 0);
	}
	return dropped;
}

static void bomb(void) {
	const int32_t x = _x >> RR_FP, y = _y >> RR_FP;
	bool dropped = bomb_from(x + 28, y + 22, 0);
	for (uint32_t i = 0; i < _level[RR_POW_DRONE]; ++i) {
		int32_t dx, dy;
		drone_pos(i, &dx, &dy);
		dropped |= bomb_from(dx + 4, dy + RR_DRONE_SIZE, i + 1);
	}
	if (dropped) game_sound_play(RR_SFX_BOMB);
}

static void let_go_beam(void) {
	if (_charge >= RR_BEAM_READY) {
		_beam.frames = RR_BEAM_FRAMES;
		_beam.x = (_x >> RR_FP) + RR_SHIP_NOSE_X;
		_beam.y = (_y >> RR_FP) + RR_SHIP_NOSE_Y;
		_beam.h = 10 + (int32_t)_charge * 30 / RR_BEAM_FULL;
		_beam.damage = 2 + (int32_t)_charge * 8 / RR_BEAM_FULL;
		_beam.pierce = next_pierce();
		game_sound_play(RR_SFX_BEAM);
	}
	_charge = 0;
}

// Whether a point on the screen, in pixels, is in the land
static bool in_land(int32_t sx, int32_t sy) {
	const int32_t wx = cam_px() + sx;
	return sy >= rr_world_floor(wx) || sy <= rr_world_ceil(wx);
}

static void blow_bomb(RrShot_t* s) {
	const int32_t x = (s->x >> RR_FP) + 6, y = (s->y >> RR_FP) + 4;
	rr_fx_explode(rr_world_camera() + RR_PX(x), RR_PX(y), RR_BOOM_SMALL);
	rr_enemy_blast(cam_px() + x, y, RR_BOMB_BLAST, 3);
	game_sound_play(RR_SFX_BLAST);
	s->type = RR_SHOT_NONE;
}

static void update_shots(void) {
	const int32_t cam = cam_px();
	for (uint32_t i = 0; i < RR_SHOTS; ++i) {
		RrShot_t* s = &_shots[i];
		if (s->type == RR_SHOT_NONE) continue;
		if (s->type == RR_SHOT_BOMB || s->type == RR_SHOT_BOMB_UP) {
			const int32_t g = s->type == RR_SHOT_BOMB ? RR_BOMB_GRAVITY : -RR_BOMB_GRAVITY;
			s->vy = game_clamp(s->vy + g, -RR_BOMB_MAX_VY, RR_BOMB_MAX_VY);
		}
		s->x += s->vx;
		s->y += s->vy;
		const int32_t x = s->x >> RR_FP, y = s->y >> RR_FP;
		if (x >= GAME_SCREEN_W || y < RR_PLAY_TOP - 16 || y >= RR_PLAY_BOTTOM) {
			s->type = RR_SHOT_NONE;
			continue;
		}
		switch (s->type) {
			case RR_SHOT_NORMAL:
				if (in_land(x + 22, y + 3) || rr_enemy_shot(cam + x, y, 24, 6, 1, 0)) s->type = RR_SHOT_NONE;
				break;
			case RR_SHOT_TWIN:
				if (in_land(x + 10, y + 2) || rr_enemy_shot(cam + x, y, 12, 12, 1, 0)) s->type = RR_SHOT_NONE;
				break;
			case RR_SHOT_LASER:
				if (in_land(x + 70, y + 2)) s->type = RR_SHOT_NONE;
				else rr_enemy_shot(cam + x, y, 76, 4, 1, s->pierce);
				break;
			default:
				if (in_land(x + 6, s->type == RR_SHOT_BOMB ? y + 8 : y) || rr_enemy_shot(cam + x, y, 12, 8, 3, 0)) blow_bomb(s);
				break;
		}
	}
	if (_beam.frames) {
		--_beam.frames;
		_beam.x += RR_BEAM_SPEED;
		const int32_t x0 = _beam.x - RR_BEAM_LEN;
		rr_enemy_shot(cam + x0, _beam.y - _beam.h / 2, RR_BEAM_LEN, _beam.h, _beam.damage, _beam.pierce);
		if (x0 >= GAME_SCREEN_W) _beam.frames = 0;
	}
}

static void take_power(void) {
	if (_cursor < 0 || rr_player_power_full(_cursor)) return;
	switch (_cursor) {
		case RR_POW_TWIN:
			_level[RR_POW_TWIN] = 1;
			_level[RR_POW_LASER] = 0;
			break;
		case RR_POW_LASER:
			_level[RR_POW_LASER] = 1;
			_level[RR_POW_TWIN] = 0;
			break;
		case RR_POW_DRONE:
			if (!_level[RR_POW_DRONE]) reset_trail();
			++_level[RR_POW_DRONE];
			break;
		default:
			++_level[_cursor];
			break;
	}
	_cursor = -1;
	game_sound_play(RR_SFX_POWER);
}

// Points on the ship that hit the land, from its top left
static const int8_t _feel[][2] = { { 60, 16 }, { 46, 12 }, { 46, 20 }, { 30, 8 }, { 30, 22 }, { 8, 6 }, { 8, 24 } };

static void fly(uint32_t frame, uint16_t input, uint16_t pressed) {
	const int32_t speed = RR_SHIP_SPEED + RR_SHIP_SPEED_STEP * _level[RR_POW_SPEED];
	const int32_t old_x = _x, old_y = _y;
	if (_fuel > 0) {
		if (input & RR_INP_BIT_L) _x -= speed;
		if (input & RR_INP_BIT_R) _x += speed;
		if (input & RR_INP_BIT_U) _y -= speed;
		if (input & RR_INP_BIT_D) _y += speed;
	}
	else {
		// Out of fuel, falling
		_y += RR_PX(2);
		if ((frame & 1) == 0) _x -= RR_PX(1);
	}
	_x = game_clamp(_x, 0, RR_PX(GAME_SCREEN_W - RR_SHIP_W - 24));
	_y = game_clamp(_y, RR_PX(RR_PLAY_TOP), RR_PX(RR_PLAY_BOTTOM - RR_SHIP_H));
	_moving = _x != old_x || _y != old_y;
	if (_moving) {
		_trail_i = (_trail_i + 1) & (RR_TRAIL - 1);
		_trail_x[_trail_i] = _x >> RR_FP;
		_trail_y[_trail_i] = _y >> RR_FP;
	}

	// Fire goes off when pressed; held, it charges the beam until let go
	if (pressed & RR_INP_BIT_F) fire();
	if (input & RR_INP_BIT_F) {
		++_held;
		if (_held > RR_HOLD_CHARGE && _charge < RR_BEAM_FULL) ++_charge;
	}
	else {
		if (_held) let_go_beam();
		_held = 0;
	}
	game_sound_hold_level(RR_SFX_CHARGE, _charge > 4, (float)_charge / RR_BEAM_FULL);
	if (pressed & RR_INP_BIT_B) bomb();
	if (pressed & RR_INP_BIT_POWER) take_power();

	if (_burning && _fuel > 0) {
		--_fuel;
		if (_fuel < RR_FUEL_LOW && (_fuel % 60) == 0) game_sound_play(RR_SFX_FUEL_LOW);
	}

	// The orb bites what it touches
	if (_level[RR_POW_ORB] && (frame % RR_ORB_BITE) == 0) {
		rr_enemy_shot(cam_px() + (_x >> RR_FP) + RR_SHIP_W - 2, (_y >> RR_FP) + 2, RR_ORB_SIZE, RR_ORB_SIZE, 1, next_pierce());
	}

	if (!_safe) {
		const int32_t x = _x >> RR_FP, y = _y >> RR_FP;
		for (uint32_t i = 0; i < sizeof(_feel) / sizeof(_feel[0]); ++i) {
			if (in_land(x + _feel[i][0], y + _feel[i][1])) {
				rr_player_kill();
				return;
			}
		}
		if (_fuel <= 0 && y >= RR_PLAY_BOTTOM - RR_SHIP_H) rr_player_kill();
	}
}

static void respawn(void) {
	_state = RR_PLAYER_ALIVE;
	_x = RR_PX(RR_SHIP_START_X);
	const int32_t wx = cam_px() + RR_SHIP_START_X;
	int32_t top = rr_world_ceil_max(wx - 16, wx + 160);
	int32_t bottom = rr_world_floor_min(wx - 16, wx + 160);
	if (top < RR_PLAY_TOP) top = RR_PLAY_TOP;
	if (bottom > RR_PLAY_BOTTOM) bottom = RR_PLAY_BOTTOM;
	_y = RR_PX((top + bottom) / 2 - RR_SHIP_H / 2);
	_safe = RR_SAFE_FRAMES;
	_fuel = RR_FUEL_MAX;
	reset_trail();
	rr_enemy_clear_bullets();
	game_sound_play(RR_SFX_RESPAWN);
}

void rr_player_update(uint32_t frame, uint16_t input, uint16_t pressed) {
	update_shots();
	switch (_state) {
		case RR_PLAYER_ALIVE:
			if (_safe) --_safe;
			fly(frame, input, pressed);
			break;
		case RR_PLAYER_DEAD:
			game_sound_hold(RR_SFX_CHARGE, false);
			if (--_timer == 0) {
				if (rr_hud_take_life()) respawn();
				else _state = RR_PLAYER_OVER;
			}
			break;
		default:
			break;
	}
}

void rr_player_kill(void) {
	if (_state != RR_PLAYER_ALIVE || _safe) return;
	_state = RR_PLAYER_DEAD;
	_timer = RR_DEAD_FRAMES;
	const int32_t cx = rr_world_camera() + _x + RR_PX(RR_SHIP_W / 2);
	const int32_t cy = _y + RR_PX(RR_SHIP_H / 2);
	rr_fx_explode(cx, cy, RR_BOOM_BIG);
	game_sound_play(RR_SFX_DEAD);
	clear_powers();
	_held = 0;
	_charge = 0;
}

bool rr_player_alive(void) {
	return _state == RR_PLAYER_ALIVE;
}

bool rr_player_over(void) {
	return _state == RR_PLAYER_OVER;
}

int32_t rr_player_px(void) {
	return cam_px() + (_x >> RR_FP);
}

int32_t rr_player_py(void) {
	return _y >> RR_FP;
}

static bool overlap(int32_t ax, int32_t ay, int32_t aw, int32_t ah, int32_t bx, int32_t by, int32_t bw, int32_t bh) {
	return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

bool rr_player_touch(int32_t x, int32_t y, int32_t w, int32_t h) {
	if (_state != RR_PLAYER_ALIVE) return false;
	return overlap(x, y, w, h, rr_player_px(), rr_player_py(), RR_SHIP_W, RR_SHIP_H);
}

bool rr_player_hit(int32_t x, int32_t y, int32_t w, int32_t h) {
	if (_state != RR_PLAYER_ALIVE || _safe) return false;
	// A smaller box than the ship, to be fair
	if (!overlap(x, y, w, h, rr_player_px() + 10, rr_player_py() + 8, RR_SHIP_W - 18, RR_SHIP_H - 14)) return false;
	rr_player_kill();
	return true;
}

bool rr_player_orb(int32_t x, int32_t y, int32_t w, int32_t h) {
	if (_state != RR_PLAYER_ALIVE || !_level[RR_POW_ORB]) return false;
	return overlap(x, y, w, h, rr_player_px() + RR_SHIP_W - 2, rr_player_py() + 2, RR_ORB_SIZE, RR_ORB_SIZE);
}

void rr_player_add_capsule(void) {
	_cursor = (_cursor + 1) % RR_POWERS;
	game_sound_play(RR_SFX_CAPSULE);
}

void rr_player_add_fuel(int32_t fuel) {
	_fuel = _fuel + fuel > RR_FUEL_MAX ? RR_FUEL_MAX : _fuel + fuel;
}

void rr_player_set_burning(bool burning) {
	_burning = burning;
}

int32_t rr_player_fuel(void) {
	return _fuel;
}

int32_t rr_player_power_cursor(void) {
	return _cursor;
}

bool rr_player_power_full(uint32_t power) {
	switch (power) {
		case RR_POW_SPEED: return _level[power] >= RR_MAX_SPEED;
		case RR_POW_DRONE: return _level[power] >= 2;
		default: return _level[power] >= 1;
	}
}

uint32_t rr_player_power_level(uint32_t power) {
	return power < RR_POWERS ? _level[power] : 0;
}

uint32_t rr_player_beam(void) {
	return _charge;
}

// ----------------------------------------------------------------------------
// Drawing

static const hstx_dvi_pixel_t _beam_px[3] = { RR_RGB(255, 255, 255), RR_RGB(0, 255, 255), RR_RGB(0, 96, 255) };

// d1 holds the length, the height above it and a phase above that
static void __not_in_flash_func(beam_renderer)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	const uint32_t v = (uint32_t)(uintptr_t)d1;
	const int32_t len = v & 0x3ff;
	const int32_t h = (v >> 10) & 0x3f;
	const uint32_t phase = v >> 16;
	const int32_t d = game_abs(2 * row - (h - 1));
	const uint32_t band = (uint32_t)(d * 3 / (h + 1));
	// Wavy at the ends, thinner towards the edges
	const int32_t wave = (game_sin(phase + row * 24) + 127) >> 3;
	const int32_t in = d * 2 + wave;
	sprite_render_span(r, x + in, x + len - in / 2, _beam_px[band > 2 ? 2 : band], spriteId);
}

void rr_player_draw_beam(int32_t x, int32_t y, int32_t len, int32_t h, uint32_t frame) {
	if (len <= 0) return;
	if (len > 0x3ff) len = 0x3ff;
	game_spr_add(x, y - h / 2, len, h, (void*)(uintptr_t)(len | (h << 10) | ((frame * 32) & 0xff) << 16), NULL, beam_renderer);
}

void rr_player_draw_shots(uint32_t frame) {
	if (_beam.frames) rr_player_draw_beam(_beam.x - RR_BEAM_LEN, _beam.y, RR_BEAM_LEN, _beam.h, frame);
	for (uint32_t i = 0; i < RR_SHOTS; ++i) {
		const RrShot_t* s = &_shots[i];
		const int32_t x = s->x >> RR_FP, y = s->y >> RR_FP;
		switch (s->type) {
			case RR_SHOT_NORMAL: rr_art_draw(&rr_gfx.shot, x, y); break;
			case RR_SHOT_TWIN: rr_art_draw(&rr_gfx.twin, x, y); break;
			case RR_SHOT_LASER: rr_art_draw(&rr_gfx.laser, x, y); break;
			case RR_SHOT_BOMB:
			case RR_SHOT_BOMB_UP: rr_art_draw(&rr_gfx.bomb, x, y); break;
			default: break;
		}
	}
}

void rr_player_draw(uint32_t frame) {
	if (_state != RR_PLAYER_ALIVE) return;
	const int32_t x = _x >> RR_FP, y = _y >> RR_FP;
	// The charge, a ball of light growing on the nose
	if (_charge > 4) {
		const uint32_t r = 3 + _charge * 12 / RR_BEAM_FULL;
		const bool full = _charge >= RR_BEAM_FULL;
		rr_fx_disc(x + RR_SHIP_NOSE_X + 4, y + RR_SHIP_NOSE_Y, r + (full && (frame & 2) ? 3 : 0), 0,
			(frame & 4) ? RR_RGB(255, 255, 255) : full ? RR_RGB(255, 96, 255) : RR_RGB(0, 224, 255));
	}
	if (_level[RR_POW_ORB]) rr_art_draw(&rr_gfx.orb[(frame >> 3) & 1], x + RR_SHIP_W - 2, y + 2);
	// Blinks while safe after coming back
	if (!_safe || (frame & 4)) {
		rr_art_draw(&rr_gfx.ship, x, y);
		if (_fuel > 0) rr_art_draw(&rr_gfx.flame[(frame >> 1) & 1], x - 16, y + 12);
	}
	for (uint32_t i = 0; i < _level[RR_POW_DRONE]; ++i) {
		int32_t dx, dy;
		drone_pos(i, &dx, &dy);
		rr_art_draw(&rr_gfx.drone[((frame >> 3) + i) & 1], dx, dy);
	}
}
