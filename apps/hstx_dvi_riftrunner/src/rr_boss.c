// Copyright (c) 2024 fruit-bat.

// See rr_boss.h.
//
// The warship is kept on the screen, its top left in fixed point pixels, as
// it keeps pace with the ship; it is turned into the world's to hit things.

#include "rr_boss.h"
#include "rr_enemy.h"
#include "rr_world.h"
#include "rr_player.h"
#include "rr_hud.h"
#include "rr_fx.h"
#include "rr_gfx.h"
#include "rr_sound.h"
#include "game_fx.h"
#include "game_util.h"
#include "game_render.h"

#define RR_BOSS_W 128
#define RR_BOSS_H 80
#define RR_BOSS_X 470
#define RR_BOSS_WALLS 3
#define RR_BOSS_TURRETS 2
#define RR_BOSS_DYING 180
#define RR_BOSS_FLASH 6

typedef enum {
	RR_BOSS_NONE = 0,
	RR_BOSS_ENTER,
	RR_BOSS_FIGHT,
	RR_BOSS_DIE,
	RR_BOSS_DONE,
} RrBossState_e;

typedef struct {
	int16_t x, y, w, h; // from the warship's top left
	int16_t hp;
	uint8_t flash;
	uint32_t pierce;
} RrBossPart_t;

// The walls, front first, then the core, then the turrets above and below
#define RR_PART_CORE RR_BOSS_WALLS
#define RR_PART_TURRET (RR_BOSS_WALLS + 1)
#define RR_PARTS (RR_BOSS_WALLS + 1 + RR_BOSS_TURRETS)

static const RrBossPart_t _parts_start[RR_PARTS] = {
	{ 0, 30, 6, 20, 0, 0, 0 },
	{ 7, 30, 6, 20, 0, 0, 0 },
	{ 14, 30, 6, 20, 0, 0, 0 },
	{ 20, 30, 24, 20, 0, 0, 0 },
	{ 60, -8, 32, 20, 0, 0, 0 },
	{ 60, 66, 32, 20, 0, 0, 0 },
};

// The hull, which takes shots without harm
static const int16_t _hull_boxes[][4] = {
	{ 44, 8, 84, 64 },
	{ 0, 22, 48, 8 },
	{ 0, 50, 48, 8 },
};
#define RR_HULL_BOXES (sizeof(_hull_boxes) / sizeof(_hull_boxes[0]))

static RrBossState_e _state = RR_BOSS_NONE;
static RrBossPart_t _parts[RR_PARTS];
static int32_t _x, _y; // on the screen
static uint32_t _timer = 0;
static uint32_t _loop = 0;

void rr_boss_init(void) {
	_state = RR_BOSS_NONE;
}

void rr_boss_start(uint32_t loop) {
	_loop = loop > 4 ? 4 : loop;
	_state = RR_BOSS_ENTER;
	_x = RR_PX(GAME_SCREEN_W + 20);
	_y = RR_PX(200);
	_timer = 0;
	for (uint32_t i = 0; i < RR_PARTS; ++i) {
		_parts[i] = _parts_start[i];
		_parts[i].hp = i < RR_BOSS_WALLS ? 10 + 4 * _loop : i == RR_PART_CORE ? 24 + 8 * _loop : 10 + 3 * _loop;
	}
}

bool rr_boss_active(void) {
	return _state != RR_BOSS_NONE && _state != RR_BOSS_DONE;
}

bool rr_boss_done(void) {
	return _state == RR_BOSS_DONE;
}

static int32_t world_x(void) {
	return (rr_world_camera() + _x) >> RR_FP;
}

static bool walls_up(void) {
	for (uint32_t i = 0; i < RR_BOSS_WALLS; ++i) if (_parts[i].hp > 0) return true;
	return false;
}

static void fan(void) {
	const int32_t speed = RR_PX(3) + (int32_t)_loop * (RR_PX(1) / 2);
	const int32_t cx = rr_world_camera() + _x + RR_PX(24), cy = _y + RR_PX(40);
	const int32_t n = walls_up() ? 2 : 3;
	for (int32_t k = -n; k <= n; ++k) {
		const uint32_t a = 128 + k * 10;
		rr_enemy_fire(cx, cy, game_sin(a + 64) * speed / 127, game_sin(a) * speed / 127);
	}
	game_sound_play(RR_SFX_ENEMY_FIRE);
}

static void part_box(const RrBossPart_t* p, int32_t* x, int32_t* y) {
	*x = world_x() + p->x;
	*y = (_y >> RR_FP) + p->y;
}

static void blow_up(const RrBossPart_t* p, uint32_t size) {
	int32_t x, y;
	part_box(p, &x, &y);
	rr_fx_explode(RR_PX(x + p->w / 2), RR_PX(y + p->h / 2), size);
}

void rr_boss_update(uint32_t frame) {
	if (_state == RR_BOSS_NONE || _state == RR_BOSS_DONE) return;
	++_timer;
	for (uint32_t i = 0; i < RR_PARTS; ++i) if (_parts[i].flash) --_parts[i].flash;
	switch (_state) {
		case RR_BOSS_ENTER:
			_x -= RR_PX(3) / 2;
			if (_x <= RR_PX(RR_BOSS_X)) {
				_state = RR_BOSS_FIGHT;
				_timer = 0;
			}
			break;
		case RR_BOSS_FIGHT: {
			// Keeps its core in line with the ship, slowly
			const int32_t want = game_clamp(RR_PX(rr_player_py() + RR_SHIP_H / 2 - 40), RR_PX(RR_PLAY_TOP + 16), RR_PX(RR_PLAY_BOTTOM - RR_BOSS_H - 16));
			_y += game_clamp(want - _y, -RR_PX(3) / 4, RR_PX(3) / 4);
			_x = RR_PX(RR_BOSS_X) + game_sin(_timer) * RR_PX(36) / 127;
			const uint32_t gap = walls_up() ? 150 - _loop * 15 : 90 - _loop * 8;
			if (_timer % gap == gap / 2) fan();
			for (uint32_t t = 0; t < RR_BOSS_TURRETS; ++t) {
				const RrBossPart_t* p = &_parts[RR_PART_TURRET + t];
				if (p->hp <= 0 || (_timer + t * 40) % (80 - _loop * 8)) continue;
				int32_t x, y;
				part_box(p, &x, &y);
				rr_enemy_fire_at(RR_PX(x + 8), RR_PX(y + (t ? 14 : 6)), RR_PX(3) + (int32_t)_loop * (RR_PX(1) / 2));
			}
			// Running into it
			const int32_t wx = world_x(), wy = _y >> RR_FP;
			for (uint32_t i = 0; i < RR_HULL_BOXES; ++i) {
				rr_player_hit(wx + _hull_boxes[i][0], wy + _hull_boxes[i][1], _hull_boxes[i][2], _hull_boxes[i][3]);
			}
			for (uint32_t i = RR_PART_TURRET; i < RR_PARTS; ++i) {
				const RrBossPart_t* p = &_parts[i];
				if (p->hp > 0) rr_player_hit(wx + p->x + 4, wy + p->y + 4, p->w - 8, p->h - 8);
			}
			break;
		}
		case RR_BOSS_DIE:
			_y += RR_PX(1) / 4;
			if ((_timer & 7) == 0) {
				rr_fx_explode(rr_world_camera() + _x + RR_PX(game_rand() % RR_BOSS_W), _y + RR_PX(game_rand() % RR_BOSS_H),
					(_timer & 8) ? RR_BOOM_BIG : RR_BOOM_MEDIUM);
				game_sound_play(RR_SFX_EXPLODE);
			}
			if (_timer >= RR_BOSS_DYING) {
				rr_fx_explode(rr_world_camera() + _x + RR_PX(RR_BOSS_W / 2), _y + RR_PX(RR_BOSS_H / 2), RR_BOOM_HUGE);
				game_fx_flash(8);
				game_sound_play(RR_SFX_BOSS_DIE);
				_state = RR_BOSS_DONE;
			}
			break;
		default:
			break;
	}
}

static void hurt(uint32_t i, int32_t damage) {
	RrBossPart_t* p = &_parts[i];
	p->hp -= damage;
	if (p->hp > 0) {
		p->flash = RR_BOSS_FLASH;
		game_sound_play(RR_SFX_BOSS_HIT);
		return;
	}
	int32_t x, y;
	part_box(p, &x, &y);
	if (i == RR_PART_CORE) {
		rr_hud_add_score(10000);
		game_fx_popup(RR_PX(x), y, 10000);
		_state = RR_BOSS_DIE;
		_timer = 0;
		blow_up(p, RR_BOOM_BIG);
		game_sound_play(RR_SFX_EXPLODE_BIG);
		rr_enemy_clear_bullets();
		return;
	}
	const uint32_t points = i < RR_BOSS_WALLS ? 500 : 1000;
	rr_hud_add_score(points);
	game_fx_popup(RR_PX(x), y, points);
	blow_up(p, RR_BOOM_MEDIUM);
	game_sound_play(RR_SFX_EXPLODE);
}

static bool overlap(int32_t ax, int32_t ay, int32_t aw, int32_t ah, int32_t bx, int32_t by, int32_t bw, int32_t bh) {
	return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

bool rr_boss_shot(int32_t x, int32_t y, int32_t w, int32_t h, int32_t damage, uint32_t pierce) {
	if (_state != RR_BOSS_ENTER && _state != RR_BOSS_FIGHT) return false;
	const int32_t wx = world_x(), wy = _y >> RR_FP;
	bool hit = false;
	const bool shielded = walls_up();
	for (uint32_t i = 0; i < RR_PARTS; ++i) {
		RrBossPart_t* p = &_parts[i];
		if (p->hp <= 0 || !overlap(x, y, w, h, wx + p->x, wy + p->y, p->w, p->h)) continue;
		if (pierce) {
			if (p->pierce == pierce) continue;
			p->pierce = pierce;
		}
		hit = true;
		// Behind the walls the core shrugs it off
		if (i == RR_PART_CORE && shielded) {
			if (!pierce) return true;
			continue;
		}
		hurt(i, damage);
		if (!pierce || _state != RR_BOSS_FIGHT) return true;
	}
	if (pierce) return hit;
	for (uint32_t i = 0; i < RR_HULL_BOXES; ++i) {
		if (overlap(x, y, w, h, wx + _hull_boxes[i][0], wy + _hull_boxes[i][1], _hull_boxes[i][2], _hull_boxes[i][3])) return true;
	}
	return hit;
}

void rr_boss_blast(int32_t x, int32_t y, int32_t r, int32_t damage) {
	if (_state != RR_BOSS_FIGHT) return;
	const int32_t wx = world_x(), wy = _y >> RR_FP;
	for (uint32_t i = 0; i < RR_PARTS; ++i) {
		const RrBossPart_t* p = &_parts[i];
		if (p->hp <= 0 || (i == RR_PART_CORE && walls_up())) continue;
		const int32_t nx = game_clamp(x, wx + p->x, wx + p->x + p->w);
		const int32_t ny = game_clamp(y, wy + p->y, wy + p->y + p->h);
		if ((nx - x) * (nx - x) + (ny - y) * (ny - y) <= r * r) {
			hurt(i, damage);
			if (_state != RR_BOSS_FIGHT) return;
		}
	}
}

bool rr_boss_target(int32_t* tx, int32_t* ty) {
	if (_state != RR_BOSS_FIGHT) return false;
	// The front wall still standing, or else the core
	for (uint32_t i = 0; i <= RR_PART_CORE; ++i) {
		const RrBossPart_t* p = &_parts[i];
		if (p->hp <= 0) continue;
		*tx = world_x() + p->x + p->w / 2;
		*ty = (_y >> RR_FP) + p->y + p->h / 2;
		return true;
	}
	return false;
}

static void draw_part(uint32_t i, const RrArt_t* a, int32_t sx, int32_t sy) {
	const RrBossPart_t* p = &_parts[i];
	if (p->hp <= 0) return;
	if (p->flash) rr_art_draw_flash(a, sx + p->x, sy + p->y, (p->flash & 2) ? RR_RGB(255, 255, 255) : RR_RGB(255, 64, 64));
	else rr_art_draw(a, sx + p->x, sy + p->y);
}

void rr_boss_draw(uint32_t frame) {
	if (_state == RR_BOSS_NONE || _state == RR_BOSS_DONE) return;
	const int32_t sx = _x >> RR_FP, sy = _y >> RR_FP;
	draw_part(RR_PART_TURRET, &rr_gfx.turret[1], sx, sy);
	draw_part(RR_PART_TURRET + 1, &rr_gfx.turret_c[1], sx, sy);
	for (uint32_t i = 0; i < RR_BOSS_WALLS; ++i) draw_part(i, &rr_gfx.wall, sx, sy);
	// The core pulses, faster once bare
	const uint32_t pulse = walls_up() ? (frame >> 4) % 3 : (frame >> 2) % 3;
	if (_state == RR_BOSS_DIE) {
		if (frame & 2) rr_art_draw_flash(&rr_gfx.core[0], sx + 20, sy + 30, RR_RGB(255, 255, 255));
	}
	else {
		draw_part(RR_PART_CORE, &rr_gfx.core[pulse], sx, sy);
	}
	if (_state == RR_BOSS_DIE && (frame & 4)) rr_art_draw_flash(&rr_gfx.hull, sx, sy, RR_RGB(255, 96, 32));
	else rr_art_draw(&rr_gfx.hull, sx, sy);
}
