// Copyright (c) 2024 fruit-bat.

// See rr_world.h.
//
// The land is columns RR_COL_W pixels wide, each with the top of its floor
// and the bottom of its ceiling at its left and right edges, straight between.
// Smooth land slopes from one column to the next; blocky land, buildings and
// fortress walls, steps. Columns are made a little ahead of the screen, into
// a ring. Each frame, the game's core works out the runs of colour on every
// row of the land and hills from the columns on the screen, for the renderer
// to draw; see Drawing below.

#include "rr_world.h"
#include "rr_enemy.h"
#include "game_pallet.h"
#include <string.h>

#define RR_COL_SHIFT 4
#define RR_COL_W (1 << RR_COL_SHIFT)
#define RR_RING 64
// Columns made ahead of the left of the screen
#define RR_AHEAD 46
// On the screen, part columns at either end
#define RR_VIS ((GAME_SCREEN_W >> RR_COL_SHIFT) + 1)
// The bright edge along the top of the floor and bottom of the ceiling
#define RR_RIM 6
// The rows of windows on each floor of a building, of 16
#define RR_WINDOW_ROWS 6
// No floor, or no ceiling, just out of sight
#define RR_FLOOR_NONE (RR_PLAY_BOTTOM + 16)
#define RR_CEIL_NONE (RR_PLAY_TOP - 16)
// Level ground to start on
#define RR_START_COLS 44
// The fewest columns between things on the ground; more where the land is
// blocky, as it is level nearly everywhere, and too many on the same rows
// are more than the renderer can draw
#define RR_PLACE_GAP 3
#define RR_PLACE_GAP_BLOCKY 8

// The hills behind: columns twice as wide, round again every so often
#define RR_HILL_SHIFT 5
#define RR_HILL_COLS 64

// The stars go round every 1024 pixels, in three layers at different speeds
#define RR_STAR_WRAP 1024

typedef struct {
	const char* name;
	int16_t f_lo, f_hi; // the top of the floor between these
	int16_t c_lo, c_hi; // the bottom of the ceiling between these
	int16_t gap;        // the least room between them
	uint8_t blocky;
	uint8_t windows;
	uint16_t len;       // columns
	hstx_dvi_pixel_t rim, body, dark, window;
	hstx_dvi_pixel_t hill; // 0 for no hills
} RrSectionInfo_t;

static const RrSectionInfo_t _sections[RR_SECTIONS] = {
	[RR_SEC_RIDGES] = { "RIDGES", 272, 424, RR_CEIL_NONE, RR_CEIL_NONE, 0, 0, 0, 150,
		RR_RGB(96, 255, 64), RR_RGB(160, 96, 32), RR_RGB(96, 64, 0), 0, RR_RGB(64, 32, 128) },
	[RR_SEC_CAVERNS] = { "CAVERNS", 312, 424, 64, 152, 160, 0, 0, 150,
		RR_RGB(0, 255, 224), RR_RGB(32, 96, 128), RR_RGB(0, 64, 96), 0, 0 },
	[RR_SEC_DEBRIS] = { "DEBRIS FIELD", 392, 432, RR_CEIL_NONE, RR_CEIL_NONE, 0, 0, 0, 150,
		RR_RGB(224, 224, 224), RR_RGB(128, 128, 128), RR_RGB(64, 64, 64), 0, RR_RGB(0, 32, 96) },
	[RR_SEC_CITY] = { "CITY", 240, 416, RR_CEIL_NONE, RR_CEIL_NONE, 0, 1, 1, 150,
		RR_RGB(255, 160, 0), RR_RGB(32, 32, 128), RR_RGB(32, 32, 96), RR_RGB(255, 255, 96), RR_RGB(64, 0, 96) },
	[RR_SEC_FORTRESS] = { "FORTRESS", 288, 432, 64, 160, 136, 1, 0, 150,
		RR_RGB(255, 64, 192), RR_RGB(96, 32, 128), RR_RGB(64, 0, 96), 0, 0 },
	[RR_SEC_WARSHIP] = { "WARSHIP", RR_FLOOR_NONE, RR_FLOOR_NONE, RR_CEIL_NONE, RR_CEIL_NONE, 0, 0, 0, 0,
		0, 0, 0, 0, RR_RGB(96, 0, 32) },
};

typedef struct {
	int16_t fl, fr, cl, cr;
	uint8_t section;
} RrCol_t;

// The columns on the screen, for working out the runs of the land
typedef struct {
	int32_t x0;       // the screen x of the first column, 0 or less
	int32_t c0;       // and which column it is
	int32_t floor_min;
	int32_t ceil_max;
	int16_t fl[RR_VIS], fr[RR_VIS], cl[RR_VIS], cr[RR_VIS];
	hstx_dvi_pixel_t rim[RR_VIS], body[RR_VIS], dark[RR_VIS], window[RR_VIS];
} RrTerrainView_t;

static RrCol_t _col[RR_RING];
static int32_t _cam = 0;
// The next column to make
static int32_t _gen = 0;
static uint32_t _gen_section = 0;
static int32_t _gen_left = 0;
static uint32_t _loop = 0;
static bool _boss_held = false;
static int32_t _floor = 400, _ceil = RR_CEIL_NONE;
static int32_t _f_slope = 0, _c_slope = 0;
static int32_t _f_run = 0, _c_run = 0;
static int32_t _place_wait = 0, _place_wait_c = 0;


static int16_t _hill[RR_HILL_COLS + 1];
static int16_t _hill_min;
// What the hills cover on each row, worked out once, as runs round the
// hills' width; one at the end may go on past it, round to the start
#define RR_HILL_W (RR_HILL_COLS << RR_HILL_SHIFT)
#define RR_HILL_RUNS 2400
typedef struct {
	int16_t x0, x1;
} RrHillRun_t;
static RrHillRun_t _hill_run[RR_HILL_RUNS];
static uint16_t _hill_row[RR_PLAY_BOTTOM - RR_PLAY_TOP + 1];
static uint16_t _star_x[GAME_SCREEN_H];
static uint8_t _star_layer[GAME_SCREEN_H];

// The pixels of a column w wide, from a to b, whose top is at or above y,
// the top going straight from yl at its left to yr at its right
static __force_inline void below(const int32_t yl, const int32_t yr, const int32_t y, const int32_t w, int32_t* a, int32_t* b) {
	// Most columns are wholly above or below the row, with no sums to do
	if (y >= yl && y >= yr) {
		*a = 0;
		*b = w;
	}
	else if (y < yl && y < yr) {
		*a = 0;
		*b = 0;
	}
	else if (yr < yl) {
		const int32_t d = yl - yr;
		const int32_t n = (yl - y) * 2 * w - d;
		const int32_t xs = n <= 0 ? 0 : (n + 2 * d - 1) / (2 * d);
		*a = xs > w ? w : xs;
		*b = w;
	}
	else {
		const int32_t d = yr - yl;
		const int32_t n = (y - yl) * 2 * w - d;
		const int32_t xe = n < 0 ? 0 : n / (2 * d) + 1;
		*a = 0;
		*b = xe > w ? w : xe;
	}
}

// ----------------------------------------------------------------------------
// Making the land

static int32_t pick(int32_t lo, int32_t hi, int32_t step) {
	if (hi <= lo) return lo;
	return lo + (int32_t)(game_rand() % (uint32_t)((hi - lo) / step + 1)) * step;
}

// Walks smoothly between lo and hi, sloping a run of columns at a time
static int32_t walk(int32_t v, int32_t lo, int32_t hi, int32_t* slope, int32_t* run) {
	if (v < lo) return v + 16 > lo ? lo : v + 16;
	if (v > hi) return v - 16 < hi ? hi : v - 16;
	if (*run <= 0) {
		static const int8_t slopes[8] = { -16, -16, -8, 0, 0, 8, 16, 16 };
		*slope = slopes[game_rand() & 7];
		*run = *slope ? 1 + game_rand() % 4 : 2 + game_rand() % 5;
	}
	--*run;
	const int32_t n = v + *slope;
	if (n < lo || n > hi) {
		*slope = -*slope;
		return v;
	}
	return n;
}

static void next_section(void) {
	if (_gen_section == RR_SEC_WARSHIP) {
		_gen_section = 0;
		++_loop;
	}
	else {
		++_gen_section;
	}
	_gen_left = _sections[_gen_section].len;
	_boss_held = _gen_section == RR_SEC_WARSHIP;
	_f_run = _c_run = 0;
}

static void make_column(void) {
	const RrSectionInfo_t* s = &_sections[_gen_section];
	RrCol_t* c = &_col[_gen & (RR_RING - 1)];
	const RrCol_t* p = &_col[(_gen - 1) & (RR_RING - 1)];
	c->section = _gen_section;
	if (_gen < RR_START_COLS) {
		c->fl = c->fr = _floor = 424;
		c->cl = c->cr = _ceil = RR_CEIL_NONE;
	}
	else if (s->blocky) {
		if (_f_run <= 0) {
			_floor = pick(s->f_lo, s->f_hi, 16);
			_f_run = 2 + game_rand() % 4;
		}
		--_f_run;
		if (_c_run <= 0) {
			_ceil = s->c_hi > RR_CEIL_NONE ? pick(s->c_lo, s->c_hi, 16) : RR_CEIL_NONE;
			_c_run = 2 + game_rand() % 4;
		}
		--_c_run;
		if (_floor - _ceil < s->gap) _ceil = _floor - s->gap;
		c->fl = c->fr = _floor;
		c->cl = c->cr = _ceil;
	}
	else {
		c->fl = _floor;
		c->cl = _ceil;
		_floor = walk(_floor, s->f_lo, s->f_hi, &_f_slope, &_f_run);
		_ceil = walk(_ceil, s->c_lo, s->c_hi, &_c_slope, &_c_run);
		if (_floor - _ceil < s->gap) _ceil = _floor - s->gap;
		c->fr = _floor;
		c->cr = _ceil;
	}

	// Things on the ground, where it is level for two columns
	if (_place_wait) --_place_wait;
	if (_place_wait_c) --_place_wait_c;
	if (_gen > RR_START_COLS) {
		const int32_t x = _gen << RR_COL_SHIFT;
		if (!_place_wait && c->fl == c->fr && p->fl == p->fr && p->fr == c->fl && c->fl < RR_PLAY_BOTTOM - 8) {
			if (rr_enemy_place(x, c->fl, false, _gen_section)) _place_wait = s->blocky ? RR_PLACE_GAP_BLOCKY : RR_PLACE_GAP;
		}
		if (!_place_wait_c && c->cl == c->cr && p->cl == p->cr && p->cr == c->cl && c->cl > RR_PLAY_TOP + 8) {
			if (rr_enemy_place(x, c->cl, true, _gen_section)) _place_wait_c = s->blocky ? RR_PLACE_GAP_BLOCKY : RR_PLACE_GAP;
		}
	}

	++_gen;
	if (!_boss_held && --_gen_left <= 0) next_section();
}

static void make_hill_runs(void) {
	static bool made = false;
	if (made) return;
	made = true;
	static uint8_t covered[RR_HILL_W];
	uint32_t n = 0;
	const uint32_t rows = RR_PLAY_BOTTOM - _hill_min;
	for (uint32_t row = 0; row < rows; ++row) {
		_hill_row[row] = n;
		const int32_t y = _hill_min + (int32_t)row;
		for (uint32_t c = 0; c < RR_HILL_COLS; ++c) {
			int32_t a, b;
			below(_hill[c], _hill[c + 1], y, 1 << RR_HILL_SHIFT, &a, &b);
			for (int32_t i = 0; i < (1 << RR_HILL_SHIFT); ++i) covered[(c << RR_HILL_SHIFT) + i] = i >= a && i < b;
		}
		const uint32_t first = n;
		for (int32_t x = 0; x < RR_HILL_W; ) {
			if (!covered[x]) {
				++x;
				continue;
			}
			int32_t e = x + 1;
			while (e < RR_HILL_W && covered[e]) ++e;
			if (n < RR_HILL_RUNS) {
				_hill_run[n].x0 = x;
				_hill_run[n].x1 = e;
				++n;
			}
			x = e;
		}
		// A run up to the end goes on into one from the start
		if (n - first >= 2 && _hill_run[first].x0 == 0 && _hill_run[n - 1].x1 == RR_HILL_W) {
			_hill_run[n - 1].x1 = RR_HILL_W + _hill_run[first].x1;
			for (uint32_t i = first; i + 1 < n; ++i) _hill_run[i] = _hill_run[i + 1];
			--n;
		}
	}
	_hill_row[rows] = n;
}

void rr_world_init(void) {
	_cam = 0;
	_gen = 0;
	_loop = 0;
	_gen_section = RR_SEC_RIDGES;
	_gen_left = _sections[_gen_section].len + RR_START_COLS;
	_boss_held = false;
	_floor = 424;
	_ceil = RR_CEIL_NONE;
	_f_run = _c_run = 0;
	_f_slope = _c_slope = 0;
	_place_wait = _place_wait_c = 0;
	memset(_col, 0, sizeof(_col));
	for (uint32_t i = 0; i < RR_RING; ++i) {
		_col[i].fl = _col[i].fr = RR_FLOOR_NONE;
		_col[i].cl = _col[i].cr = RR_CEIL_NONE;
	}
	while (_gen < RR_AHEAD) make_column();

	// Rolling hills behind, the same every game
	uint32_t seed = 0x1234567;
	_hill_min = RR_PLAY_BOTTOM;
	for (uint32_t i = 0; i < RR_HILL_COLS; ++i) {
		seed = seed * 1103515245u + 12345u;
		const int32_t h = 300 + (int32_t)((seed >> 16) % 7) * 16 + ((i & 7) < 4 ? 0 : 24);
		_hill[i] = h;
		if (h < _hill_min) _hill_min = h;
	}
	_hill[RR_HILL_COLS] = _hill[0];
	make_hill_runs();

	for (uint32_t y = 0; y < GAME_SCREEN_H; ++y) {
		seed = seed * 1103515245u + 12345u;
		_star_x[y] = 0xffff;
		if (((seed >> 16) & 7) < 2) {
			_star_x[y] = (seed >> 3) & (RR_STAR_WRAP - 1);
			_star_layer[y] = (seed >> 20) % 3;
		}
	}
}

void rr_world_scroll(int32_t dx) {
	_cam += dx;
	const int32_t want = ((_cam >> RR_FP) >> RR_COL_SHIFT) + RR_AHEAD;
	while (_gen < want) make_column();
}

int32_t rr_world_camera(void) {
	return _cam;
}

int32_t __not_in_flash_func(rr_world_sx)(int32_t x) {
	return (x - _cam) >> RR_FP;
}

static int32_t fx_wrap(int32_t x) {
	return x;
}

static const GameFxView_t _fx_view = { rr_world_sx, fx_wrap, RR_PLAY_TOP, RR_PLAY_BOTTOM, NULL };

const GameFxView_t* rr_world_fx_view(void) {
	return &_fx_view;
}

// The column, or NULL if it has not been made or has gone
static const RrCol_t* column(int32_t c) {
	if (c >= _gen || c < _gen - RR_RING || c < 0) return NULL;
	return &_col[c & (RR_RING - 1)];
}

int32_t rr_world_floor(int32_t px) {
	const RrCol_t* c = column(px >> RR_COL_SHIFT);
	if (!c) return RR_FLOOR_NONE;
	const int32_t f = px & (RR_COL_W - 1);
	return c->fl + (c->fr - c->fl) * (2 * f + 1) / (2 * RR_COL_W);
}

int32_t rr_world_ceil(int32_t px) {
	const RrCol_t* c = column(px >> RR_COL_SHIFT);
	if (!c) return RR_CEIL_NONE;
	const int32_t f = px & (RR_COL_W - 1);
	return c->cl + (c->cr - c->cl) * (2 * f + 1) / (2 * RR_COL_W);
}

int32_t rr_world_floor_min(int32_t px0, int32_t px1) {
	int32_t m = RR_FLOOR_NONE;
	for (int32_t x = px0; x <= px1; x += 4) {
		const int32_t f = rr_world_floor(x);
		if (f < m) m = f;
	}
	return m;
}

int32_t rr_world_ceil_max(int32_t px0, int32_t px1) {
	int32_t m = RR_CEIL_NONE;
	for (int32_t x = px0; x <= px1; x += 4) {
		const int32_t c = rr_world_ceil(x);
		if (c > m) m = c;
	}
	return m;
}

uint32_t rr_world_section(int32_t px) {
	const RrCol_t* c = column(px >> RR_COL_SHIFT);
	return c ? c->section : RR_SEC_RIDGES;
}

const char* rr_world_section_name(uint32_t section) {
	return _sections[section < RR_SECTIONS ? section : 0].name;
}

uint32_t rr_world_loop(void) {
	return _loop;
}

void rr_world_release_boss(void) {
	if (_boss_held) {
		_boss_held = false;
		next_section();
	}
}

void rr_world_jump(uint32_t section) {
	_gen_section = section < RR_SECTIONS ? section : 0;
	_gen_left = _sections[_gen_section].len;
	_boss_held = _gen_section == RR_SEC_WARSHIP;
	_f_run = _c_run = 0;
}

// ----------------------------------------------------------------------------
// Drawing
//
// The land and the hills behind it are worked out here, on the game's core,
// a frame at a time, as runs of colour for every row, side by side so no
// pixel is drawn twice. The renderer, on the other core, only draws them.
// One set is drawn into while the other is shown.

#define RR_ROWS (RR_PLAY_BOTTOM - RR_PLAY_TOP)
#define RR_POOL 6144

typedef struct {
	uint16_t start[RR_ROWS + 1];
	SpriteSpan_t span[RR_POOL];
} RrLand_t;

static RrLand_t _land[2];
static uint32_t _land_i = 0;
static RrTerrainView_t _v;

// For putting a row together
#define RR_SPANS (RR_VIS * 2 + 4)
static SpriteSpan_t _rim[RR_SPANS], _body[RR_SPANS], _win[RR_SPANS], _cut[RR_SPANS * 2], _hills[RR_SPANS * 2];
// What the floor covers on the row, which the hills go behind
static SpriteSpan_t _cover[RR_VIS + 1];

static __force_inline void add_span(SpriteSpan_t* l, uint32_t* n, int32_t x0, int32_t x1, hstx_dvi_pixel_t p) {
	if (x0 >= x1) return;
	if (*n && l[*n - 1].x1 == x0 && l[*n - 1].p == p) {
		l[*n - 1].x1 = x1;
		return;
	}
	if (*n >= RR_SPANS) return;
	l[*n].x0 = x0;
	l[*n].x1 = x1;
	l[*n].p = p;
	++*n;
}

// Adds the parts of the runs in src not in any of the runs in cut, both in
// order along the row, to dst
static uint32_t subtract(SpriteSpan_t* dst, uint32_t n, const SpriteSpan_t* src, uint32_t ns, const SpriteSpan_t* cut, uint32_t nc) {
	uint32_t k = 0;
	for (uint32_t i = 0; i < ns; ++i) {
		int32_t s = src[i].x0;
		const int32_t e = src[i].x1;
		while (k < nc && cut[k].x1 <= s) ++k;
		for (uint32_t j = k; s < e; ++j) {
			if (j >= nc || cut[j].x0 >= e) {
				add_span(dst, &n, s, e, src[i].p);
				break;
			}
			add_span(dst, &n, s, cut[j].x0, src[i].p);
			if (cut[j].x1 > s) s = cut[j].x1;
		}
	}
	return n;
}

// The rim is what is below the top less what is more than RR_RIM below it,
// one run, sharing an end with the body unless the body is empty
static __force_inline void add_land(const int32_t cx, const int32_t a1, const int32_t b1, const int32_t a2, const int32_t b2,
	const hstx_dvi_pixel_t rim, const hstx_dvi_pixel_t body, uint32_t* nr, uint32_t* nb) {
	if (a1 >= b1) return;
	if (a2 >= b2) {
		add_span(_rim, nr, cx + a1, cx + b1, rim);
		return;
	}
	if (b2 == b1) add_span(_rim, nr, cx + a1, cx + a2, rim);
	else add_span(_rim, nr, cx + b2, cx + b1, rim);
	add_span(_body, nb, cx + a2, cx + b2, body);
}

// A window in the column at cx, lit or not, for each floor of each column
static __force_inline void add_window(const int32_t cx, const int32_t c, const int32_t wy,
	const hstx_dvi_pixel_t lit, const hstx_dvi_pixel_t dark, uint32_t* nw) {
	const uint32_t h = (uint32_t)c * 7u + (uint32_t)(wy >> 4) * 13u;
	add_span(_win, nw, cx + 4, cx + 12, ((h * 2654435761u) >> 29) < 6 ? lit : dark);
}

static uint32_t append(RrLand_t* l, uint32_t n, const SpriteSpan_t* src, uint32_t ns) {
	for (uint32_t i = 0; i < ns && n < RR_POOL; ++i) l->span[n++] = src[i];
	return n;
}

// The hills on a row, a hills' width round from scroll, as runs
static uint32_t hill_row(int32_t y, uint32_t scroll, hstx_dvi_pixel_t p) {
	uint32_t n = 0;
	if (y < _hill_min) return 0;
	const uint32_t row = y - _hill_min;
	const uint32_t r0 = _hill_row[row], r1 = _hill_row[row + 1];
	// The runs a hills' width back, for the end of the one that goes on round
	// to the start, then the runs, then again a hills' width on, as the screen
	// can show the join
	for (int32_t round = -RR_HILL_W; round < 2 * RR_HILL_W; round += RR_HILL_W) {
		for (uint32_t i = r0; i < r1; ++i) {
			int32_t s = _hill_run[i].x0 - (int32_t)scroll + round;
			int32_t e = _hill_run[i].x1 - (int32_t)scroll + round;
			if (s >= GAME_SCREEN_W) return n;
			if (e <= 0) continue;
			add_span(_hills, &n, s < 0 ? 0 : s, e > GAME_SCREEN_W ? GAME_SCREEN_W : e, p);
		}
	}
	return n;
}

// One row of the land and hills, added to l from n, returning the new end
static uint32_t land_row(RrLand_t* l, uint32_t n, const RrTerrainView_t* v, int32_t y, uint32_t hill_scroll, hstx_dvi_pixel_t hill) {
	const bool do_floor = y >= v->floor_min;
	const bool do_ceil = y <= v->ceil_max;
	uint32_t nr = 0, nb = 0, nw = 0, nc = 0;
	if (do_floor || do_ceil) {
		// Darker bands, as layers of rock
		const bool strata = (y & 15) < 3;
		for (uint32_t i = 0; i < RR_VIS; ++i) {
			const int32_t cx = v->x0 + (int32_t)(i << RR_COL_SHIFT);
			const hstx_dvi_pixel_t body = strata ? v->dark[i] : v->body[i];
			int32_t a1, b1, a2, b2;
			if (do_floor) {
				const int32_t fl = v->fl[i], fr = v->fr[i];
				below(fl, fr, y, RR_COL_W, &a1, &b1);
				if (a1 < b1) {
					if (nc && _cover[nc - 1].x1 == cx + a1) _cover[nc - 1].x1 = cx + b1;
					else if (nc < RR_VIS + 1) {
						_cover[nc].x0 = cx + a1;
						_cover[nc].x1 = cx + b1;
						++nc;
					}
					below(fl, fr, y - RR_RIM, RR_COL_W, &a2, &b2);
					// Windows, in rows down a building
					if (v->window[i] && fl == fr) {
						const int32_t wy = y - fl - RR_RIM - 6;
						if (wy >= 0 && (wy & 15) < RR_WINDOW_ROWS) add_window(cx, v->c0 + (int32_t)i, wy, v->window[i], v->dark[i], &nw);
					}
					add_land(cx, a1, b1, a2, b2, v->rim[i], v->window[i] ? v->body[i] : body, &nr, &nb);
				}
			}
			if (do_ceil) {
				const int32_t cl = -v->cl[i], cr = -v->cr[i];
				below(cl, cr, -y, RR_COL_W, &a1, &b1);
				if (a1 < b1) {
					below(cl, cr, -y - RR_RIM, RR_COL_W, &a2, &b2);
					add_land(cx, a1, b1, a2, b2, v->rim[i], body, &nr, &nb);
				}
			}
		}
	}
	n = append(l, n, _rim, nr);
	if (nw) {
		// The building around the windows, not under them
		n = append(l, n, _win, nw);
		n = append(l, n, _cut, subtract(_cut, 0, _body, nb, _win, nw));
	}
	else {
		n = append(l, n, _body, nb);
	}
	if (hill) {
		// Only the parts of the hills not behind the floor
		const uint32_t nh = hill_row(y, hill_scroll, hill);
		n = append(l, n, _cut, subtract(_cut, 0, _hills, nh, _cover, nc));
	}
	return n;
}

// d1 is the land worked out for the frame
static void __not_in_flash_func(land_renderer)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	const RrLand_t* l = (const RrLand_t*)d1;
	const uint32_t a = l->start[row], b = l->start[row + 1];
	if (a < b) sprite_render_spans(r, &l->span[a], b - a, spriteId);
}

void rr_world_draw_terrain(void) {
	RrTerrainView_t* v = &_v;
	const int32_t cam = _cam >> RR_FP;
	v->c0 = cam >> RR_COL_SHIFT;
	v->x0 = -(cam & (RR_COL_W - 1));
	v->floor_min = RR_PLAY_BOTTOM;
	v->ceil_max = RR_PLAY_TOP - 1;
	for (uint32_t i = 0; i < RR_VIS; ++i) {
		const RrCol_t* c = column(v->c0 + (int32_t)i);
		if (!c) {
			v->fl[i] = v->fr[i] = RR_FLOOR_NONE;
			v->cl[i] = v->cr[i] = RR_CEIL_NONE;
			v->rim[i] = v->body[i] = v->dark[i] = v->window[i] = 0;
			continue;
		}
		const RrSectionInfo_t* s = &_sections[c->section];
		v->fl[i] = c->fl;
		v->fr[i] = c->fr;
		v->cl[i] = c->cl;
		v->cr[i] = c->cr;
		v->rim[i] = s->rim;
		v->body[i] = s->body;
		v->dark[i] = s->dark;
		v->window[i] = s->windows ? s->window : 0;
		const int32_t f = c->fl < c->fr ? c->fl : c->fr;
		const int32_t e = c->cl > c->cr ? c->cl : c->cr;
		if (f < v->floor_min) v->floor_min = f;
		if (e > v->ceil_max) v->ceil_max = e;
	}
	// The hills of the section in the middle of the screen, going by at half speed
	const RrSectionInfo_t* s = &_sections[rr_world_section(cam + (GAME_SCREEN_W >> 1))];
	const uint32_t hill_scroll = ((uint32_t)cam >> 1) & (RR_HILL_W - 1);

	_land_i ^= 1;
	RrLand_t* l = &_land[_land_i];
	uint32_t n = 0;
	for (int32_t row = 0; row < RR_ROWS; ++row) {
		l->start[row] = n;
		n = land_row(l, n, v, RR_PLAY_TOP + row, hill_scroll, s->hill);
	}
	l->start[RR_ROWS] = n;
	if (n) game_spr_add(0, RR_PLAY_TOP, GAME_SCREEN_W, RR_ROWS, l, NULL, land_renderer);
}

// d1 holds how far the nearest stars have scrolled
static void __not_in_flash_func(stars_renderer)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	const uint32_t sx0 = _star_x[row];
	if (sx0 == 0xffff) return;
	const uint32_t layer = _star_layer[row];
	const uint32_t scroll = (uint32_t)(uintptr_t)d1 >> (2 - layer);
	const int32_t sx = (int32_t)((sx0 - scroll) & (RR_STAR_WRAP - 1));
	if (sx >= GAME_SCREEN_W) return;
	static const hstx_dvi_pixel_t colours[3] = { RR_RGB(64, 64, 160), RR_RGB(160, 160, 192), RR_RGB(255, 255, 255) };
	sprite_render_span(r, sx, sx + 1 + (int32_t)layer * 2, colours[layer], spriteId);
}

void rr_world_draw_stars(uint32_t scroll) {
	game_spr_add(0, 0, GAME_SCREEN_W, GAME_SCREEN_H, (void*)(uintptr_t)scroll, NULL, stars_renderer);
}

void rr_world_draw_back(void) {
	const int32_t cam = _cam >> RR_FP;
	game_spr_add(0, RR_PLAY_TOP, GAME_SCREEN_W, RR_PLAY_BOTTOM - RR_PLAY_TOP, (void*)(uintptr_t)cam, NULL, stars_renderer);
}
