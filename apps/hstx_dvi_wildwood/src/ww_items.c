#include "ww_items.h"
#include "ww_world.h"
#include "ww_witch.h"
#include "ww_hud.h"
#include "ww_art.h"
#include "game_pallet.h"
#include "game_util.h"
#include "game_fx.h"

#define WW_FLOWER_CHANCE 35
// Each colour lasts this long before the flower changes to the next
#define WW_FLOWER_FRAMES 150
#define WW_FLOWER_REGROW (60 * 40)
#define WW_FLOWER_POINTS 25
#define WW_FLY_FRAMES 60
#define WW_PIECE_POINTS 500

typedef struct {
	bool has;
	int16_t x;
	int16_t y;
	uint8_t phase;
	uint32_t regrow; // the frame it is back in flower
} WwFlower_t;

typedef struct {
	uint32_t life;
	int32_t x0, y0; // on the screen
} WwFlying_t;

static WwFlower_t _flowers[WW_ROOMS];
static uint32_t _piece_room = WW_NO_ROOM;
static int32_t _piece_x, _piece_y;
static WwFlying_t _flying[WW_PIECES];
static uint32_t _frame = 0;

static GameArt_t _bloom[2];
static GameArt_t _bud;

static const char* const _bloom_rows[2][13] = {
	{
		"....CC..CC......",
		"...CCCCCCCC.....",
		"..CCCCYYCCCC....",
		".CCCCYYYYCCCC...",
		"..CCCYYYYCCC....",
		"...CCCYYCCC.....",
		"....CC..CC......",
		".......G........",
		"..GG...G...GG...",
		"...GGG.G.GGG....",
		".....GGGGG......",
		".......G........",
		".......G........",
	},
	{
		"................",
		"....CC..CC......",
		"...CCCYYCCC.....",
		"..CCCYYYYCCC....",
		"...CCYYYYCC.....",
		"....CCCYYC......",
		"................",
		".......G........",
		"..GG...G...GG...",
		"...GGG.G.GGG....",
		".....GGGGG......",
		".......G........",
		".......G........",
	},
};

static const char* const _bud_rows[] = {
	"................",
	"................",
	"................",
	"................",
	"................",
	".......GG.......",
	"......GGGG......",
	".......GG.......",
	".......G........",
	"..GG...G...GG...",
	"...GGG.G.GGG....",
	".....GGGGG......",
	".......G........",
};

static const uint8_t _flower_colours[] = { GAME_PINK, GAME_YELLOW, GAME_GREEN };
static const uint8_t _bud_colours[] = { GAME_GREEN };
// Petals and middle for each effect
static const uint8_t _effect_colours[WW_EFFECTS][3] = {
	{ GAME_PINK, GAME_WHITE, GAME_GREEN },
	{ GAME_YELLOW, GAME_ORANGE, GAME_GREEN },
	{ GAME_CYAN, GAME_WHITE, GAME_GREEN },
	{ GAME_PURPLE, GAME_YELLOW, GAME_GREEN },
};
static const char* const _effect_words[WW_EFFECTS] = { "HEARTS!", "SPEEDY!", "HUSH...", "MUDDLED!" };

void ww_items_init(void) {
	for (uint32_t i = 0; i < 2; ++i) game_art_make(&_bloom[i], _bloom_rows[i], 13, "CYG", _flower_colours);
	game_art_make(&_bud, _bud_rows, GAME_ART_ROWS(_bud_rows), "G", _bud_colours);
}

void ww_items_new_night(uint32_t bear_room, int32_t bear_x, int32_t bear_y) {
	const uint32_t start = ww_world_start();
	for (uint32_t room = 0; room < WW_ROOMS; ++room) {
		WwFlower_t* f = &_flowers[room];
		int32_t x, y;
		f->has = (room == start || game_rand() % 100 < WW_FLOWER_CHANCE) && ww_world_random_spot(room, &x, &y);
		f->x = x;
		f->y = y;
		f->phase = game_rand() & 3;
		f->regrow = 0;
	}
	// Left on the ground a little way from where the bear sleeps
	_piece_room = bear_room;
	_piece_x = bear_x;
	_piece_y = bear_y;
	if (bear_room != WW_NO_ROOM) {
		const uint8_t* m = ww_world_logic(bear_room);
		static const int8_t near[][2] = { { 2, 0 }, { -2, 0 }, { 0, 2 }, { 0, -2 }, { 2, 1 }, { -2, 1 }, { 1, 2 }, { -1, 2 } };
		for (uint32_t i = 0; i < sizeof(near) / sizeof(near[0]); ++i) {
			const int32_t c = (bear_x >> WW_TILE_SHIFT) + near[i][0];
			const int32_t r = (bear_y >> WW_TILE_SHIFT) + near[i][1];
			if (c < 1 || r < 1 || c >= WW_ROOM_COLS - 1 || r >= WW_ROOM_ROWS - 1) continue;
			const uint8_t l = m[r * WW_ROOM_COLS + c];
			if (ww_tile_solid(l) || !(l & WW_L_REACH)) continue;
			_piece_x = c * WW_TILE + WW_TILE / 2;
			_piece_y = r * WW_TILE + WW_TILE - 6;
			break;
		}
		// Nobody has the flower by the bear, it would wake him
		_flowers[bear_room].has = false;
	}
	for (uint32_t i = 0; i < WW_PIECES; ++i) _flying[i].life = 0;
}

static uint32_t flower_effect(const WwFlower_t* f) {
	return ((_frame / WW_FLOWER_FRAMES) + f->phase) % WW_EFFECTS;
}

static bool in_flower(const WwFlower_t* f) {
	return f->has && _frame >= f->regrow;
}

void ww_items_award_piece(uint32_t piece, int32_t x, int32_t y) {
	if (piece >= WW_PIECES) return;
	WwFlying_t* p = &_flying[piece];
	p->life = WW_FLY_FRAMES;
	p->x0 = x - 8;
	p->y0 = y - 8 + WW_HUD_H;
	ww_hud_add_score(WW_PIECE_POINTS);
	ww_hud_say(x, y + WW_HUD_H - 30, "A PIECE OF THE MOON CHARM!", GAME_YELLOW);
	ww_world_burst(x, y, GAME_YELLOW, 24, GAME_PX(3));
}

bool ww_items_flying(void) {
	for (uint32_t i = 0; i < WW_PIECES; ++i) {
		if (_flying[i].life) return true;
	}
	return false;
}

void ww_items_update(uint32_t frame) {
	_frame = frame;
	for (uint32_t i = 0; i < WW_PIECES; ++i) {
		WwFlying_t* p = &_flying[i];
		if (!p->life) continue;
		if (--p->life == 0) {
			ww_hud_add_piece(i);
			int32_t hx, hy;
			ww_hud_piece_pos(i, &hx, &hy);
			game_fx_explode(GAME_PX(hx + 8), GAME_PX(hy + 8), GAME_YELLOW, 16, GAME_PX(2));
		}
	}
	if (!ww_witch_alive()) return;
	const uint32_t room = ww_world_room();
	WwFlower_t* f = &_flowers[room];
	if (in_flower(f) && ww_witch_overlaps(f->x - 7, f->y - 12, f->x + 7, f->y)) {
		const uint32_t e = flower_effect(f);
		ww_witch_effect(e);
		f->regrow = frame + WW_FLOWER_REGROW;
		ww_hud_add_score(WW_FLOWER_POINTS);
		ww_hud_say(f->x, f->y + WW_HUD_H - 40, _effect_words[e], _effect_colours[e][0]);
		ww_world_burst(f->x, f->y - 8, _effect_colours[e][0], 16, GAME_PX(2));
	}
	if (room == _piece_room && !(ww_hud_pieces() & (1u << WW_PIECE_HUSH)) && !_flying[WW_PIECE_HUSH].life
		&& ww_witch_overlaps(_piece_x - 8, _piece_y - 14, _piece_x + 8, _piece_y)) {
		ww_items_award_piece(WW_PIECE_HUSH, _piece_x, _piece_y - 8);
	}
}

bool ww_items_piece_here(int32_t* x, int32_t* y) {
	if (ww_world_room() != _piece_room || (ww_hud_pieces() & (1u << WW_PIECE_HUSH)) || _flying[WW_PIECE_HUSH].life) return false;
	*x = _piece_x;
	*y = _piece_y;
	return true;
}

bool ww_items_flower_here(int32_t* x, int32_t* y, uint32_t* effect) {
	const WwFlower_t* f = &_flowers[ww_world_room()];
	if (!in_flower(f)) return false;
	*x = f->x;
	*y = f->y;
	*effect = flower_effect(f);
	return true;
}

void ww_items_draw_flower(uint32_t effect, int32_t sx, int32_t sy, uint32_t frame) {
	ww_art_draw_small(&_bloom[(frame >> 5) & 1], sx, sy, _effect_colours[effect % WW_EFFECTS]);
}

void ww_items_draw(uint32_t frame) {
	const uint32_t room = ww_world_room();
	const WwFlower_t* f = &_flowers[room];
	if (f->has) {
		const int32_t sx = f->x - 8, sy = f->y - 13 + WW_HUD_H;
		if (in_flower(f)) {
			ww_items_draw_flower(flower_effect(f), sx, sy, frame);
			if (((frame + f->phase * 16) & 31) == 0) {
				game_fx_explode(GAME_PX(f->x), GAME_PX(sy + 3), _effect_colours[flower_effect(f)][0], 1, GAME_PX(1) >> 1);
			}
		}
		else {
			ww_art_draw_small(&_bud, sx, sy, NULL);
		}
	}
	int32_t x, y;
	if (ww_items_piece_here(&x, &y)) {
		const int32_t bob = game_sin(frame << 2) * 3 / 127;
		ww_hud_draw_piece(WW_PIECE_HUSH, x - 8, y - 18 + WW_HUD_H + bob);
		if ((frame & 15) == 0) game_fx_explode(GAME_PX(x), GAME_PX(y - 10 + WW_HUD_H), GAME_YELLOW, 1, GAME_PX(1) >> 1);
	}
}

void ww_items_draw_flying(uint32_t frame) {
	ww_art_set_clip(0);
	for (uint32_t i = 0; i < WW_PIECES; ++i) {
		const WwFlying_t* p = &_flying[i];
		if (!p->life) continue;
		int32_t hx, hy;
		ww_hud_piece_pos(i, &hx, &hy);
		// Rises first, then swoops up to the HUD
		const int32_t t = WW_FLY_FRAMES - p->life;
		const int32_t x = p->x0 + (hx - p->x0) * t / WW_FLY_FRAMES;
		const int32_t y = p->y0 + (hy - p->y0) * t * t / (WW_FLY_FRAMES * WW_FLY_FRAMES) - game_sin(t * 128 / WW_FLY_FRAMES) * 40 / 127;
		ww_hud_draw_piece(i, x, y);
		if (frame & 1) game_fx_explode(GAME_PX(x + 8), GAME_PX(y + 8), GAME_YELLOW, 1, GAME_PX(1) >> 1);
	}
	ww_art_set_clip(WW_HUD_H);
}
