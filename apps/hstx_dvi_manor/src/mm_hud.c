#include "mm_hud.h"
#include "mm_witch.h"
#include "mm_thing.h"
#include "mm_camera.h"
#include "mm_tiles.h"
#include "mm_art.h"
#include "mm_gfx.h"
#include "game_render.h"
#include "game_pallet.h"
#include "game_hiscore.h"
#include "game_fx.h"
#include "game_util.h"
#include <stdio.h>

#define MM_HUD_SAYS 8
#define MM_HUD_SAY_FRAMES 90
#define MM_HUD_KITTENS_X 236
#define MM_HUD_ITEMS_X 330

typedef struct {
	int32_t x; // in the house, the middle of the text
	int32_t y;
	uint32_t life;
	uint8_t colour;
	GameText_t text;
} MmSay_t;

static uint32_t _score = 0;
static uint32_t _lives = 0;
static uint32_t _next_bonus = 0;
static GameText_t _score_text;
static GameText_t _hi_text;
static GameText_t _night_text;
static GameText_t _status_text;
static MmSay_t _says[MM_HUD_SAYS];
static hstx_dvi_pixel_t _band[1];
static hstx_dvi_pixel_t _edge[1];

void mm_hud_init(void) {
	_band[0] = HSTX_DVI_PIXEL_RGB(0, 0, 32);
	_edge[0] = HSTX_DVI_PIXEL_RGB(96, 32, 128);
}

void mm_hud_reset(void) {
	_score = 0;
	_lives = MM_HUD_START_LIVES;
	_next_bonus = MM_HUD_BONUS;
}

void mm_hud_add_score(uint32_t points) {
	_score += points;
	while (_score >= _next_bonus) {
		_next_bonus += MM_HUD_BONUS;
		if (_lives < MM_HUD_MAX_LIVES) ++_lives;
	}
}

void mm_hud_score_at(int32_t x, int32_t y, uint32_t points) {
	mm_hud_add_score(points);
	// Under anything said there
	game_fx_popup(GAME_PX(x - 8), y + 8, points);
}

uint32_t mm_hud_score(void) {
	return _score;
}

void mm_hud_set_score(uint32_t score) {
	_score = score;
}

uint32_t mm_hud_lives(void) {
	return _lives;
}

bool mm_hud_take_life(void) {
	if (!_lives) return false;
	--_lives;
	return true;
}

void mm_hud_draw(uint32_t night, uint32_t frame, bool in_game) {
	// Over anything in the house that has gone up under it
	mm_gfx_rect(0, 0, GAME_SCREEN_W, MM_HUD_H - 2, _band);
	mm_gfx_rect(0, MM_HUD_H - 2, GAME_SCREEN_W, 2, _edge);

	char t[GAME_TEXT_MAX];
	snprintf(t, sizeof(t), "%07lu", (unsigned long)(_score % 10000000));
	game_text_set(&_score_text, t, 2);
	game_spr_text(&_score_text, 8, 7, game_pallet(GAME_WHITE));

	const uint32_t top = game_hiscore_top();
	snprintf(t, sizeof(t), "HI %07lu", (unsigned long)((_score > top ? _score : top) % 10000000));
	game_text_set(&_hi_text, t, 1);
	game_spr_text(&_hi_text, 540, 4, game_pallet(GAME_GREY));

	if (!in_game) return;

	for (uint32_t i = 0; i < mm_witch_hearts(); ++i) mm_gfx_draw(&mm_art_heart, mm_pal_heart, 132 + i * 18, 0, 2, 0);
	for (uint32_t i = 0; i < _lives; ++i) mm_gfx_draw(&mm_art_hat, mm_pal_hazel, 132 + i * 16, 16, 2, 0);

	// A face for each kitten, coloured in once found
	const uint32_t found = mm_thing_kitten_mask();
	for (uint32_t i = 0; i < MM_KITTENS; ++i) {
		const hstx_dvi_pixel_t* pal = (found & (1u << i)) ? mm_pal_kitten[i] : mm_pal_kitten_none;
		mm_gfx_draw(&mm_art_kitten_face, pal, MM_HUD_KITTENS_X + i * 18, 4, 2, 0);
	}
	int32_t x = MM_HUD_ITEMS_X;
	if (mm_witch_keys()) {
		mm_gfx_draw(&mm_art_key, mm_pal_key, x, 2, 2, 0);
		x += 36;
	}
	if (mm_witch_can_float()) mm_gfx_draw(&mm_art_feather, mm_pal_feather, x, 0, 2, 0);

	snprintf(t, sizeof(t), "NIGHT %lu", (unsigned long)night);
	game_text_set(&_night_text, t, 1);
	game_spr_text(&_night_text, 440, 4, game_pallet(GAME_CYAN));
	snprintf(t, sizeof(t), "KITTENS %lu/%u", (unsigned long)mm_thing_kittens_found(), MM_KITTENS);
	game_text_set(&_status_text, t, 1);
	game_spr_text(&_status_text, 440, 18, game_pallet(GAME_PINK));
}

void mm_hud_say_clear(void) {
	for (uint32_t i = 0; i < MM_HUD_SAYS; ++i) _says[i].life = 0;
}

void mm_hud_say(int32_t x, int32_t y, const char* s, uint32_t colour) {
	// Takes the oldest if they are all in use
	MmSay_t* w = &_says[0];
	for (uint32_t i = 0; i < MM_HUD_SAYS; ++i) {
		if (!_says[i].life) {
			w = &_says[i];
			break;
		}
		if (_says[i].life < w->life) w = &_says[i];
	}
	game_text_set(&w->text, s, 1);
	w->x = x;
	w->y = y;
	const int32_t width = game_text_width(&w->text);
	// Above anything else being said in the same place
	for (uint32_t i = 0; i < MM_HUD_SAYS; ++i) {
		const MmSay_t* o = &_says[i];
		if (o == w || !o->life || game_abs(o->y - w->y) >= 10) continue;
		if (game_abs(o->x - w->x) < (width + game_text_width(&o->text)) / 2) w->y = o->y - 10;
	}
	w->life = MM_HUD_SAY_FRAMES;
	w->colour = colour;
}

void mm_hud_say_update(void) {
	for (uint32_t i = 0; i < MM_HUD_SAYS; ++i) {
		MmSay_t* w = &_says[i];
		if (!w->life) continue;
		--w->life;
		if ((w->life & 3) == 0) --w->y;
	}
}

void mm_hud_say_draw(void) {
	for (uint32_t i = 0; i < MM_HUD_SAYS; ++i) {
		MmSay_t* w = &_says[i];
		if (!w->life) continue;
		const int32_t width = game_text_width(&w->text);
		const int32_t sx = game_clamp(mm_screen_x(w->x) - width / 2, 4, GAME_SCREEN_W - 4 - width);
		const int32_t sy = mm_screen_y(w->y);
		if (sy < MM_HUD_H + 2 || sy > GAME_SCREEN_H - 8) continue;
		game_spr_text(&w->text, sx, sy, game_pallet(w->colour));
	}
}
