#include "ww_demo.h"
#include "ww_input.h"
#include "ww_world.h"
#include "ww_witch.h"
#include "ww_animal.h"
#include "ww_items.h"
#include "ww_hud.h"
#include "game_util.h"

// Lines up a spell to within this of the middle of an animal
#define WW_DEMO_AIM 9
#define WW_DEMO_RANGE 240
// Stands about this far off to cast
#define WW_DEMO_STAND 90
#define WW_DEMO_DANGER 170
#define WW_DEMO_TIPTOE 70
#define WW_DEMO_STUCK 50

static uint32_t _room = WW_NO_ROOM;
static int32_t _came_from = -1;
static int32_t _exit = -1;
static int32_t _last_x = 0, _last_y = 0;
static uint32_t _still = 0;
static uint32_t _jiggle = 0;
static uint8_t _jiggle_dir = 0;

void ww_demo_reset(void) {
	_room = WW_NO_ROOM;
	_came_from = -1;
	_exit = -1;
	_still = 0;
	_jiggle = 0;
}

// Off to the nearest room not yet visited, otherwise the least visited one
// next door, not back the way it came if it can help it
static int32_t choose_exit(uint32_t room) {
	if (ww_animal_following() && ww_animal_owl_room() != WW_NO_ROOM) {
		const int32_t d = ww_world_route(room, ww_animal_owl_room());
		if (d >= 0) return d;
	}
	const int32_t explore = ww_world_route_unvisited(room);
	if (explore >= 0) return explore;
	int32_t best = -1;
	uint32_t best_score = 0xffffffff;
	for (uint32_t d = 0; d < 4; ++d) {
		const uint32_t next = ww_world_next(room, d);
		if (next == WW_NO_ROOM) continue;
		uint32_t score = ww_world_visits(next) * 8 + (game_rand() & 7);
		if ((int32_t)d == _came_from) score += 1000;
		if (score < best_score) {
			best_score = score;
			best = d;
		}
	}
	return best;
}

static uint8_t dir_bits(int32_t dx, int32_t dy) {
	return (dx > 0 ? WW_INP_BIT_R : dx < 0 ? WW_INP_BIT_L : 0) | (dy > 0 ? WW_INP_BIT_D : dy < 0 ? WW_INP_BIT_U : 0);
}

// Steers towards a point, tile by tile round anything in the way
static uint8_t go_to(int32_t x, int32_t y) {
	const int32_t wx = ww_witch_x(), wy = ww_witch_y();
	const int32_t fc = wx >> WW_TILE_SHIFT, fr = (wy - 4) >> WW_TILE_SHIFT;
	const int32_t tc = x >> WW_TILE_SHIFT, tr = (y - 4) >> WW_TILE_SHIFT;
	int32_t px = x, py = y;
	if (fc != tc || fr != tr) {
		int32_t nc, nr;
		if (!ww_world_step(fc, fr, tc, tr, &nc, &nr)) return 0;
		px = nc * WW_TILE + WW_TILE / 2;
		py = nr * WW_TILE + WW_TILE - 10;
	}
	const int32_t dx = px - wx, dy = py - wy;
	return dir_bits(game_abs(dx) > 2 ? dx : 0, game_abs(dy) > 2 ? dy : 0);
}

// Where it leaves by, and which way to keep walking once there
static uint8_t go_out(int32_t dir) {
	dir &= 3;
	static const int16_t gap[4][2] = {
		{ (WW_GAP_C0 + 2) * WW_TILE, 12 },
		{ WW_ROOM_W - 4, (WW_GAP_R0 + 2) * WW_TILE },
		{ (WW_GAP_C0 + 2) * WW_TILE, WW_ROOM_H - 2 },
		{ 4, (WW_GAP_R0 + 2) * WW_TILE },
	};
	static const uint8_t push[4] = { WW_INP_BIT_U, WW_INP_BIT_R, WW_INP_BIT_D, WW_INP_BIT_L };
	const int32_t x = gap[dir][0], y = gap[dir][1];
	const int32_t wx = ww_witch_x(), wy = ww_witch_y();
	if (game_abs(x - wx) < 40 && game_abs(y - wy) < 40) {
		const int32_t dx = dir == WW_DIR_N || dir == WW_DIR_S ? x - wx : 0;
		const int32_t dy = dir == WW_DIR_E || dir == WW_DIR_W ? y - wy : 0;
		return push[dir] | dir_bits(game_abs(dx) > 2 ? dx : 0, game_abs(dy) > 2 ? dy : 0);
	}
	return go_to(x, y);
}

// The way to face to hit x, y with a spell, or -1 if it is not lined up
static int32_t aim(int32_t x, int32_t y) {
	const int32_t wx = ww_witch_x(), wy = ww_witch_y() - 16;
	for (uint32_t f = 0; f < 8; ++f) {
		int32_t dx, dy;
		ww_witch_facing_step(f, &dx, &dy);
		const int32_t vx = x - wx, vy = y - wy;
		const int32_t along = vx * dx + vy * dy;
		int32_t across = vx * dy - vy * dx;
		if (dx && dy) across = across * 181 >> 8;
		if (along <= 0 || along > WW_DEMO_RANGE || game_abs(across) > WW_DEMO_AIM) continue;
		if (!ww_world_line_clear(wx, wy, x, y)) continue;
		return f;
	}
	return -1;
}

static uint8_t cast_at(int32_t f, uint32_t frame) {
	int32_t dx, dy;
	ww_witch_facing_step(f, &dx, &dy);
	int32_t bx, by;
	// Turns to face it on tiptoe, so barely moving, then casts
	uint8_t input = WW_INP_BIT_B;
	if ((int32_t)ww_witch_facing() != f) return input | dir_bits(dx, dy);
	if (!ww_witch_bolt(&bx, &by) && (frame & 1)) input |= WW_INP_BIT_F;
	return input;
}

// Gets in line with something to cast at it
static uint8_t engage(const WwAnimal_t* a, uint32_t frame) {
	const int32_t ax = a->x >> GAME_FP, ay = (a->y >> GAME_FP) - (int32_t)(ww_animal_height(a->type) >> 1);
	const int32_t f = aim(ax, ay);
	if (f >= 0) return cast_at(f, frame);
	const int32_t wx = ww_witch_x(), wy = ww_witch_y();
	const int32_t dx = ax - wx, dy = ay + 16 - wy;
	// Too close, so back off
	if (game_abs(dx) + game_abs(dy) < 40 && a->mood == WW_MOOD_GRUMPY) return dir_bits(-game_sign(dx), -game_sign(dy));
	// Stands off to one side or the other, whichever is nearer
	int32_t best_x = ax, best_y = ay + 16, best = 0x7fffffff;
	static const int8_t spots[4][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
	for (uint32_t i = 0; i < 4; ++i) {
		const int32_t sx = ax + spots[i][0] * WW_DEMO_STAND;
		const int32_t sy = ay + 16 + spots[i][1] * WW_DEMO_STAND;
		if (sx < 16 || sy < 24 || sx >= WW_ROOM_W - 16 || sy >= WW_ROOM_H - 4) continue;
		if (!ww_world_box_free(sx - 8, sy - 8, sx + 8, sy - 1)) continue;
		const int32_t d = game_abs(sx - wx) + game_abs(sy - wy);
		if (d < best) {
			best = d;
			best_x = sx;
			best_y = sy;
		}
	}
	return go_to(best_x, best_y);
}

static int32_t distance(const WwAnimal_t* a) {
	return game_abs((a->x >> GAME_FP) - ww_witch_x()) + game_abs((a->y >> GAME_FP) - ww_witch_y());
}

uint8_t ww_demo_input(uint32_t frame) {
	if (!ww_witch_alive()) return 0;
	const uint32_t room = ww_world_room();
	const int32_t wx = ww_witch_x(), wy = ww_witch_y();
	if (room != _room) {
		// Came in from the opposite side to the way it left the last room
		if (_room != WW_NO_ROOM && _exit >= 0) _came_from = (_exit + 2) & 3;
		_room = room;
		_exit = choose_exit(room);
	}

	// Gets out of corners it has wedged itself into
	if (wx == _last_x && wy == _last_y) ++_still;
	else _still = 0;
	_last_x = wx;
	_last_y = wy;
	if (_still > WW_DEMO_STUCK) {
		_jiggle = 20;
		_jiggle_dir = dir_bits((game_rand() % 3) - 1, (game_rand() % 3) - 1);
		_still = 0;
	}
	if (_jiggle) {
		--_jiggle;
		return _jiggle_dir;
	}

	uint8_t tiptoe = 0;
	const WwAnimal_t* grumpy = NULL;
	const WwAnimal_t* hurt = NULL;
	const WwAnimal_t* lost = NULL;
	const WwAnimal_t* wolf = NULL;
	const WwAnimal_t* mother = NULL;
	int32_t grumpy_d = 0x7fffffff, hurt_d = 0x7fffffff;
	for (uint32_t i = 0; i < ww_animal_here(); ++i) {
		const WwAnimal_t* a = ww_animal_get(i);
		const int32_t d = distance(a);
		if (a->type == WW_WOLF) {
			wolf = a;
			continue;
		}
		switch (a->mood) {
			case WW_MOOD_ASLEEP:
				if (d < 100 + WW_DEMO_TIPTOE) tiptoe = WW_INP_BIT_B;
				break;
			case WW_MOOD_GRUMPY:
				if (d < grumpy_d) { grumpy = a; grumpy_d = d; }
				break;
			case WW_MOOD_HURT:
				if (d < hurt_d) { hurt = a; hurt_d = d; }
				break;
			case WW_MOOD_LOST:
				lost = a;
				break;
			case WW_MOOD_WAITING:
				mother = a;
				break;
		}
	}

	if (wolf) {
		if (ww_hud_charm_whole()) return engage(wolf, frame);
		// Holds it off if it is in line, otherwise runs for it
		const int32_t f = aim(wolf->x >> GAME_FP, (wolf->y >> GAME_FP) - 10);
		if (f >= 0 && distance(wolf) < 160 && !wolf->stun) return cast_at(f, frame);
		return go_out(_exit);
	}
	if (grumpy && grumpy_d < WW_DEMO_DANGER) return engage(grumpy, frame) | tiptoe;
	if (hurt) return engage(hurt, frame) | tiptoe;
	if (lost) return go_to(lost->x >> GAME_FP, lost->y >> GAME_FP) | tiptoe;
	if (mother && ww_animal_following()) return go_to(mother->x >> GAME_FP, (mother->y >> GAME_FP) + 24) | tiptoe;
	int32_t x, y;
	if (ww_items_piece_here(&x, &y)) return go_to(x, y) | WW_INP_BIT_B;
	uint32_t effect;
	if (ww_items_flower_here(&x, &y, &effect)) {
		if ((effect == WW_EFFECT_HEART && ww_witch_hearts() < WW_WITCH_START_HEARTS) || effect == WW_EFFECT_SPEEDY) return go_to(x, y) | tiptoe;
	}
	if (grumpy) return engage(grumpy, frame) | tiptoe;
	if (_exit < 0) return 0;
	return go_out(_exit) | tiptoe;
}
