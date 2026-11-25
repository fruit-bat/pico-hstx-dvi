// Copyright (c) 2024 fruit-bat.

// See rr_enemy.h.

#include "rr_enemy.h"
#include "rr_boss.h"
#include "rr_world.h"
#include "rr_player.h"
#include "rr_hud.h"
#include "rr_fx.h"
#include "rr_gfx.h"
#include "rr_sound.h"
#include "game_fx.h"
#include "game_util.h"
#include "game_render.h"
#include "game_pallet.h"
#include <math.h>

#define RR_ENEMIES 56
#define RR_BULLETS 40
#define RR_WAVES 8
#define RR_BULLET_SIZE 8
#define RR_BULLET_SPEED RR_PX(3)
#define RR_HIT_FLASH 6
// Gone once this far off the left of the screen
#define RR_OFF_LEFT 80

typedef struct {
	const char* name;
	uint16_t score;
	int8_t hp;
	uint8_t boom;
	uint8_t ground; // on the floor or ceiling, moving with the land
} RrEnemyInfo_t;

static const RrEnemyInfo_t _info[RR_EN_TYPES] = {
	[RR_EN_DART] = { "DART", 100, 1, RR_BOOM_SMALL, 0 },
	[RR_EN_CARRIER] = { "CARRIER", 300, 1, RR_BOOM_MEDIUM, 0 },
	[RR_EN_SAUCER] = { "SAUCER", 200, 2, RR_BOOM_MEDIUM, 0 },
	[RR_EN_METEOR] = { "METEOR", 300, 6, RR_BOOM_BIG, 0 },
	[RR_EN_ROCK] = { "ROCK", 50, 2, RR_BOOM_SMALL, 0 },
	[RR_EN_ROCKET] = { "ROCKET", 80, 1, RR_BOOM_MEDIUM, 1 },
	[RR_EN_FUEL] = { "FUEL", 150, 1, RR_BOOM_MEDIUM, 1 },
	[RR_EN_TURRET] = { "TURRET", 200, 2, RR_BOOM_MEDIUM, 1 },
	[RR_EN_TURRET_C] = { "TURRET", 200, 2, RR_BOOM_MEDIUM, 1 },
	[RR_EN_JELLY] = { "JELLYFISH", 500, 8, RR_BOOM_HUGE, 0 },
	[RR_EN_MOSQUITO] = { "MOSQUITO", 300, 3, RR_BOOM_BIG, 0 },
	[RR_EN_WASP] = { "WASP", 400, 5, RR_BOOM_BIG, 0 },
	[RR_EN_CAPSULE] = { "CAPSULE", 0, 0, RR_BOOM_SMALL, 0 },
};

typedef struct {
	uint8_t type;   // RR_EN_TYPES when not in use
	uint8_t state;
	int8_t hp;
	uint8_t flash;
	uint8_t wave;   // its wave, plus 1, or 0
	uint8_t phase;
	uint8_t pattern;
	uint16_t timer;
	uint32_t pierce;
	int32_t x, y;   // top left
	int32_t vx, vy;
	int32_t base_y;
} RrEnemy_t;

typedef struct {
	int32_t x, y, vx, vy;
	bool active;
} RrBullet_t;

typedef struct {
	uint8_t count;  // 0 when free
	uint8_t killed;
	uint8_t left;   // still flying
	bool broken;    // one got away
} RrWave_t;

static RrEnemy_t _enemies[RR_ENEMIES];
static RrBullet_t _bullets[RR_BULLETS];
static RrWave_t _waves[RR_WAVES];
static uint32_t _loop = 0;
static uint32_t _frame = 0;

static const RrArt_t* art_of(const RrEnemy_t* e, uint32_t frame) {
	switch (e->type) {
		case RR_EN_DART: return &rr_gfx.dart[(frame >> 3) & 1];
		case RR_EN_CARRIER: return &rr_gfx.carrier[(frame >> 2) & 1];
		case RR_EN_SAUCER: return &rr_gfx.saucer[(frame >> 3) & 1];
		case RR_EN_METEOR: return &rr_gfx.meteor;
		case RR_EN_ROCK: return &rr_gfx.rock;
		case RR_EN_ROCKET: return &rr_gfx.rocket;
		case RR_EN_FUEL: return &rr_gfx.fuel;
		case RR_EN_TURRET: return &rr_gfx.turret[e->phase % 3];
		case RR_EN_TURRET_C: return &rr_gfx.turret_c[e->phase % 3];
		// Squeezed while pushing up
		case RR_EN_JELLY: return &rr_gfx.jelly[e->state == 1 ? 1 : 0];
		case RR_EN_MOSQUITO: return &rr_gfx.mosquito[(frame >> 1) & 1];
		// Facing the way it flies
		case RR_EN_WASP: return e->state == 0 ? &rr_gfx.wasp_r[(frame >> 1) & 1] : &rr_gfx.wasp_l[(frame >> 1) & 1];
		default: return &rr_gfx.capsule[(frame >> 3) & 1];
	}
}

static int32_t width_of(const RrEnemy_t* e) {
	return rr_art_w(art_of(e, 0));
}

static int32_t height_of(const RrEnemy_t* e) {
	return rr_art_h(art_of(e, 0));
}

void rr_enemy_init(void) {
	for (uint32_t i = 0; i < RR_ENEMIES; ++i) _enemies[i].type = RR_EN_TYPES;
	for (uint32_t i = 0; i < RR_WAVES; ++i) _waves[i].count = 0;
	rr_enemy_clear_bullets();
}

void rr_enemy_set_loop(uint32_t loop) {
	_loop = loop;
}

void rr_enemy_clear_bullets(void) {
	for (uint32_t i = 0; i < RR_BULLETS; ++i) _bullets[i].active = false;
}

static RrEnemy_t* add(uint32_t type, int32_t px, int32_t y) {
	for (uint32_t i = 0; i < RR_ENEMIES; ++i) {
		RrEnemy_t* e = &_enemies[i];
		if (e->type != RR_EN_TYPES) continue;
		e->type = type;
		e->state = 0;
		e->hp = _info[type].hp;
		e->flash = 0;
		e->wave = 0;
		e->phase = 0;
		e->pattern = 0;
		e->timer = 0;
		e->pierce = 0;
		e->x = RR_PX(px);
		e->y = RR_PX(y);
		e->base_y = e->y;
		e->vx = 0;
		e->vy = 0;
		return e;
	}
	return NULL;
}

void rr_enemy_spawn(uint32_t type, int32_t px, int32_t y) {
	RrEnemy_t* e = add(type, px, y);
	if (!e) return;
	switch (type) {
		case RR_EN_CARRIER:
			e->vx = RR_SCROLL - RR_PX(5) / 2;
			e->phase = game_rand();
			break;
		case RR_EN_SAUCER:
			e->vx = RR_SCROLL - RR_PX(5) / 2;
			break;
		case RR_EN_METEOR:
			e->vx = RR_SCROLL - RR_PX(1) - (int32_t)(game_rand() % RR_PX(2));
			e->vy = (int32_t)(game_rand() % 192) - 96;
			break;
		case RR_EN_ROCKET:
			// How close the ship gets before it goes
			e->timer = 140 + game_rand() % 200;
			break;
		case RR_EN_TURRET:
		case RR_EN_TURRET_C:
			e->timer = 60 + game_rand() % 90;
			break;
		default:
			break;
	}
}

// The giants' ways of moving
#define RR_JELLY_CYCLE 100   // frames between pushes
#define RR_JELLY_PUSH 18     // frames squeezed, pushing up
#define RR_JELLY_SINK 8      // how fast it starts sinking, a frame
#define RR_MOSQUITO_HOVER 50
#define RR_MOSQUITO_DASH 26
#define RR_MOSQUITO_DASHES 3
#define RR_WASP_AHEAD 230    // how far ahead of the ship it turns
#define RR_WASP_TURN 36      // frames turned, before diving back

void rr_enemy_giants(uint32_t type, uint32_t count, int32_t y) {
	uint32_t w = 0;
	while (w < RR_WAVES && _waves[w].count) ++w;
	const int32_t cam = rr_world_camera() >> RR_FP;
	uint32_t made = 0;
	for (uint32_t i = 0; i < count; ++i) {
		int32_t px, py;
		switch (type) {
			case RR_EN_JELLY:
				px = cam + GAME_SCREEN_W + 20 + (int32_t)i * 170;
				py = y + (int32_t)(game_rand() % 120) - 60;
				break;
			case RR_EN_WASP:
				// From behind, one after the other
				px = cam - 110 - (int32_t)i * 140;
				py = y + ((i & 1) ? 60 : -60);
				break;
			default:
				px = cam + GAME_SCREEN_W + 20 + (int32_t)i * 70;
				py = y + (int32_t)i * 50 - (int32_t)count * 25;
				break;
		}
		py = game_clamp(py, RR_PLAY_TOP + 8, RR_PLAY_BOTTOM - 100);
		RrEnemy_t* e = add(type, px, py);
		if (!e) break;
		e->phase = game_rand();
		e->wave = w < RR_WAVES ? w + 1 : 0;
		switch (type) {
			case RR_EN_JELLY:
				// Drifting slowly, each pushing at its own time
				e->vx = RR_SCROLL - RR_PX(5) / 4;
				e->timer = (i * 37) % RR_JELLY_CYCLE;
				break;
			case RR_EN_MOSQUITO:
				e->vx = RR_SCROLL - RR_PX(2);
				e->timer = i * 12;
				break;
			default:
				e->vx = RR_SCROLL + RR_PX(9) / 2;
				break;
		}
		++made;
	}
	if (w < RR_WAVES && made) {
		_waves[w].count = made;
		_waves[w].killed = 0;
		_waves[w].left = made;
		_waves[w].broken = false;
	}
	game_sound_play(type == RR_EN_JELLY ? RR_SFX_PULSE : RR_SFX_BUZZ);
}

bool rr_enemy_place(int32_t px, int32_t y, bool ceiling, uint32_t section) {
	const uint32_t r = game_rand() % 100;
	uint32_t type = RR_EN_TYPES;
	if (ceiling) {
		const uint32_t chance = section == RR_SEC_FORTRESS ? 40 : section == RR_SEC_CAVERNS ? 30 : 0;
		if (r < chance) type = RR_EN_TURRET_C;
	}
	else {
		// Out of a hundred: rockets, fuel, turrets
		static const uint8_t odds[RR_SECTIONS][3] = {
			[RR_SEC_RIDGES] = { 45, 20, 10 },
			[RR_SEC_CAVERNS] = { 25, 20, 20 },
			[RR_SEC_DEBRIS] = { 0, 25, 15 },
			[RR_SEC_CITY] = { 35, 20, 15 },
			[RR_SEC_FORTRESS] = { 20, 20, 25 },
			[RR_SEC_WARSHIP] = { 0, 0, 0 },
		};
		const uint8_t* o = odds[section < RR_SECTIONS ? section : 0];
		if (r < o[0]) type = RR_EN_ROCKET;
		else if (r < o[0] + o[1]) type = RR_EN_FUEL;
		else if (r < o[0] + o[1] + o[2]) type = RR_EN_TURRET;
	}
	if (type == RR_EN_TYPES) return false;
	RrEnemy_t probe = { .type = type };
	const int32_t w = width_of(&probe), h = height_of(&probe);
	// Sat on the floor, or hung from the ceiling, a little into it
	rr_enemy_spawn(type, px - w / 2, ceiling ? y - 2 : y - h + 2);
	return true;
}

void rr_enemy_wave(uint32_t type, uint32_t count, int32_t y, uint32_t pattern) {
	uint32_t w = 0;
	while (w < RR_WAVES && _waves[w].count) ++w;
	const int32_t px = (rr_world_camera() >> RR_FP) + GAME_SCREEN_W + 16;
	uint32_t made = 0;
	for (uint32_t i = 0; i < count; ++i) {
		RrEnemy_t* e = add(type, px + (int32_t)i * 40, y);
		if (!e) break;
		e->pattern = pattern;
		e->phase = i * 20;
		e->vx = RR_SCROLL - (pattern == RR_PAT_HAIRPIN ? RR_PX(4) : RR_PX(3));
		e->wave = w < RR_WAVES ? w + 1 : 0;
		++made;
	}
	if (w < RR_WAVES && made) {
		_waves[w].count = made;
		_waves[w].killed = 0;
		_waves[w].left = made;
		_waves[w].broken = false;
	}
}

static void wave_gone(RrEnemy_t* e, bool killed) {
	if (!e->wave) return;
	RrWave_t* w = &_waves[e->wave - 1];
	if (killed) ++w->killed;
	else w->broken = true;
	if (w->left) --w->left;
	// The whole wave shot leaves a capsule where the last one was
	if (killed && !w->broken && w->killed == w->count) {
		rr_enemy_spawn(RR_EN_CAPSULE, e->x >> RR_FP, e->y >> RR_FP);
	}
	if (!w->left) w->count = 0;
	e->wave = 0;
}

static void drop(RrEnemy_t* e) {
	wave_gone(e, false);
	e->type = RR_EN_TYPES;
}

static void kill(RrEnemy_t* e, bool scored) {
	const int32_t w = width_of(e), h = height_of(e);
	const int32_t cx = e->x + RR_PX(w / 2), cy = e->y + RR_PX(h / 2);
	rr_fx_explode(cx, cy, _info[e->type].boom);
	game_sound_play(e->type == RR_EN_METEOR ? RR_SFX_EXPLODE_BIG : RR_SFX_EXPLODE);
	if (scored) {
		const uint32_t points = _info[e->type].score + (e->type == RR_EN_ROCKET && e->state ? 40 : 0);
		rr_hud_add_score(points);
		game_fx_popup(e->x, e->y >> RR_FP, points);
	}
	switch (e->type) {
		case RR_EN_FUEL:
			rr_player_add_fuel(RR_FUEL_TANK);
			game_sound_play(RR_SFX_FUEL);
			break;
		case RR_EN_CARRIER:
			rr_enemy_spawn(RR_EN_CAPSULE, e->x >> RR_FP, e->y >> RR_FP);
			break;
		case RR_EN_METEOR:
			for (int32_t i = -1; i <= 1; i += 2) {
				RrEnemy_t* r = add(RR_EN_ROCK, (cx >> RR_FP) - 12, (cy >> RR_FP) - 7 + i * 10);
				if (r) {
					r->vx = e->vx - RR_PX(1) / 2;
					r->vy = i * RR_PX(1);
				}
			}
			break;
		default:
			break;
	}
	wave_gone(e, true);
	e->type = RR_EN_TYPES;
}

static void hurt(RrEnemy_t* e, int32_t damage) {
	e->hp -= damage;
	if (e->hp <= 0) {
		kill(e, true);
	}
	else {
		e->flash = RR_HIT_FLASH;
		game_sound_play(RR_SFX_HIT);
	}
}

static bool overlap(int32_t ax, int32_t ay, int32_t aw, int32_t ah, int32_t bx, int32_t by, int32_t bw, int32_t bh) {
	return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

bool rr_enemy_shot(int32_t x, int32_t y, int32_t w, int32_t h, int32_t damage, uint32_t pierce) {
	bool hit = false;
	for (uint32_t i = 0; i < RR_ENEMIES; ++i) {
		RrEnemy_t* e = &_enemies[i];
		if (e->type >= RR_EN_CAPSULE) continue;
		if (!overlap(x, y, w, h, e->x >> RR_FP, e->y >> RR_FP, width_of(e), height_of(e))) continue;
		if (pierce) {
			if (e->pierce == pierce) continue;
			e->pierce = pierce;
		}
		hurt(e, damage);
		hit = true;
		if (!pierce) return true;
	}
	return rr_boss_shot(x, y, w, h, damage, pierce) || hit;
}

void rr_enemy_blast(int32_t x, int32_t y, int32_t r, int32_t damage) {
	for (uint32_t i = 0; i < RR_ENEMIES; ++i) {
		RrEnemy_t* e = &_enemies[i];
		if (e->type >= RR_EN_CAPSULE) continue;
		const int32_t ex = e->x >> RR_FP, ey = e->y >> RR_FP;
		const int32_t nx = game_clamp(x, ex, ex + width_of(e));
		const int32_t ny = game_clamp(y, ey, ey + height_of(e));
		if ((nx - x) * (nx - x) + (ny - y) * (ny - y) <= r * r) hurt(e, damage);
	}
	rr_boss_blast(x, y, r, damage);
}

void rr_enemy_fire(int32_t x, int32_t y, int32_t vx, int32_t vy) {
	for (uint32_t i = 0; i < RR_BULLETS; ++i) {
		RrBullet_t* b = &_bullets[i];
		if (b->active) continue;
		b->active = true;
		b->x = x - RR_PX(RR_BULLET_SIZE / 2);
		b->y = y - RR_PX(RR_BULLET_SIZE / 2);
		// Moving with the land, so as seen on the screen
		b->vx = vx + RR_SCROLL;
		b->vy = vy;
		return;
	}
}

void rr_enemy_fire_at(int32_t x, int32_t y, int32_t speed) {
	if (!rr_player_alive()) return;
	const float dx = (float)(RR_PX(rr_player_px() + RR_SHIP_W / 2) - x);
	const float dy = (float)(RR_PX(rr_player_py() + RR_SHIP_H / 2) - y);
	const float len = sqrtf(dx * dx + dy * dy);
	if (len < 1.0f) return;
	rr_enemy_fire(x, y, (int32_t)(dx * speed / len), (int32_t)(dy * speed / len));
	game_sound_play(RR_SFX_ENEMY_FIRE);
}

static int32_t bullet_speed(void) {
	return RR_BULLET_SPEED + (int32_t)(_loop > 4 ? 4 : _loop) * (RR_PX(1) / 2);
}

static uint32_t fire_gap(void) {
	const uint32_t cut = (_loop > 4 ? 4 : _loop) * 15;
	return 110 - cut + game_rand() % 60;
}

static void update_turret(RrEnemy_t* e, bool ceiling) {
	const int32_t w = width_of(e);
	const int32_t cx = (e->x >> RR_FP) + w / 2;
	const int32_t cy = (e->y >> RR_FP) + (ceiling ? 14 : 6);
	const int32_t dx = rr_player_px() + RR_SHIP_W / 2 - cx;
	int32_t dy = rr_player_py() + RR_SHIP_H / 2 - cy;
	if (ceiling) dy = -dy;
	// Pointing left, up and left, or up; down for those on the ceiling
	if (dx >= 0 || game_abs(dx) < -dy / 2) e->phase = 2;
	else if (dy > dx / 2) e->phase = 0;
	else e->phase = 1;
	const int32_t sx = rr_world_sx(e->x);
	if (e->timer) --e->timer;
	if (!e->timer && sx > 0 && sx < GAME_SCREEN_W - w) {
		rr_enemy_fire_at(RR_PX(cx), RR_PX(cy), bullet_speed());
		e->timer = fire_gap();
	}
}

static void update_enemy(RrEnemy_t* e) {
	++e->timer;
	switch (e->type) {
		case RR_EN_DART:
			e->x += e->vx;
			if (e->pattern == RR_PAT_HAIRPIN) {
				if (e->state == 0 && rr_world_sx(e->x) < 220) {
					// Turn back out, towards the ship
					e->state = 1;
					e->vx = RR_SCROLL + RR_PX(3);
					e->vy = rr_player_py() > (e->y >> RR_FP) ? RR_PX(2) : -RR_PX(2);
					e->timer = 0;
				}
				if (e->state == 1 && e->timer > 30) e->vy = 0;
				e->y += e->vy;
			}
			else {
				e->y = e->base_y + game_sin(e->phase + e->timer * 2) * RR_PX(56) / 127;
			}
			break;
		case RR_EN_CARRIER:
			e->x += e->vx;
			e->y = e->base_y + game_sin(e->phase + e->timer * 2) * RR_PX(30) / 127;
			break;
		case RR_EN_SAUCER: {
			e->x += e->vx;
			const int32_t sx = rr_world_sx(e->x);
			const int32_t ty = RR_PX(rr_player_py());
			if (sx > (rr_player_px() - (rr_world_camera() >> RR_FP)) + 120) {
				e->y += game_clamp(ty - e->y, -RR_PX(1), RR_PX(1));
			}
			if (!e->state && sx < 480 && sx > 200) {
				e->state = 1;
				rr_enemy_fire_at(e->x + RR_PX(8), e->y + RR_PX(6), bullet_speed());
			}
			break;
		}
		case RR_EN_METEOR:
		case RR_EN_ROCK:
			e->x += e->vx;
			e->y += e->vy;
			if (e->y < RR_PX(RR_PLAY_TOP) || e->y > RR_PX(RR_PLAY_BOTTOM - height_of(e))) e->vy = -e->vy;
			break;
		case RR_EN_ROCKET:
			if (!e->state) {
				if (rr_player_alive() && (e->x >> RR_FP) - rr_player_px() < (int32_t)e->timer) {
					e->state = 1;
					game_sound_play(RR_SFX_LAUNCH);
				}
			}
			else {
				e->vy = e->vy - 12 < -RR_PX(5) ? -RR_PX(5) : e->vy - 12;
				e->y += e->vy;
			}
			break;
		case RR_EN_TURRET:
			update_turret(e, false);
			break;
		case RR_EN_TURRET_C:
			update_turret(e, true);
			break;
		case RR_EN_CAPSULE:
			e->y = e->base_y + game_sin(e->timer * 4) * RR_PX(4) / 127;
			break;
		case RR_EN_JELLY: {
			// Squeezes and pushes up, then opens out and sinks slowly
			const uint32_t t = e->timer % RR_JELLY_CYCLE;
			if (t == 0) {
				e->state = 1;
				e->vy = -RR_PX(2);
				if (rr_world_sx(e->x) < GAME_SCREEN_W - 40) game_sound_play(RR_SFX_PULSE);
			}
			else if (t == RR_JELLY_PUSH) {
				// Opening out stops it, then it sinks
				e->state = 0;
				e->vy = 0;
				// Now and then a sting, dropped straight down
				if ((e->phase++ & 1) && rr_world_sx(e->x) < GAME_SCREEN_W - 40) {
					rr_enemy_fire(e->x + RR_PX(40), e->y + RR_PX(60), -RR_SCROLL / 2, bullet_speed() / 2);
				}
			}
			if (e->state == 0 && e->vy < RR_PX(1) / 2) e->vy += RR_JELLY_SINK;
			// Drawn back towards where it came in, so a group stays spread out
			if (e->state == 0) e->vy += game_clamp((e->base_y - e->y) >> 10, -2, 2);
			e->x += e->vx;
			e->y += e->vy;
			// Kept off the top, and pushed up sooner near the bottom
			if (e->y < RR_PX(RR_PLAY_TOP)) e->y = RR_PX(RR_PLAY_TOP);
			if (e->y > RR_PX(RR_PLAY_BOTTOM - 110) && t > RR_JELLY_PUSH + 10) e->timer += RR_JELLY_CYCLE - t - 1;
			break;
		}
		case RR_EN_MOSQUITO: {
			const int32_t sx = rr_world_sx(e->x);
			if (e->state == 0) {
				// Hovering, jittering, until it is time to dart
				e->x += RR_SCROLL - RR_PX(1) / 2;
				e->y += ((int32_t)(game_rand() % 5) - 2) * RR_PX(1) / 2;
				if (sx > GAME_SCREEN_W - 60) e->x -= RR_PX(2);
				if (e->timer >= RR_MOSQUITO_HOVER && sx < GAME_SCREEN_W - 40) {
					if (e->phase >= RR_MOSQUITO_DASHES || !rr_player_alive()) {
						// Done, off it goes
						e->state = 2;
						e->vx = RR_SCROLL - RR_PX(6);
						e->vy = 0;
					}
					else {
						// Straight at where the ship is now
						const float dx = (float)(RR_PX(rr_player_px() + RR_SHIP_W / 2) - e->x - RR_PX(48));
						const float dy = (float)(RR_PX(rr_player_py() + RR_SHIP_H / 2) - e->y - RR_PX(24));
						const float len = sqrtf(dx * dx + dy * dy) + 1.0f;
						const float speed = (float)(RR_PX(5) + (int32_t)(_loop > 4 ? 4 : _loop) * RR_PX(1) / 2);
						e->vx = RR_SCROLL + (int32_t)(dx * speed / len);
						e->vy = (int32_t)(dy * speed / len);
						e->state = 1;
						++e->phase;
						game_sound_play(RR_SFX_BUZZ);
					}
					e->timer = 0;
				}
			}
			else {
				e->x += e->vx;
				e->y += e->vy;
				if (e->state == 1 && e->timer >= RR_MOSQUITO_DASH) {
					e->state = 0;
					e->timer = 0;
				}
			}
			e->y = game_clamp(e->y, RR_PX(RR_PLAY_TOP), RR_PX(RR_PLAY_BOTTOM - 48));
			break;
		}
		case RR_EN_WASP: {
			const int32_t sx = rr_world_sx(e->x);
			const int32_t ship_sx = rr_player_px() - (rr_world_camera() >> RR_FP);
			if (e->state == 0) {
				// Overtaking, weaving
				e->x += e->vx;
				e->y = e->base_y + game_sin(e->phase + e->timer * 4) * RR_PX(40) / 127;
				if (sx > ship_sx + RR_WASP_AHEAD || sx > GAME_SCREEN_W - 110) {
					// Turns to face the ship, and stings
					e->state = 1;
					e->timer = 0;
					e->vx = RR_SCROLL;
				}
			}
			else if (e->state == 1) {
				e->x += e->vx;
				if (e->timer == 12) {
					rr_enemy_fire_at(e->x + RR_PX(8), e->y + RR_PX(26), bullet_speed() + RR_PX(2));
					game_sound_play(RR_SFX_STING);
				}
				if (e->timer >= RR_WASP_TURN) {
					// Dives back past, towards the ship
					e->state = 2;
					e->vx = RR_SCROLL - RR_PX(6);
					e->vy = rr_player_py() > (e->y >> RR_FP) ? RR_PX(3) / 2 : -RR_PX(3) / 2;
				}
			}
			else {
				e->x += e->vx;
				e->y += e->vy;
			}
			e->y = game_clamp(e->y, RR_PX(RR_PLAY_TOP), RR_PX(RR_PLAY_BOTTOM - 48));
			break;
		}
		default:
			break;
	}
}

void rr_enemy_update(uint32_t frame) {
	_frame = frame;
	const int32_t cam = rr_world_camera() >> RR_FP;
	for (uint32_t i = 0; i < RR_ENEMIES; ++i) {
		RrEnemy_t* e = &_enemies[i];
		if (e->type == RR_EN_TYPES) continue;
		if (e->flash) --e->flash;
		update_enemy(e);
		const int32_t x = e->x >> RR_FP, y = e->y >> RR_FP;
		const int32_t w = width_of(e), h = height_of(e);
		// Wasps come from behind, so only go once they have turned
		const int32_t off_left = e->type == RR_EN_WASP && e->state == 0 ? -400 : -RR_OFF_LEFT;
		if (x - cam < off_left || x - cam > GAME_SCREEN_W + 400 || y < RR_PLAY_TOP - 60 || y > RR_PLAY_BOTTOM + 40) {
			drop(e);
			continue;
		}
		// Fliers that turned back, once off the right
		if (e->type == RR_EN_DART && e->state && x - cam > GAME_SCREEN_W) {
			drop(e);
			continue;
		}
		if (e->type == RR_EN_CAPSULE) {
			if (rr_player_touch(x, y, w, h)) {
				rr_player_add_capsule();
				e->type = RR_EN_TYPES;
			}
			continue;
		}
		if (rr_player_hit(x + 4, y + 4, w - 8, h - 8)) {
			kill(e, false);
			continue;
		}
		// The orb wears down whatever runs into it, see rr_player
	}

	for (uint32_t i = 0; i < RR_BULLETS; ++i) {
		RrBullet_t* b = &_bullets[i];
		if (!b->active) continue;
		b->x += b->vx;
		b->y += b->vy;
		const int32_t x = b->x >> RR_FP, y = b->y >> RR_FP;
		const int32_t cx = x + RR_BULLET_SIZE / 2, cy = y + RR_BULLET_SIZE / 2;
		if (x - cam < -RR_BULLET_SIZE || x - cam >= GAME_SCREEN_W || y < RR_PLAY_TOP || y >= RR_PLAY_BOTTOM - RR_BULLET_SIZE) {
			b->active = false;
			continue;
		}
		if (cy >= rr_world_floor(cx) || cy <= rr_world_ceil(cx)) {
			b->active = false;
			continue;
		}
		if (rr_player_orb(x, y, RR_BULLET_SIZE, RR_BULLET_SIZE)) {
			game_fx_explode(b->x, b->y, GAME_PINK, 4, RR_PX(1));
			b->active = false;
			continue;
		}
		if (rr_player_hit(x + 1, y + 1, RR_BULLET_SIZE - 2, RR_BULLET_SIZE - 2)) b->active = false;
	}
}

void rr_enemy_draw(uint32_t frame) {
	for (uint32_t i = 0; i < RR_ENEMIES; ++i) {
		const RrEnemy_t* e = &_enemies[i];
		if (e->type == RR_EN_TYPES) continue;
		const int32_t sx = rr_world_sx(e->x), sy = e->y >> RR_FP;
		const RrArt_t* a = art_of(e, frame + i * 3);
		if (sx <= -rr_art_w(a) || sx >= GAME_SCREEN_W) continue;
		if (e->flash) rr_art_draw_flash(a, sx, sy, (e->flash & 2) ? RR_RGB(255, 255, 255) : RR_RGB(255, 64, 64));
		else rr_art_draw(a, sx, sy);
		if (e->type == RR_EN_ROCKET && e->state) {
			rr_art_draw(&rr_gfx.rocket_flame[(frame >> 1) & 1], sx, sy + rr_art_h(a) - 2);
		}
	}
}

void rr_enemy_draw_bullets(uint32_t frame) {
	for (uint32_t i = 0; i < RR_BULLETS; ++i) {
		const RrBullet_t* b = &_bullets[i];
		if (!b->active) continue;
		rr_art_draw(&rr_gfx.bullet[((frame >> 2) + i) & 1], rr_world_sx(b->x), b->y >> RR_FP);
	}
}

// ----------------------------------------------------------------------------
// For the demo

bool rr_enemy_target(int32_t px, int32_t py, int32_t* tx, int32_t* ty) {
	int32_t best = 0x7fffffff;
	bool found = false;
	for (uint32_t i = 0; i < RR_ENEMIES; ++i) {
		const RrEnemy_t* e = &_enemies[i];
		if (e->type >= RR_EN_CAPSULE || _info[e->type].ground) continue;
		const int32_t cx = (e->x >> RR_FP) + width_of(e) / 2;
		const int32_t cy = (e->y >> RR_FP) + height_of(e) / 2;
		const int32_t d = cx - px;
		if (d < 40 || d > 560) continue;
		const int32_t score = d + game_abs(cy - py) * 2;
		if (score < best) {
			best = score;
			*tx = cx;
			*ty = cy;
			found = true;
		}
	}
	return found;
}

bool rr_enemy_ground_ahead(int32_t px, int32_t range, int32_t* tx, int32_t* ty) {
	int32_t best = range + 1;
	bool found = false;
	for (uint32_t i = 0; i < RR_ENEMIES; ++i) {
		const RrEnemy_t* e = &_enemies[i];
		if (e->type >= RR_EN_CAPSULE || !_info[e->type].ground || e->type == RR_EN_TURRET_C) continue;
		if (e->type == RR_EN_ROCKET && e->state) continue;
		const int32_t cx = (e->x >> RR_FP) + width_of(e) / 2;
		const int32_t d = cx - px;
		if (d < 0 || d >= best) continue;
		best = d;
		*tx = cx;
		*ty = e->y >> RR_FP;
		found = true;
	}
	return found;
}

bool rr_enemy_danger_near(int32_t px, int32_t py, int32_t r, int32_t* bx, int32_t* by) {
	for (uint32_t i = 0; i < RR_BULLETS; ++i) {
		const RrBullet_t* b = &_bullets[i];
		if (!b->active) continue;
		const int32_t x = b->x >> RR_FP, y = b->y >> RR_FP;
		if (game_abs(x - px) < r && game_abs(y - py) < r) {
			*bx = x;
			*by = y;
			return true;
		}
	}
	// Fliers and rockets about to run into it
	for (uint32_t i = 0; i < RR_ENEMIES; ++i) {
		const RrEnemy_t* e = &_enemies[i];
		if (e->type >= RR_EN_CAPSULE || e->type == RR_EN_FUEL || e->type == RR_EN_TURRET || e->type == RR_EN_TURRET_C) continue;
		if (e->type == RR_EN_ROCKET && !e->state) continue;
		const int32_t x = (e->x >> RR_FP) + width_of(e) / 2, y = (e->y >> RR_FP) + height_of(e) / 2;
		if (x - px > -40 && x - px < r + 60 && game_abs(y - py) < 40) {
			*bx = x;
			*by = y;
			return true;
		}
	}
	return false;
}

bool rr_enemy_capsule(int32_t* tx, int32_t* ty) {
	for (uint32_t i = 0; i < RR_ENEMIES; ++i) {
		const RrEnemy_t* e = &_enemies[i];
		if (e->type != RR_EN_CAPSULE) continue;
		*tx = (e->x >> RR_FP) + 10;
		*ty = (e->y >> RR_FP) + 7;
		return true;
	}
	return false;
}

uint32_t rr_enemy_count(void) {
	uint32_t n = 0;
	for (uint32_t i = 0; i < RR_ENEMIES; ++i) if (_enemies[i].type != RR_EN_TYPES) ++n;
	return n;
}

uint32_t rr_enemy_fliers(void) {
	uint32_t n = 0;
	for (uint32_t i = 0; i < RR_ENEMIES; ++i) {
		const RrEnemy_t* e = &_enemies[i];
		if (e->type < RR_EN_CAPSULE && !_info[e->type].ground) ++n;
	}
	return n;
}

// ----------------------------------------------------------------------------
// The roll call

const char* rr_enemy_name(uint32_t type) {
	return _info[type < RR_EN_TYPES ? type : 0].name;
}

uint32_t rr_enemy_score(uint32_t type) {
	return _info[type < RR_EN_TYPES ? type : 0].score;
}

int32_t rr_enemy_width(uint32_t type) {
	const RrEnemy_t e = { .type = type };
	return width_of(&e);
}

int32_t rr_enemy_height(uint32_t type) {
	const RrEnemy_t e = { .type = type };
	return height_of(&e);
}

void rr_enemy_draw_icon(uint32_t type, int32_t sx, int32_t sy, uint32_t frame) {
	const RrEnemy_t e = { .type = type, .phase = 1 };
	rr_art_draw(art_of(&e, frame), sx, sy);
}
