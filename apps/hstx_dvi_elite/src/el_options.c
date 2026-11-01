// Copyright (c) 2024 fruit-bat.

// See el_options.h.

#include "el_options.h"
#include "el_chart.h"
#include "el_draw.h"
#include "el_input.h"
#include "el_save.h"
#include "game_pallet.h"
#include <stdio.h>

typedef enum {
	OPTION_STYLE = 0,
	OPTION_DITHER,
	OPTION_HUD,
	OPTION_SCREEN,
	OPTION_END,
	OPTIONS
} Option_e;

static bool _shown = false;
static bool _in_game = false;
static uint32_t _option = 0;
static GameText_t _text[2][OPTIONS + 1];

// How many there are: ending the game is only there in one
static uint32_t count(void) {
	return _in_game ? OPTIONS : OPTION_END;
}

void options_open(bool in_game) {
	_shown = true;
	_in_game = in_game;
	if (_option >= count()) _option = 0;
}

void options_close(void) {
	_shown = false;
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
	const int32_t top = CENTRE_Y - 50;
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
