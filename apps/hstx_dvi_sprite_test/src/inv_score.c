#include "inv_bullets.h"
#include "font_inv.h" // TODO extern linkage
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "inv_pallet.h"
#include "inv_gun.h"
#include <string.h>

static SpriteId _sprite_index = 0;
static uint8_t _score_text[80] = {0};
static TextGrid8_t _textGrid1 = {
	72,
	_score_text,
	(uint8_t*)&font_8x8
};
static uint32_t _score = 0;

void __not_in_flash_func(inv_score_update)(void) {
	// Update the score display
	sprintf((char*)&_score_text[0],  "Score %5.5ld", _score);
	_score_text[11] = ' ';
	sprintf((char*)&_score_text[60], "Lives %1.1ld", inv_gun_get_lives());
	_score_text[60 + 7] = ' ';
}

SpriteId inv_score_init(SpriteId start) {
    _sprite_index = start;
	_score = 0;
	inv_score_update();
	init_sprite(
		_sprite_index,
		(4*8),
		0,
		(80-8)*8,
		1*8,
		SF_ENABLE,
		&_textGrid1,
		inv_pallet_green(),
		text_renderer_8x8_p1);

	memset(_score_text, 32, sizeof(_score_text));
	return _sprite_index + 1;
}

void inv_score_clear() {
	_score = 0;
}

void __not_in_flash_func(inv_score_add)(uint32_t score) {
	_score += score;
}
