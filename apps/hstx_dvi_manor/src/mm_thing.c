#include "mm_thing.h"
#include "mm_map.h"
#include "mm_art.h"
#include "mm_camera.h"
#include "mm_witch.h"
#include "mm_hud.h"
#include "game_fx.h"
#include "game_pallet.h"
#include "game_util.h"

#define MM_THINGS 128
// How long a spell cheers a creature up for
#define MM_HAPPY_FRAMES 480
#define MM_HAPPY_FRAMES_MIN 240
// Creatures flash for this long before they get grumpy again
#define MM_HAPPY_WARN 120
#define MM_GHOST_RANGE 96
#define MM_GHOST_HOVER 22
#define MM_BAT_RANGE_X 80
#define MM_BAT_RANGE_Y 24
#define MM_SPIDER_PERIOD 320
#define MM_PUMPKIN_HOP GAME_PX(5)
#define MM_PUMPKIN_SPEED (GAME_PX(3) / 2)
#define MM_CRATE_PULL_FRAMES 16
#define MM_CRATE_PULL_SPEED GAME_PX(2)
// Crates this close to lining up with a hole drop into it
#define MM_CRATE_SNAP 4

typedef struct {
	uint8_t type;
	uint8_t mood;
	uint8_t group;
	uint8_t variant;
	bool befriended;
	int8_t dir;
	uint16_t timer;
	uint32_t phase;
	int32_t hx, hy; // home, in pixels: the middle and the floor, or the ceiling for spiders
	int32_t depth; // how far a spider can drop
	MmBody_t b;
} MmThing_t;

static MmThing_t _things[MM_THINGS];
static uint32_t _count = 0;
static uint32_t _kittens = 0;
static uint32_t _kitten_mask = 0;
static uint32_t _kitten_coat = 0;
static uint32_t _sweets = 0;
static uint32_t _cheered = 0;
static hstx_dvi_pixel_t _thread[1];
static uint32_t _happy_frames = MM_HAPPY_FRAMES;

void mm_thing_reset(uint32_t night) {
	const uint32_t less = (night - 1) * 60;
	_happy_frames = less < MM_HAPPY_FRAMES - MM_HAPPY_FRAMES_MIN ? MM_HAPPY_FRAMES - less : MM_HAPPY_FRAMES_MIN;
	_count = 0;
	_kittens = 0;
	_kitten_mask = 0;
	_kitten_coat = 0;
	_sweets = 0;
	_cheered = 0;
	_thread[0] = HSTX_DVI_PIXEL_RGB(160, 160, 192);
}

static bool is_creature(uint32_t type) {
	return type >= MM_T_GHOST && type <= MM_T_PUMPKIN;
}

static bool is_happy(const MmThing_t* t) {
	return t->mood == MM_MOOD_HAPPY;
}

// Whether a happy creature can be stood on
static bool is_step(const MmThing_t* t) {
	return is_happy(t) && (t->type == MM_T_GHOST || t->type == MM_T_PUMPKIN);
}

static int32_t px(int32_t v) {
	return v >> GAME_FP;
}

static void box(const MmThing_t* t, int32_t* x0, int32_t* y0, int32_t* x1, int32_t* y1) {
	*x0 = px(t->b.x) - t->b.hw;
	*x1 = px(t->b.x) + t->b.hw;
	*y1 = px(t->b.y);
	*y0 = *y1 - t->b.h;
}

static bool overlaps(const MmThing_t* t, int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
	int32_t a0, b0, a1, b1;
	box(t, &a0, &b0, &a1, &b1);
	return a0 < x1 && x0 < a1 && b0 < y1 && y0 < b1;
}

static void hazel_box(int32_t* x0, int32_t* y0, int32_t* x1, int32_t* y1) {
	mm_witch_box(x0, y0, x1, y1);
}

void mm_thing_spawn(char ch, int32_t cx, int32_t cy, uint32_t group) {
	if (_count >= MM_THINGS) return;
	MmThing_t* t = &_things[_count];
	*t = (MmThing_t){ 0 };
	t->hx = cx * MM_TILE + MM_TILE / 2;
	t->hy = (cy + 1) * MM_TILE;
	t->dir = 1;
	t->phase = (uint32_t)(cx * 37 + cy * 11);
	t->group = group;
	switch (ch) {
	case 'G': t->type = MM_T_GHOST; t->b.hw = 20; t->b.h = 36; break;
	case 'V': t->type = MM_T_BAT; t->b.hw = 18; t->b.h = 20; break;
	case 'S': {
		t->type = MM_T_SPIDER;
		t->b.hw = 12;
		t->b.h = 20;
		// Hangs from the top of its tile, and drops nearly to whatever is below
		t->hy = cy * MM_TILE;
		int32_t y = cy;
		while (y < MM_MAP_H - 1 && !mm_map_solid(cx, y + 1) && !mm_map_platform(cx, y + 1)) ++y;
		t->depth = (y + 1) * MM_TILE - t->hy - 52;
		if (t->depth > 6 * MM_TILE) t->depth = 6 * MM_TILE;
		if (t->depth < MM_TILE) t->depth = MM_TILE;
		break;
	}
	case 'J': t->type = MM_T_PUMPKIN; t->b.hw = 14; t->b.h = 28; break;
	case 'K': t->type = MM_T_KITTEN; t->b.hw = 12; t->b.h = 28; t->variant = _kitten_coat++ % MM_KITTEN_COATS; break;
	case '*': t->type = MM_T_SWEET; t->b.hw = 8; t->b.h = 16; t->variant = (uint8_t)((cx + cy) % MM_SWEET_COLOURS); break;
	case '+': t->type = MM_T_HEART; t->b.hw = 8; t->b.h = 16; break;
	case 'k': t->type = MM_T_KEY; t->b.hw = 14; t->b.h = 16; break;
	case 'F': t->type = MM_T_FEATHER; t->b.hw = 12; t->b.h = 28; break;
	case 'X': t->type = MM_T_CRATE; t->b.hw = 16; t->b.h = 32; break;
	default: return;
	}
	t->b.x = GAME_PX(t->hx);
	t->b.y = GAME_PX(t->hy);
	if (t->type == MM_T_GHOST) t->b.y = GAME_PX(t->hy - MM_GHOST_HOVER);
	++_count;
}

bool mm_thing_crate_box(int32_t x0, int32_t y0, int32_t x1, int32_t y1, const void* self) {
	for (uint32_t i = 0; i < _count; ++i) {
		const MmThing_t* t = &_things[i];
		if (t->type == MM_T_CRATE && t != self && overlaps(t, x0, y0, x1, y1)) return true;
	}
	return false;
}

bool mm_thing_top_at(int32_t x0, int32_t x1, int32_t y, const void* self) {
	for (uint32_t i = 0; i < _count; ++i) {
		const MmThing_t* t = &_things[i];
		if (t == self || !is_step(t)) continue;
		int32_t a0, b0, a1, b1;
		box(t, &a0, &b0, &a1, &b1);
		if (b0 == y && a0 < x1 && x0 < a1) return true;
	}
	return false;
}

// Lines a crate up with a hole it is nearly over, so it drops in
static void crate_settle(MmThing_t* t) {
	if (!t->b.ground) return;
	const int32_t x0 = px(t->b.x) - MM_TILE / 2;
	const int32_t cx = (x0 + MM_TILE / 2) >> MM_TILE_SHIFT;
	const int32_t off = x0 - cx * MM_TILE;
	const int32_t below = px(t->b.y) >> MM_TILE_SHIFT;
	if (off == 0 || game_abs(off) > MM_CRATE_SNAP) return;
	if (mm_map_solid(cx, below) || mm_map_platform(cx, below)) return;
	t->b.x = GAME_PX(cx * MM_TILE + MM_TILE / 2);
	t->b.ground = false;
	t->timer = 0;
}

bool mm_thing_push(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t dx) {
	for (uint32_t i = 0; i < _count; ++i) {
		MmThing_t* t = &_things[i];
		if (t->type != MM_T_CRATE || !overlaps(t, x0, y0, x1, y1)) continue;
		if (!t->b.ground || t->timer) return false;
		t->b.vx = GAME_PX(dx);
		t->b.vy = 0;
		mm_body_move(&t->b, false, t);
		t->b.vx = 0;
		crate_settle(t);
		return !t->b.wall;
	}
	return false;
}

bool mm_thing_crate_in(int32_t cx, int32_t cy) {
	for (uint32_t i = 0; i < _count; ++i) {
		const MmThing_t* t = &_things[i];
		if (t->type != MM_T_CRATE) continue;
		if (px(t->b.x) >> MM_TILE_SHIFT == cx && (px(t->b.y) - MM_TILE / 2) >> MM_TILE_SHIFT == cy) return true;
	}
	return false;
}

static void cheer(MmThing_t* t) {
	static const char* const says[] = { "", "HEE HEE!", "YAWN...", "BYE BYE!", "YAY!" };
	const bool was_happy = is_happy(t);
	t->mood = MM_MOOD_HAPPY;
	t->timer = _happy_frames;
	if (was_happy) return;
	int32_t x0, y0, x1, y1;
	box(t, &x0, &y0, &x1, &y1);
	mm_hud_say(px(t->b.x), y0 - 4, says[t->type], GAME_PINK);
	game_fx_explode(t->b.x, GAME_PX((y0 + y1) / 2), GAME_PINK, 12, GAME_PX(2));
	mm_hud_score_at(px(t->b.x), y0, t->befriended ? 20 : 200);
	if (!t->befriended) ++_cheered;
	t->befriended = true;
	t->b.vx = 0;
}

bool mm_thing_cast(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
	for (uint32_t i = 0; i < _count; ++i) {
		MmThing_t* t = &_things[i];
		if (!overlaps(t, x0, y0, x1, y1)) continue;
		if (is_creature(t->type)) {
			cheer(t);
			return true;
		}
		if (t->type == MM_T_CRATE) {
			// Pulled a tile towards Hazel
			t->dir = mm_witch_x() < px(t->b.x) ? -1 : 1;
			t->timer = MM_CRATE_PULL_FRAMES;
			game_fx_explode(t->b.x, t->b.y - GAME_PX(16), GAME_CYAN, 8, GAME_PX(2));
			return true;
		}
	}
	return false;
}

static void collect(MmThing_t* t) {
	const int32_t x = px(t->b.x), y = px(t->b.y) - t->b.h;
	switch (t->type) {
	case MM_T_KITTEN:
		++_kittens;
		_kitten_mask |= 1u << t->variant;
		mm_hud_say(x, y - 8, "MEW!", GAME_PINK);
		mm_hud_score_at(x, y, 1000);
		game_fx_explode(t->b.x, GAME_PX(y + 16), GAME_PINK, 20, GAME_PX(3));
		break;
	case MM_T_SWEET:
		++_sweets;
		mm_hud_score_at(x, y, 50);
		game_fx_explode(t->b.x, GAME_PX(y + 8), GAME_COLOUR_RANDOM, 6, GAME_PX(2));
		break;
	case MM_T_HEART:
		if (mm_witch_heal()) mm_hud_say(x, y - 8, "HEART", GAME_PINK);
		else mm_hud_score_at(x, y, 200);
		game_fx_explode(t->b.x, GAME_PX(y + 8), GAME_PINK, 8, GAME_PX(2));
		break;
	case MM_T_KEY:
		mm_witch_add_key(t->group);
		mm_hud_say(x, y - 8, "A KEY!", GAME_YELLOW);
		mm_hud_score_at(x, y, 500);
		game_fx_explode(t->b.x, GAME_PX(y + 8), GAME_YELLOW, 12, GAME_PX(2));
		break;
	case MM_T_FEATHER:
		mm_witch_learn_float();
		mm_hud_say(x, y - 16, "FLOAT SPELL!", GAME_CYAN);
		mm_hud_say(x, y - 4, "HOLD JUMP TO FLOAT", GAME_WHITE);
		mm_hud_score_at(x, y, 1000);
		game_fx_explode(t->b.x, GAME_PX(y + 16), GAME_CYAN, 20, GAME_PX(3));
		break;
	default:
		return;
	}
	t->type = MM_T_NONE;
}

void mm_thing_touch(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
	for (uint32_t i = 0; i < _count; ++i) {
		MmThing_t* t = &_things[i];
		if (t->type == MM_T_NONE || t->type == MM_T_CRATE || !overlaps(t, x0, y0, x1, y1)) continue;
		if (!is_creature(t->type)) collect(t);
		else if (!is_happy(t)) mm_witch_hurt(px(t->b.x));
	}
}

// Happy creatures wait for Hazel to get off before getting grumpy again
static void calm_down(MmThing_t* t) {
	if (!is_happy(t)) return;
	if (t->timer > 1) {
		--t->timer;
		return;
	}
	int32_t x0, y0, x1, y1, h0, k0, h1, k1;
	box(t, &x0, &y0, &x1, &y1);
	hazel_box(&h0, &k0, &h1, &k1);
	if (mm_witch_standing_on(x0, x1, y0) || overlaps(t, h0, k0, h1, k1)) return;
	t->mood = MM_MOOD_GRUMPY;
	t->timer = 0;
}

static void update_ghost(MmThing_t* t) {
	if (is_happy(t)) return;
	const int32_t s = game_sin(t->phase >> 1);
	const int32_t x = t->hx + s * MM_GHOST_RANGE / 127;
	const int32_t bob = game_sin(t->phase << 1) * 4 / 127;
	const int32_t nx = GAME_PX(x);
	if (nx != t->b.x) t->dir = nx > t->b.x ? 1 : -1;
	t->b.x = nx;
	t->b.y = GAME_PX(t->hy - MM_GHOST_HOVER + bob);
	++t->phase;
}

static void update_bat(MmThing_t* t) {
	if (is_happy(t)) return;
	const int32_t x = t->hx + game_sin(t->phase) * MM_BAT_RANGE_X / 127;
	const int32_t y = t->hy + game_sin(t->phase << 1) * MM_BAT_RANGE_Y / 127;
	const int32_t nx = GAME_PX(x);
	if (nx != t->b.x) t->dir = nx > t->b.x ? 1 : -1;
	t->b.x = nx;
	t->b.y = GAME_PX(y);
	++t->phase;
}

static void update_spider(MmThing_t* t) {
	int32_t drop;
	if (is_happy(t)) {
		// Climbs back up out of the way
		drop = px(t->b.y) - t->b.h - t->hy - 3;
		if (drop < 0) drop = 0;
	}
	else {
		++t->phase;
		const int32_t c = game_sin((t->phase << 8) / MM_SPIDER_PERIOD + 64);
		drop = t->depth * (127 - c) / 254;
	}
	t->b.y = GAME_PX(t->hy + t->b.h + drop);
}

static void update_pumpkin(MmThing_t* t) {
	if (is_happy(t)) {
		t->b.vx = 0;
	}
	else if (t->b.ground) {
		t->b.vx = 0;
		if (++t->phase % 40 == 0) {
			// Turns round at walls and edges before hopping
			const int32_t ahead = px(t->b.x) + t->dir * (t->b.hw + MM_TILE);
			const int32_t row = px(t->b.y) >> MM_TILE_SHIFT;
			const int32_t col = ahead >> MM_TILE_SHIFT;
			if (mm_map_solid(col, row - 1) || !(mm_map_solid(col, row) || mm_map_platform(col, row))) t->dir = -t->dir;
			t->b.vx = t->dir * MM_PUMPKIN_SPEED;
			t->b.vy = -MM_PUMPKIN_HOP;
		}
	}
	t->b.vy += MM_GRAVITY;
	if (t->b.vy > MM_FALL_MAX) t->b.vy = MM_FALL_MAX;
	mm_body_move(&t->b, false, t);
	if (t->b.wall) t->dir = -t->dir;
}

static void update_crate(MmThing_t* t) {
	t->b.vx = 0;
	if (t->timer) {
		--t->timer;
		t->b.vx = t->dir * MM_CRATE_PULL_SPEED;
		// Stops short of Hazel
		int32_t h0, k0, h1, k1, x0, y0, x1, y1;
		hazel_box(&h0, &k0, &h1, &k1);
		box(t, &x0, &y0, &x1, &y1);
		const int32_t d = px(t->b.vx) + game_sign(t->dir);
		if (x0 + d < h1 && h0 < x1 + d && y0 < k1 && k0 < y1) {
			t->b.vx = 0;
			t->timer = 0;
		}
	}
	t->b.vy += MM_GRAVITY;
	if (t->b.vy > MM_FALL_MAX) t->b.vy = MM_FALL_MAX;
	mm_body_move(&t->b, false, t);
	if (t->b.wall) t->timer = 0;
	crate_settle(t);
}

void mm_thing_update(uint32_t frame) {
	for (uint32_t i = 0; i < _count; ++i) {
		MmThing_t* t = &_things[i];
		switch (t->type) {
		case MM_T_GHOST: update_ghost(t); break;
		case MM_T_BAT: update_bat(t); break;
		case MM_T_SPIDER: update_spider(t); break;
		case MM_T_PUMPKIN: update_pumpkin(t); break;
		case MM_T_CRATE: update_crate(t); break;
		default: ++t->phase; break;
		}
		if (is_creature(t->type)) calm_down(t);
		// Sleepy bats snore
		if (t->type == MM_T_BAT && is_happy(t) && (frame & 63) == (i & 63)) {
			mm_hud_say(px(t->b.x) + 12, px(t->b.y) - 24, "Z", GAME_CYAN);
		}
	}
}

// Happy creatures flash grumpy when they are about to stop being happy
static uint32_t mood_shown(const MmThing_t* t, uint32_t frame) {
	if (is_happy(t) && t->timer <= MM_HAPPY_WARN && t->timer > 1 && (frame & 8)) return MM_MOOD_GRUMPY;
	return t->mood;
}

static void draw_thing(const MmThing_t* t, uint32_t frame) {
	const int32_t sx = mm_screen_x(px(t->b.x));
	const int32_t sy = mm_screen_y(px(t->b.y));
	// Nothing is bigger than 64 pixels, so this is enough to skip what is off screen
	if (sx < -64 || sx > GAME_SCREEN_W + 64 || sy < 0 || sy > GAME_SCREEN_H + 96) return;
	const uint32_t flip = t->dir < 0 ? MM_GFX_FLIP : 0;
	const int32_t bob = game_sin((frame << 2) + t->phase * 8) * 3 / 127;
	switch (t->type) {
	case MM_T_GHOST: {
		const uint32_t m = mood_shown(t, frame);
		const uint32_t f = (frame >> 4) & 1;
		// Grumpy ghosts are see through, happy ones solid enough to stand on
		mm_gfx_draw(&mm_art_ghost[m][f], mm_pal_ghost[m], sx - 24, sy - t->b.h - 4, 2,
			flip | (m == MM_MOOD_GRUMPY ? MM_GFX_GLASS : 0));
		break;
	}
	case MM_T_BAT: {
		const uint32_t m = mood_shown(t, frame);
		mm_gfx_draw(&mm_art_bat[m][(frame >> 3) & 1], mm_pal_bat, sx - 24, sy - t->b.h - 6, 2, flip);
		break;
	}
	case MM_T_SPIDER: {
		const int32_t top = mm_screen_y(t->hy);
		const int32_t len = sy - t->b.h - 2 - top;
		if (len > 0) mm_gfx_rect(sx - 1, top, 2, len, _thread);
		const uint32_t m = mood_shown(t, frame);
		mm_gfx_draw(&mm_art_spider[m][(frame >> 4) & 1], mm_pal_spider, sx - 16, sy - t->b.h - 4, 2, 0);
		break;
	}
	case MM_T_PUMPKIN: {
		const uint32_t m = mood_shown(t, frame);
		// Squashed on the ground, stretched in the air
		const uint32_t f = t->b.ground ? 1 : 0;
		mm_gfx_draw(&mm_art_pumpkin[m][f], mm_pal_pumpkin[m], sx - 16, sy - 32 - (f ? 0 : 2), 2, flip);
		break;
	}
	case MM_T_KITTEN: {
		const uint32_t f = ((t->phase >> 5) % 5) == 0 ? 1 : 0;
		mm_gfx_draw(&mm_art_kitten[f], mm_pal_kitten[t->variant], sx - 16, sy - 32, 2, 0);
		break;
	}
	case MM_T_SWEET:
		mm_gfx_draw(&mm_art_sweet, mm_pal_sweet[t->variant], sx - 8, sy - 16 + bob, 2, 0);
		break;
	case MM_T_HEART:
		mm_gfx_draw(&mm_art_heart, mm_pal_heart, sx - 8, sy - 16 + bob, 2, 0);
		break;
	case MM_T_KEY:
		mm_gfx_draw(&mm_art_key, mm_pal_key, sx - 16, sy - 16 + bob, 2, 0);
		break;
	case MM_T_FEATHER:
		mm_gfx_draw(&mm_art_feather, mm_pal_feather, sx - 16, sy - 32 + bob, 2, 0);
		if ((frame & 15) == 0) game_fx_explode(t->b.x, t->b.y - GAME_PX(16), GAME_CYAN, 3, GAME_PX(1));
		break;
	case MM_T_CRATE:
		mm_gfx_draw(&mm_art_crate, mm_pal_crate, sx - 16, sy - 32, 2, 0);
		break;
	default:
		break;
	}
}

void mm_thing_draw(uint32_t frame) {
	// Crates and things to pick up first, so creatures go in front of them
	for (uint32_t i = 0; i < _count; ++i) {
		if (!is_creature(_things[i].type)) draw_thing(&_things[i], frame);
	}
	for (uint32_t i = 0; i < _count; ++i) {
		if (is_creature(_things[i].type)) draw_thing(&_things[i], frame);
	}
}

uint32_t mm_thing_kittens_found(void) {
	return _kittens;
}

uint32_t mm_thing_sweets_found(void) {
	return _sweets;
}

uint32_t mm_thing_cheered(void) {
	return _cheered;
}

uint32_t mm_thing_kitten_mask(void) {
	return _kitten_mask;
}

bool mm_thing_find(uint32_t type, int32_t near_x, int32_t near_y, int32_t* x, int32_t* y, bool* happy) {
	int32_t best = -1;
	for (uint32_t i = 0; i < _count; ++i) {
		const MmThing_t* t = &_things[i];
		if (t->type != type) continue;
		const int32_t d = game_abs(px(t->b.x) - near_x) + game_abs(px(t->b.y) - near_y);
		if (best >= 0 && d >= best) continue;
		best = d;
		*x = px(t->b.x);
		*y = px(t->b.y);
		*happy = is_happy(t);
	}
	return best >= 0;
}
