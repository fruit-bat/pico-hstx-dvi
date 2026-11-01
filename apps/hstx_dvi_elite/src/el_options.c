// Copyright (c) 2024 fruit-bat.

// See el_options.h.

#include "el_options.h"
#include "el_chart.h"
#include "el_draw.h"
#include "el_input.h"
#include "el_save.h"
#include "el_screen.h"
#include "el_trade.h"
#include "game_pallet.h"
#include <stdio.h>

typedef enum {
	OPTION_STYLE = 0,
	OPTION_DITHER,
	OPTION_HUD,
	OPTION_SCREEN,
	OPTION_DOCK,
	// For trying things out, until the game is finished
	OPTION_MAX_EQUIP,
	OPTION_WITCH,
	OPTION_END,
	OPTIONS
} Option_e;

// How wide the mark on the chosen option is, as wide as the widest
#define SELECT_W (26 * 8)
// And the panel they are on, with room for the arrows
#define PANEL_W (SELECT_W + 64)

static bool _shown = false;
static bool _in_game = false;
static uint32_t _option = 0;
// Whether the equipment was fitted since they were opened
static bool _fitted = false;
// Whether the docking computer was asked for
static bool _dock = false;
// Whether the next jump is to go wrong, into witchspace
static bool _witch = false;
static GameText_t _text[2][OPTIONS + 1];

// How many there are: the docking computer, the equipment and ending the
// game are only there in one
static uint32_t count(void) {
	return _in_game ? OPTIONS : OPTION_DOCK;
}

void options_open(bool in_game) {
	_shown = true;
	_in_game = in_game;
	_fitted = false;
	_dock = false;
	if (_option >= count()) _option = 0;
}

void options_close(void) {
	_shown = false;
}

bool options_dock(void) {
	const bool d = _dock;
	_dock = false;
	return d;
}

bool options_witch(void) {
	const bool w = _witch;
	_witch = false;
	return w;
}

bool options_shown(void) {
	return _shown;
}

bool options_input(uint16_t pressed) {
	if (pressed & EL_INP_START) {
		_shown = false;
		save_settings();
		return false;
	}
	const uint32_t n = count();
	if (pressed & EL_INP_DOWN) _option = (_option + 1) % n;
	if (pressed & EL_INP_UP) _option = (_option + n - 1) % n;
	const int32_t change = (pressed & EL_INP_RIGHT ? 1 : 0) - (pressed & EL_INP_LEFT ? 1 : 0);
	switch (_option) {
		case OPTION_STYLE:
			if (change) draw_style = (Style_e)((draw_style + STYLE_COUNT + change) % STYLE_COUNT);
			break;
		case OPTION_DITHER:
			if (change) draw_dither = !draw_dither;
			break;
		case OPTION_HUD:
			if (change) draw_hud_filled = !draw_hud_filled;
			break;
		case OPTION_SCREEN:
			if (change) {
				draw_aspect = draw_aspect == ASPECT_16_9 ? ASPECT_4_3 : ASPECT_16_9;
				chart_refresh();
			}
			break;
		case OPTION_DOCK:
			// It goes back to the game, to see it
			if (pressed & EL_INP_A) {
				_dock = true;
				_shown = false;
				save_settings();
			}
			break;
		case OPTION_WITCH:
			if (pressed & EL_INP_A) _witch = !_witch;
			break;
		case OPTION_MAX_EQUIP:
			if (pressed & EL_INP_A) {
				trade_max_equipment();
				_fitted = true;
			}
			break;
		default:
			// The game ends
			if (pressed & EL_INP_A) {
				_shown = false;
				return true;
			}
			break;
	}
	return false;
}

void options_draw(void) {
	GameText_t* t = _text[draw_set()];
	char s[GAME_TEXT_MAX + 1];
	const uint32_t n = count();
	// Centred in the view, with the line at the end
	const int32_t top = CENTRE_Y - (24 * (int32_t)n + 16) / 2;
	// The mark on the chosen one, in front of the panel, so first
	screen_select((GAME_SCREEN_W - SELECT_W) / 2, top + 24 * (int32_t)_option, SELECT_W);
	// On a panel of their own, hiding the screen under them
	if (object_begin()) {
		const float x0 = (float)(GAME_SCREEN_W - PANEL_W) / 2.0f, x1 = x0 + PANEL_W;
		const float y0 = (float)top - 16.0f, y1 = (float)(top + 24 * (int32_t)n + 16 + 8 + 12);
		add_box(x0, y0, x1, y1, hstx_dvi_pixel_rgb(36, 109, 219));
		add_rect(x0, y0, x1, y1, hstx_dvi_pixel_rgb(0, 0, 0));
		object_end();
	}
	for (uint32_t i = 0; i < n; ++i) {
		switch (i) {
			case OPTION_STYLE:
				snprintf(s, sizeof(s), "VIEW      < %s >", style_names[draw_style]);
				break;
			case OPTION_DITHER:
				snprintf(s, sizeof(s), "DITHERED  < %s >", draw_dither ? "ON" : "OFF");
				break;
			case OPTION_HUD:
				snprintf(s, sizeof(s), "DASHBOARD < %s >", draw_hud_filled ? "FILLED" : "LINES");
				break;
			case OPTION_SCREEN:
				snprintf(s, sizeof(s), "SCREEN    < %s >", draw_aspect == ASPECT_16_9 ? "16:9" : "4:3");
				break;
			case OPTION_DOCK:
				snprintf(s, sizeof(s), "DOCKING    (FIRE)");
				break;
			case OPTION_WITCH:
				snprintf(s, sizeof(s), _witch ? "WITCHSPACE NEXT JUMP" : "WITCHSPACE (FIRE)");
				break;
			case OPTION_MAX_EQUIP:
				snprintf(s, sizeof(s), _fitted ? "MAX EQUIP  FITTED" : "MAX EQUIP  (FIRE)");
				break;
			default:
				snprintf(s, sizeof(s), "END GAME   (FIRE)");
				break;
		}
		game_text_set(&t[i], s, 1);
		game_spr_text_centre(&t[i], top + 24 * (int32_t)i, game_pallet(i == _option ? GAME_YELLOW : GAME_WHITE));
		draw_count_sprite();
	}
	game_text_set(&t[OPTIONS], "START TO GO ON", 1);
	game_spr_text_centre(&t[OPTIONS], top + 24 * (int32_t)n + 16, game_pallet(GAME_CYAN));
	draw_count_sprite();
}
