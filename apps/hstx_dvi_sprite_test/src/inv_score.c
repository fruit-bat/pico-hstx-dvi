#include "inv_score.h"
#include "font_inv.h"
#include <stdio.h>
#include "inv_pallet.h"
#include "inv_gun.h"
#include "inv_level.h"
#include "inv_hiscore.h"
#include <string.h>

#define INV_SCORE_TEXT_WIDTH 72

static SpriteId _sprite_index = 0;
static uint8_t _score_text[80] = {0};
static TextGrid8_t _textGrid1 = {
	INV_SCORE_TEXT_WIDTH,
	_score_text,
	(uint8_t*)font_8x8
};
static uint32_t _score = 0;
static bool _in_game = false;

// Writes formatted text into the score line without a terminating nul
static void put_text(const uint32_t at, const char* fmt, const unsigned long v) {
	char t[16];
	const int n = snprintf(t, sizeof(t), fmt, v);
	if (n > 0 && at + n <= INV_SCORE_TEXT_WIDTH) memcpy(&_score_text[at], t, n);
}

void __not_in_flash_func(inv_score_update)(void) {
	const uint32_t top = inv_hiscore_top();
	put_text(0, "SCORE %06lu", _score % 1000000);
	put_text(26, "HI %06lu", (_score > top ? _score : top) % 1000000);
	if (_in_game) {
		put_text(46, "WAVE %02lu", inv_level_get() % 100);
		put_text(64, "LIVES %1lu", inv_gun_get_lives() % 10);
	}
}

SpriteId inv_score_init(SpriteId start, bool in_game) {
	_sprite_index = start;
	_in_game = in_game;
	memset(_score_text, 32, sizeof(_score_text));
	inv_score_update();
	init_sprite(
		_sprite_index,
		(4*8),
		0,
		INV_SCORE_TEXT_WIDTH*8,
		1*8,
		SF_ENABLE,
		&_textGrid1,
		inv_pallet_green(),
		text_renderer_8x8_p1);

	return _sprite_index + 1;
}

void __not_in_flash_func(inv_score_add)(uint32_t score) {
	const uint32_t before = _score;
	_score += score;
	if ((_score / INV_SCORE_BONUS_LIFE) != (before / INV_SCORE_BONUS_LIFE)) {
		inv_gun_add_life();
	}
}

void inv_score_reset(void) {
	_score = 0;
}

uint32_t inv_score_get(void) {
	return _score;
}

void inv_score_set(uint32_t score) {
	_score = score;
}
