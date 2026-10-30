#include "ww_witch.h"
#include "ww_input.h"
#include "ww_world.h"
#include "ww_art.h"
#include "game_pallet.h"
#include "game_util.h"

#define WW_WITCH_SPEED GAME_PX(2)
#define WW_WITCH_TIPTOE (GAME_PX(1) * 3 / 4)
#define WW_WITCH_HURT_FRAMES 90
#define WW_WITCH_KNOCK GAME_PX(4)
#define WW_WITCH_DIZZY_FRAMES 150
#define WW_WITCH_EFFECT_FRAMES 600
#define WW_WITCH_MUDDLE_FRAMES 360
#define WW_BOLT_SPEED GAME_PX(5)
#define WW_BOLT_FRAMES 52
#define WW_CAST_FRAMES 12
// 1/sqrt(2) in fixed point, for going diagonally
#define WW_DIAG 181

static WwArt_t _art[WW_VIEWS][WW_WITCH_STEPS];
static GameArt_t _star[2];

static int32_t _x, _y;
static int32_t _entry_x, _entry_y;
static uint32_t _facing;
static uint32_t _walk;
static bool _moving;
static bool _tiptoe;
static uint32_t _hurt;
static int32_t _kx, _ky;
static uint32_t _dizzy;
static bool _lost;
static uint32_t _hearts;
static uint32_t _effect[WW_EFFECTS];
static bool _bolt;
static int32_t _bx, _by, _bvx, _bvy;
static uint32_t _blife;
static uint32_t _cast;

static const int8_t _fdx[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
static const int8_t _fdy[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };

// The hat and hair are the same from the front and back
static const char* const _hat[] = {
	"...........PP...........",
	"..........PPP...........",
	"..........PPPP..........",
	".........PPPPP..........",
	".........PPPPPP.........",
	"........PPPPPPP.........",
	"........PPPPPPPP........",
	".......PPPPPPPPP........",
	".......PPPPPPPPPP.......",
	"......KKKKKYYKKKKK......",
	"......KKKKKYYKKKKK......",
	"..PPPPPPPPPPPPPPPPPPPP..",
	".PPPPPPPPPPPPPPPPPPPPPP.",
	"...OOOOOOOOOOOOOOOOOO...",
	"..OOOOOOOOOOOOOOOOOOOO..",
	"..OOOOOOOOOOOOOOOOOOOO..",
};

static const char* const _front[] = {
	"..OOSSSSSSSSSSSSSSSSOO..",
	"..OOSSSKKSSSSSSKKSSSOO..",
	"..OOSSSKKSSSSSSKKSSSOO..",
	"..OOSSSSSSSSSSSSSSSSOO..",
	"..OOOSSSSKSSSSKSSSSOOO..",
	"...OO.SSSSKKKKSSSS.OO...",
	"..OO....PPPPPPPP....OO..",
	"..O....PPPPPPPPPP....O..",
	".....SPPPPPPPPPPPPS.....",
	".....SPPPPPPPPPPPPS.....",
	"......PPPPPPPPPPPP......",
	".....PPPPPPPPPPPPPP.....",
	"....PPPPPPPPPPPPPPPP....",
	"....PPPPPPPPPPPPPPPP....",
};

static const char* const _back[] = {
	"..OOOOOOOOOOOOOOOOOOOO..",
	"..OOOOOOOOOOOOOOOOOOOO..",
	"..OOOOOOOOOOOOOOOOOOOO..",
	"...OOOOOOOOOOOOOOOOOO...",
	"...OOOOOOOOOOOOOOOOOO...",
	"....OOOOOOOOOOOOOOOO....",
	"..OO...PPPOOOOPPP...OO..",
	"..O....PPPPOOPPPP....O..",
	".....SPPPPPPPPPPPPS.....",
	".....SPPPPPPPPPPPPS.....",
	"......PPPPPPPPPPPP......",
	".....PPPPPPPPPPPPPP.....",
	"....PPPPPPPPPPPPPPPP....",
	"....PPPPPPPPPPPPPPPP....",
};

static const char* const _front_feet[WW_WITCH_STEPS][2] = {
	{ ".......KK......KK.......", "......KKK......KKK......" },
	{ ".......KK.......KK......", "......KKK..............." },
	{ "......KK.......KK.......", "...............KKK......" },
};

static const char* const _side[] = {
	"......PP................",
	".......PPP..............",
	"........PPP.............",
	"........PPPP............",
	".........PPPP...........",
	".........PPPPP..........",
	"........PPPPPPP.........",
	"........PPPPPPPP........",
	".......PPPPPPPPP........",
	".......KKKKKKKKYYK......",
	".......KKKKKKKKYYK......",
	"..PPPPPPPPPPPPPPPPPPPPP.",
	".PPPPPPPPPPPPPPPPPPPPPPP",
	"....OOOOOOOOOOOOOOO.....",
	"...OOOOOOOOOOOOOOOOO....",
	"...OOOOOOOOOOOOOOOOOO...",
	"..OOOOOOOOSSSSSSSSSS....",
	"..OOOOOOOSSSSSSSKKSS....",
	"..OOOOOOOSSSSSSSKKSSS...",
	"..OOOOOOOSSSSSSSSSSSSS..",
	"..OOO.OOOSSSSSSSSSSSS...",
	"..OO...OOSSSSSSSSKKS....",
	"..O.....PPPPPPPPPP......",
	"........PPPPPPPPPPP.....",
	".......PPPPPPPPPPSSS....",
	".......PPPPPPPPPPSS.....",
	".......PPPPPPPPPPP......",
	"......PPPPPPPPPPPPP.....",
	"......PPPPPPPPPPPPPP....",
	".....PPPPPPPPPPPPPPP....",
};

static const char* const _side_feet[WW_WITCH_STEPS][2] = {
	{ ".........KKK.............", "........KKKK............" },
	{ "......KKK.....KKK.......", ".....KKK.......KKKK....." },
	{ "........KKK.KKK.........", "........KKKKKKK........." },
};

static const char* const _star_rows[2][12] = {
	{
		"......W.........",
		"......Y.........",
		".....YWY........",
		"Y...YWWWY...Y...",
		".YYYWWWWWYYY....",
		"..YWWWWWWWY.....",
		"...YWWWWWY......",
		"..YWWYYYWWY.....",
		".YWY.....YWY....",
		"YY.........YY...",
		"................",
		"................",
	},
	{
		"................",
		"...Y.......Y....",
		"....Y..W..Y.....",
		".....YYWYY......",
		".....YWWWY......",
		"..WWWWWWWWWWW...",
		".....YWWWY......",
		".....YYWYY......",
		"....Y..W..Y.....",
		"...Y.......Y....",
		"................",
		"................",
	},
};

static const uint8_t _colours[] = { GAME_PURPLE, GAME_BLACK, GAME_YELLOW, GAME_ORANGE, GAME_PEACH };
#define WW_WITCH_KEY "PKYOS"
static const uint8_t _star_colours[] = { GAME_YELLOW, GAME_WHITE };

static void make_view(uint32_t view, uint32_t step) {
	WwCanvas_t c;
	ww_canvas_clear(&c, WW_WITCH_W, WW_WITCH_H);
	if (view == WW_VIEW_FRONT || view == WW_VIEW_BACK) {
		ww_canvas_stamp(&c, _hat, GAME_ART_ROWS(_hat), 0, 0);
		ww_canvas_stamp(&c, view == WW_VIEW_FRONT ? _front : _back, GAME_ART_ROWS(_front), 0, 16);
		ww_canvas_stamp(&c, _front_feet[step], 2, 0, 30);
		if (view == WW_VIEW_BACK) {
			// No buckle on the back of the hat
			for (int32_t x = 0; x < WW_WITCH_W; ++x) {
				if (c.c[9][x] == 'Y') c.c[9][x] = 'K';
				if (c.c[10][x] == 'Y') c.c[10][x] = 'K';
			}
		}
	}
	else {
		ww_canvas_stamp(&c, _side, GAME_ART_ROWS(_side), 0, 0);
		ww_canvas_stamp(&c, _side_feet[step], 2, 0, 30);
		if (view == WW_VIEW_LEFT) ww_canvas_mirror(&c);
	}
	ww_art_make(&_art[view][step], &c, WW_WITCH_KEY, _colours);
}

void ww_witch_init(void) {
	for (uint32_t v = 0; v < WW_VIEWS; ++v) {
		for (uint32_t s = 0; s < WW_WITCH_STEPS; ++s) make_view(v, s);
	}
	for (uint32_t i = 0; i < 2; ++i) game_art_make(&_star[i], _star_rows[i], 12, "YW", _star_colours);
}

void ww_witch_reset(int32_t x, int32_t y) {
	_x = GAME_PX(x);
	_y = GAME_PX(y);
	_entry_x = x;
	_entry_y = y;
	_facing = 2;
	_walk = 0;
	_moving = false;
	_tiptoe = false;
	_hurt = WW_WITCH_HURT_FRAMES;
	_kx = _ky = 0;
	_dizzy = 0;
	_lost = false;
	_hearts = WW_WITCH_START_HEARTS;
	for (uint32_t i = 0; i < WW_EFFECTS; ++i) _effect[i] = 0;
	_bolt = false;
	_cast = 0;
}

void ww_witch_facing_step(uint32_t facing, int32_t* dx, int32_t* dy) {
	*dx = _fdx[facing & 7];
	*dy = _fdy[facing & 7];
}

static uint32_t facing_of(const int32_t dx, const int32_t dy) {
	for (uint32_t f = 0; f < 8; ++f) {
		if (_fdx[f] == dx && _fdy[f] == dy) return f;
	}
	return _facing;
}

static uint32_t view_of(const uint32_t facing) {
	if (facing == 2) return WW_VIEW_FRONT;
	if (facing == 6) return WW_VIEW_BACK;
	return _fdx[facing] > 0 ? WW_VIEW_RIGHT : WW_VIEW_LEFT;
}

static void cast(void) {
	const int32_t dx = _fdx[_facing], dy = _fdy[_facing];
	const int32_t s = dx && dy ? WW_BOLT_SPEED * WW_DIAG >> GAME_FP : WW_BOLT_SPEED;
	_bolt = true;
	_bx = _x + dx * GAME_PX(10);
	_by = _y - GAME_PX(16) + dy * GAME_PX(8);
	_bvx = dx * s;
	_bvy = dy * s;
	_blife = WW_BOLT_FRAMES;
	_cast = WW_CAST_FRAMES;
	ww_world_burst(_bx >> GAME_FP, _by >> GAME_FP, GAME_COLOUR_RANDOM, 8, GAME_PX(1));
}

static void update_bolt(uint32_t frame) {
	if (!_bolt) return;
	_bx += _bvx;
	_by += _bvy;
	const int32_t x = _bx >> GAME_FP, y = _by >> GAME_FP;
	// Leaves a trail of sparkles
	if (frame & 1) {
		game_fx_explode(GAME_PX(x), GAME_PX(y + WW_HUD_H), game_rand() % GAME_RAINBOW_COUNT, 1, GAME_PX(1) >> 1);
	}
	// Spells go over water but not through bushes, trees or rocks
	const uint8_t* m = ww_world_logic(ww_world_room());
	const int32_t c = x >> WW_TILE_SHIFT, r = y >> WW_TILE_SHIFT;
	const bool off = x < 0 || y < 0 || x >= WW_ROOM_W || y >= WW_ROOM_H;
	const bool blocked = !off && ww_tile_solid(m[r * WW_ROOM_COLS + c]) && (m[r * WW_ROOM_COLS + c] & WW_L_TYPE) != WW_L_WATER;
	if (off || blocked || --_blife == 0) ww_witch_bolt_end();
}

int32_t ww_witch_update(uint8_t input, uint8_t pressed, uint32_t frame) {
	if (_cast) --_cast;
	for (uint32_t i = 0; i < WW_EFFECTS; ++i) if (_effect[i]) --_effect[i];
	update_bolt(frame);

	if (_dizzy) {
		// Stars going round her head
		if ((frame & 7) == 0) {
			const int32_t a = frame << 3;
			game_fx_explode(_x + game_sin(a + 64) * 12 * 2, GAME_PX(WW_HUD_H - 34) + _y + game_sin(a) * 4 * 2, GAME_YELLOW, 1, GAME_PX(1) >> 2);
		}
		if (--_dizzy == 0) _lost = true;
		return -1;
	}
	if (_hurt) --_hurt;

	if (_effect[WW_EFFECT_MUDDLE]) {
		// Left is right and up is down
		input = (input & ~WW_INP_DIRS)
			| ((input & WW_INP_BIT_L) ? WW_INP_BIT_R : 0) | ((input & WW_INP_BIT_R) ? WW_INP_BIT_L : 0)
			| ((input & WW_INP_BIT_U) ? WW_INP_BIT_D : 0) | ((input & WW_INP_BIT_D) ? WW_INP_BIT_U : 0);
	}
	const int32_t dx = ((input & WW_INP_BIT_R) ? 1 : 0) - ((input & WW_INP_BIT_L) ? 1 : 0);
	const int32_t dy = ((input & WW_INP_BIT_D) ? 1 : 0) - ((input & WW_INP_BIT_U) ? 1 : 0);
	_tiptoe = (input & WW_INP_BIT_B) != 0;
	_moving = dx || dy;
	if (_moving) _facing = facing_of(dx, dy);

	int32_t s = _tiptoe ? WW_WITCH_TIPTOE : WW_WITCH_SPEED;
	if (_effect[WW_EFFECT_SPEEDY]) s = s * 3 / 2;
	if (dx && dy) s = s * WW_DIAG >> GAME_FP;
	int32_t mx = dx * s + _kx, my = dy * s + _ky;
	_kx = _kx * 3 / 4;
	_ky = _ky * 3 / 4;
	ww_world_move(&_x, &_y, mx, my, WW_WITCH_HW, WW_WITCH_HH);
	if (_moving) _walk += _tiptoe ? 1 : 2;

	if ((pressed & WW_INP_BIT_F) && !_bolt) cast();

	const int32_t x = _x >> GAME_FP, y = _y >> GAME_FP;
	if (x < 2) return WW_DIR_W;
	if (x > WW_ROOM_W - 2) return WW_DIR_E;
	if (y < WW_WITCH_HH + 2) return WW_DIR_N;
	if (y > WW_ROOM_H - 1) return WW_DIR_S;
	return -1;
}

void ww_witch_enter(int32_t dir) {
	switch (dir) {
		case WW_DIR_W: _x = GAME_PX(WW_ROOM_W - 10); break;
		case WW_DIR_E: _x = GAME_PX(10); break;
		case WW_DIR_N: _y = GAME_PX(WW_ROOM_H - 4); break;
		default: _y = GAME_PX(WW_WITCH_HH + 14); break;
	}
	_entry_x = _x >> GAME_FP;
	_entry_y = _y >> GAME_FP;
	_kx = _ky = 0;
	_bolt = false;
}

void ww_witch_entry(int32_t* x, int32_t* y) {
	*x = _entry_x;
	*y = _entry_y;
}

int32_t ww_witch_x(void) {
	return _x >> GAME_FP;
}

int32_t ww_witch_y(void) {
	return _y >> GAME_FP;
}

uint32_t ww_witch_facing(void) {
	return _facing;
}

bool ww_witch_alive(void) {
	return !_dizzy && !_lost;
}

bool ww_witch_lost(void) {
	return _lost;
}

bool ww_witch_quiet(void) {
	return _tiptoe || _effect[WW_EFFECT_HUSH] || !_moving;
}

bool ww_witch_hushed(void) {
	return _effect[WW_EFFECT_HUSH] != 0;
}

bool ww_witch_overlaps(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
	const int32_t x = _x >> GAME_FP, y = _y >> GAME_FP;
	return x0 < x + 8 && x1 > x - 8 && y0 < y && y1 > y - 22;
}

bool ww_witch_hurt(int32_t x, int32_t y) {
	if (_hurt || _dizzy || _lost) return false;
	// Knocked away from whatever it was
	const int32_t dx = (_x >> GAME_FP) - x, dy = (_y >> GAME_FP) - y;
	const int32_t d = game_abs(dx) + game_abs(dy) + 1;
	_kx = WW_WITCH_KNOCK * dx / d;
	_ky = WW_WITCH_KNOCK * dy / d;
	ww_world_burst(_x >> GAME_FP, (_y >> GAME_FP) - 20, GAME_PINK, 10, GAME_PX(2));
	if (_hearts) --_hearts;
	if (!_hearts) {
		_dizzy = WW_WITCH_DIZZY_FRAMES;
		_bolt = false;
	}
	_hurt = WW_WITCH_HURT_FRAMES;
	return true;
}

uint32_t ww_witch_hearts(void) {
	return _hearts;
}

void ww_witch_effect(uint32_t effect) {
	if (effect == WW_EFFECT_HEART) {
		_hearts = WW_WITCH_MAX_HEARTS;
		return;
	}
	_effect[effect] = effect == WW_EFFECT_MUDDLE ? WW_WITCH_MUDDLE_FRAMES : WW_WITCH_EFFECT_FRAMES;
	// Hushing and muddling cancel each other out
	if (effect == WW_EFFECT_MUDDLE) _effect[WW_EFFECT_HUSH] = 0;
	if (effect == WW_EFFECT_HUSH) _effect[WW_EFFECT_MUDDLE] = 0;
}

const char* ww_witch_status(uint32_t* colour) {
	if (_effect[WW_EFFECT_MUDDLE]) { *colour = GAME_PURPLE; return "MUDDLED"; }
	if (_effect[WW_EFFECT_HUSH]) { *colour = GAME_CYAN; return "HUSHED"; }
	if (_effect[WW_EFFECT_SPEEDY]) { *colour = GAME_YELLOW; return "SPEEDY"; }
	if (_tiptoe && !_dizzy) { *colour = GAME_WHITE; return "TIPTOE"; }
	return NULL;
}

bool ww_witch_bolt(int32_t* x, int32_t* y) {
	*x = _bx >> GAME_FP;
	*y = _by >> GAME_FP;
	return _bolt;
}

void ww_witch_bolt_end(void) {
	if (!_bolt) return;
	_bolt = false;
	ww_world_burst(_bx >> GAME_FP, _by >> GAME_FP, GAME_COLOUR_RANDOM, 10, GAME_PX(2));
}

void ww_witch_draw_at(int32_t sx, int32_t sy, uint32_t view, uint32_t step) {
	ww_art_draw(&_art[view % WW_VIEWS][step % WW_WITCH_STEPS], sx, sy, NULL);
}

void ww_witch_draw(uint32_t frame) {
	// Flickers while she can not be hurt, and is see through when hushed
	if (_hurt && !_dizzy && (frame & 4)) return;
	if (_effect[WW_EFFECT_HUSH] && (frame & 1)) return;
	uint32_t view = view_of(_facing);
	uint32_t step = 0;
	if (_dizzy) {
		static const uint8_t spin[4] = { WW_VIEW_FRONT, WW_VIEW_RIGHT, WW_VIEW_BACK, WW_VIEW_LEFT };
		view = spin[(frame >> 3) & 3];
	}
	else if (_moving) {
		step = 1 + ((_walk >> 4) & 1);
	}
	const int32_t sx = (_x >> GAME_FP) - (WW_WITCH_W >> 1);
	// Bobs as she walks
	const int32_t sy = (_y >> GAME_FP) - WW_WITCH_H + WW_HUD_H - (step ? 1 : 0);
	ww_witch_draw_at(sx, sy, view, step);
}

void ww_witch_draw_bolt(uint32_t frame) {
	if (!_bolt) return;
	const uint32_t i = (frame >> 2) & 1;
	static const uint8_t twinkle[2][2] = { { GAME_YELLOW, GAME_WHITE }, { GAME_WHITE, GAME_YELLOW } };
	ww_art_draw_small(&_star[i], (_bx >> GAME_FP) - 6, (_by >> GAME_FP) - 6 + WW_HUD_H, twinkle[(frame >> 3) & 1]);
}
