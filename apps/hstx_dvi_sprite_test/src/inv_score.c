#include "inv_bullets.h"
#include "font_inv.h" // TODO extern linkage
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "inv_pallet.h"

static SpriteId _sprite_index = 0;
static uint8_t _score_text[16];
static TextGrid8_t _textGrid1 = {
	10,
	_score_text,
	(uint8_t*)&font_8x8
};
static uint32_t _score = 0;

void __not_in_flash_func(inv_score_update)(void) {
	// Update the score display
	sprintf((char*)_score_text, "Score %4.4ld", _score);
}

SpriteId inv_score_init(SpriteId start) {
    _sprite_index = start;
	_score = 0;
	inv_score_update();
	init_sprite(
		_sprite_index,
		16,
		0,
		16*8,
		1*8,
		SF_ENABLE,
		&_textGrid1,
		inv_pallet_green(),
		text_renderer_8x8_p1);

	return _sprite_index + 1;
}

void inv_score_clear() {
	_score = 0;
}

void __not_in_flash_func(inv_score_add)(uint32_t score) {
	_score += score;
}
