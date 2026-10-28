#include "def_hud.h"
#include "def_render.h"
#include "def_pallet.h"
#include "def_player.h"
#include "def_hiscore.h"
#include "def_world.h"
#include <stdio.h>

#define DEF_HUD_START_LIVES 3
#define DEF_HUD_START_SMART 3
#define DEF_HUD_MAX 9
// Most ships and smart bombs to show
#define DEF_HUD_ICONS 5

static uint32_t _score = 0;
static uint32_t _lives = 0;
static uint32_t _smart = 0;
static uint32_t _next_bonus = 0;
static DefText_t _score_text;
static DefText_t _hi_text;
static DefText_t _wave_text;

static Tile8x8p2_t _tile_smart = {{
	0b00000000,
	0b00111100,
	0b01111110,
	0b01111110,
	0b00111100,
	0b00000000,
	0b00000000,
	0b00000000,
}};

void def_hud_reset(void) {
	_score = 0;
	_lives = DEF_HUD_START_LIVES;
	_smart = DEF_HUD_START_SMART;
	_next_bonus = DEF_HUD_BONUS;
}

void __not_in_flash_func(def_hud_add_score)(uint32_t points) {
	_score += points;
	while (_score >= _next_bonus) {
		_next_bonus += DEF_HUD_BONUS;
		if (_lives < DEF_HUD_MAX) ++_lives;
		if (_smart < DEF_HUD_MAX) ++_smart;
	}
}

uint32_t def_hud_score(void) {
	return _score;
}

void def_hud_set_score(uint32_t score) {
	_score = score;
}

uint32_t def_hud_lives(void) {
	return _lives;
}

bool def_hud_take_life(void) {
	if (!_lives) return false;
	--_lives;
	return true;
}

uint32_t def_hud_smart_bombs(void) {
	return _smart;
}

bool def_hud_use_smart_bomb(void) {
	if (!_smart) return false;
	--_smart;
	return true;
}

void def_hud_draw(uint32_t wave, bool in_game) {
	char t[DEF_TEXT_MAX];
	snprintf(t, sizeof(t), "%7lu", (unsigned long)(_score % 10000000));
	def_text_set(&_score_text, t, 2);
	def_spr_text(&_score_text, 16, 6, def_pallet(DEF_CYAN));

	const uint32_t top = def_hiscore_top();
	snprintf(t, sizeof(t), "HI %07lu", (unsigned long)((_score > top ? _score : top) % 10000000));
	def_text_set(&_hi_text, t, 1);
	def_spr_text(&_hi_text, 472, 10, def_pallet(DEF_WHITE));

	if (in_game) {
		snprintf(t, sizeof(t), "WAVE %lu", (unsigned long)wave);
		def_text_set(&_wave_text, t, 1);
		def_spr_text(&_wave_text, 472, 26, def_pallet(DEF_GREEN));
	}

	// Separates the scanner from the play area
	def_spr_hline(0, DEF_HUD_H - 2, DEF_SCREEN_W, def_pallet(DEF_BLUE));

	if (in_game) {
		for (uint32_t i = 0; i < _lives && i < DEF_HUD_ICONS; ++i) {
			def_spr_add(16 + i * 20, 28, 16, 8, def_player_icon(), def_pallet(DEF_WHITE), sprite_renderer_sprite_16x8_p1);
		}
		for (uint32_t i = 0; i < _smart && i < DEF_HUD_ICONS; ++i) {
			def_spr_add(16 + i * 12, 40, 8, 6, &_tile_smart, def_pallet(DEF_RED), sprite_renderer_sprite_8x8_p1);
		}
	}
}
