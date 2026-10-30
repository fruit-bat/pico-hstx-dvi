#include "ww_hud.h"
#include "ww_witch.h"
#include "ww_world.h"
#include "ww_art.h"
#include "game_render.h"
#include "game_pallet.h"
#include "game_hiscore.h"
#include "game_util.h"
#include <stdio.h>
#include <string.h>

#define WW_HUD_SAYS 6
#define WW_HUD_SAY_FRAMES 90
#define WW_HUD_MOON_X 236
#define WW_HUD_MOON_R 15

typedef struct {
	int32_t x;
	int32_t y;
	uint32_t life;
	uint8_t colour;
	GameText_t text;
} WwSay_t;

static uint32_t _score = 0;
static uint32_t _lives = 0;
static uint32_t _next_bonus = 0;
static uint32_t _pieces = 0;
static GameText_t _score_text;
static GameText_t _hi_text;
static GameText_t _night_text;
static GameText_t _status_text;
static WwSay_t _says[WW_HUD_SAYS];

static GameArt_t _heart;
static GameArt_t _hat;
// The moon from new to full, a phase for each piece
static WwArt_t _moon[WW_PIECES + 1];
static GameArt_t _piece;

static const char* const _heart_rows[] = {
	"..PPP...PPP.....",
	".PPWPP.PPPPP....",
	"PPWPPPPPPPPPP...",
	"PPPPPPPPPPPPP...",
	"PPPPPPPPPPPPP...",
	".PPPPPPPPPPP....",
	"..PPPPPPPPP.....",
	"...PPPPPPP......",
	"....PPPPP.......",
	".....PPP........",
	"......P.........",
};

static const char* const _hat_rows[] = {
	"........PP......",
	".......PPP......",
	"......PPPP......",
	"......PPPPP.....",
	".....PPPPPP.....",
	".....PPPPPPP....",
	"....YYYYYYYY....",
	"....PPPPPPPP....",
	"PPPPPPPPPPPPPPPP",
	".PPPPPPPPPPPPPP.",
};

static const uint8_t _heart_colours[] = { GAME_PINK, GAME_WHITE };
static const uint8_t _hat_colours[] = { GAME_PURPLE, GAME_YELLOW };
// The lit side, the craters and the side in shadow
static const uint8_t _moon_colours[] = { GAME_WHITE, GAME_GREY, GAME_DARK_GREY };

static int32_t isqrt(int32_t v) {
	int32_t r = 0;
	while ((r + 1) * (r + 1) <= v) ++r;
	return r;
}

// The moon lit from the right by the given number of pieces out of WW_PIECES,
// the edge of the light curving across it so it waxes as a crescent
static void make_moon(uint32_t phase) {
	static const int8_t craters[][3] = { { -6, -5, 3 }, { 5, -6, 2 }, { 6, 4, 3 }, { -3, 7, 2 }, { -8, 3, 1 }, { 1, -1, 1 } };
	WwCanvas_t c;
	ww_canvas_clear(&c, 32, 32);
	const int32_t r2 = 4 * WW_HUD_MOON_R * WW_HUD_MOON_R;
	for (int32_t y = 0; y < 32; ++y) {
		// Doubled, from the middle of the moon
		const int32_t dy2 = 2 * y + 1 - 32;
		if (dy2 * dy2 > r2) continue;
		const int32_t h2 = isqrt(r2 - dy2 * dy2);
		const int32_t edge = h2 * (int32_t)(WW_PIECES - 2 * phase) / WW_PIECES;
		for (int32_t x = 0; x < 32; ++x) {
			const int32_t dx2 = 2 * x + 1 - 32;
			if (dx2 * dx2 + dy2 * dy2 > r2) continue;
			char ch = dx2 > edge ? 'W' : 'D';
			for (uint32_t k = 0; k < sizeof(craters) / sizeof(craters[0]); ++k) {
				const int32_t cx = dx2 - 2 * craters[k][0], cy = dy2 - 2 * craters[k][1];
				if (cx * cx + cy * cy <= 4 * craters[k][2] * craters[k][2]) ch = 'G';
			}
			ww_canvas_set(&c, x, y, ch);
		}
	}
	ww_art_make(&_moon[phase], &c, "WGD", _moon_colours);
}

// A little crescent, for a piece on its own
static void make_piece(void) {
	GameArtCanvas_t c;
	game_art_canvas_clear(&c, 16, 16);
	for (int32_t y = 0; y < 16; ++y) {
		for (int32_t x = 0; x < 16; ++x) {
			const int32_t ax = 2 * x - 15, ay = 2 * y - 15;
			const int32_t bx = 2 * x - 9, by = 2 * y - 13;
			if (ax * ax + ay * ay <= 13 * 13 && bx * bx + by * by > 11 * 11) game_art_canvas_set(&c, x, y, 'W');
		}
	}
	game_art_canvas_make(&_piece, &c, "W", _moon_colours);
}

void ww_hud_init(void) {
	game_art_make(&_heart, _heart_rows, GAME_ART_ROWS(_heart_rows), "PW", _heart_colours);
	game_art_make(&_hat, _hat_rows, GAME_ART_ROWS(_hat_rows), "PY", _hat_colours);
	for (uint32_t p = 0; p <= WW_PIECES; ++p) make_moon(p);
	make_piece();
}

void ww_hud_reset(void) {
	_score = 0;
	_lives = WW_HUD_START_LIVES;
	_next_bonus = WW_HUD_BONUS;
	_pieces = 0;
}

void ww_hud_add_score(uint32_t points) {
	_score += points;
	while (_score >= _next_bonus) {
		_next_bonus += WW_HUD_BONUS;
		if (_lives < WW_HUD_MAX_LIVES) ++_lives;
	}
}

uint32_t ww_hud_score(void) {
	return _score;
}

void ww_hud_set_score(uint32_t score) {
	_score = score;
}

uint32_t ww_hud_lives(void) {
	return _lives;
}

bool ww_hud_take_life(void) {
	if (!_lives) return false;
	--_lives;
	return true;
}

uint32_t ww_hud_pieces(void) {
	return _pieces;
}

void ww_hud_add_piece(uint32_t i) {
	_pieces |= 1u << i;
}

void ww_hud_clear_pieces(void) {
	_pieces = 0;
}

bool ww_hud_charm_whole(void) {
	return _pieces == WW_PIECES_ALL;
}

void ww_hud_piece_pos(uint32_t i, int32_t* x, int32_t* y) {
	// They all fly to the middle of the moon
	*x = WW_HUD_MOON_X + 8;
	*y = 8;
}

void ww_hud_draw_piece(uint32_t i, int32_t sx, int32_t sy) {
	ww_art_draw_small(&_piece, sx, sy, NULL);
}

void ww_hud_draw_charm(int32_t x, int32_t y, uint32_t pieces) {
	ww_art_draw(&_moon[__builtin_popcount(pieces & WW_PIECES_ALL)], x, y, NULL);
}

void ww_hud_draw(uint32_t night, uint32_t frame, bool in_game) {
	char t[GAME_TEXT_MAX];
	snprintf(t, sizeof(t), "%07lu", (unsigned long)(_score % 10000000));
	game_text_set(&_score_text, t, 2);
	game_spr_text(&_score_text, 8, 8, game_pallet(GAME_WHITE));

	const uint32_t top = game_hiscore_top();
	snprintf(t, sizeof(t), "HI %07lu", (unsigned long)((_score > top ? _score : top) % 10000000));
	game_text_set(&_hi_text, t, 1);
	game_spr_text(&_hi_text, 456, 4, game_pallet(GAME_GREY));

	if (!in_game) return;

	// The pictures in the HUD are above the play area, where they are usually cut off
	ww_art_set_clip(0);
	for (uint32_t i = 0; i < ww_witch_hearts(); ++i) {
		ww_art_draw_small(&_heart, 132 + i * 18, 2, NULL);
	}
	for (uint32_t i = 0; i < _lives; ++i) {
		ww_art_draw_small(&_hat, 132 + i * 18, 20, NULL);
	}
	// The charm waxes like the moon as the pieces come in
	ww_hud_draw_charm(WW_HUD_MOON_X, 0, _pieces);

	snprintf(t, sizeof(t), "NIGHT %lu", (unsigned long)night);
	game_text_set(&_night_text, t, 1);
	game_spr_text(&_night_text, 284, 4, game_pallet(GAME_CYAN));

	uint32_t colour;
	const char* status = ww_witch_status(&colour);
	if (status && ((frame >> 3) & 3)) {
		game_text_set(&_status_text, status, 1);
		game_spr_text(&_status_text, 284, 20, game_pallet(colour));
	}
	else if (ww_hud_charm_whole()) {
		game_text_set(&_status_text, "FIND THE WOLF", 1);
		game_spr_text(&_status_text, 284, 20, game_pallet_rainbow(frame >> 3));
	}

	ww_art_set_clip(WW_HUD_H);
	ww_world_draw_map(GAME_SCREEN_W - 72, 1, frame);
}

void ww_hud_say_clear(void) {
	for (uint32_t i = 0; i < WW_HUD_SAYS; ++i) _says[i].life = 0;
}

void ww_hud_say(int32_t x, int32_t y, const char* s, uint32_t colour) {
	// Takes the oldest if they are all in use
	WwSay_t* w = &_says[0];
	for (uint32_t i = 0; i < WW_HUD_SAYS; ++i) {
		if (!_says[i].life) {
			w = &_says[i];
			break;
		}
		if (_says[i].life < w->life) w = &_says[i];
	}
	game_text_set(&w->text, s, 1);
	const int32_t width = game_text_width(&w->text);
	w->x = game_clamp(x - (width >> 1), 4, GAME_SCREEN_W - 4 - width);
	w->y = y < WW_HUD_H + 4 ? WW_HUD_H + 4 : y;
	// Above anything else being said in the same place
	for (uint32_t i = 0; i < WW_HUD_SAYS; ++i) {
		const WwSay_t* o = &_says[i];
		if (o == w || !o->life || game_abs(o->y - w->y) >= 10) continue;
		if (o->x < w->x + width && w->x < o->x + game_text_width(&o->text)) w->y = o->y - 10;
	}
	w->life = WW_HUD_SAY_FRAMES;
	w->colour = colour;
}

void ww_hud_say_update(void) {
	for (uint32_t i = 0; i < WW_HUD_SAYS; ++i) {
		WwSay_t* w = &_says[i];
		if (!w->life) continue;
		--w->life;
		if ((w->life & 3) == 0 && w->y > WW_HUD_H + 4) --w->y;
	}
}

void ww_hud_say_draw(void) {
	for (uint32_t i = 0; i < WW_HUD_SAYS; ++i) {
		WwSay_t* w = &_says[i];
		if (!w->life) continue;
		game_spr_text(&w->text, w->x, w->y, game_pallet(w->colour));
	}
}
