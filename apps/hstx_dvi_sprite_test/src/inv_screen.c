#include "inv_screen.h"
#include "font_inv.h"
#include <string.h>

typedef struct {
	char text[INV_SCREEN_TEXT_MAX];
	uint8_t shown[INV_SCREEN_TEXT_MAX];
	TextGrid8_t grid;
	uint32_t len;
} InvScreenSlot_t;

static SpriteId _sprite_index = 0;
static uint32_t _slots = 0;
static InvScreenSlot_t _slot[INV_SCREEN_MAX_SLOTS];

// Each nibble doubled up, for drawing big text. Not const so it lives in RAM.
static uint8_t _double_nibble[16] = {
	0x00, 0x03, 0x0c, 0x0f, 0x30, 0x33, 0x3c, 0x3f,
	0xc0, 0xc3, 0xcc, 0xcf, 0xf0, 0xf3, 0xfc, 0xff
};

// Draws a single line TextGrid8_t at twice the size, 16x16 pixels per character
static void __not_in_flash_func(text_renderer_16x16_p1)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	const TextGrid8_t *tg = (const TextGrid8_t *)d1;
	const uint8_t *font = tg->font;
	const uint32_t k = (row >> 1) & 7;
	Tile16x8p2_t t;
	for (uint32_t i = 0; i < tg->w; ++i) {
		const uint32_t c = tg->s[i];
		if (c < TEXT_GRID8_FIRST_CHAR || c > TEXT_GRID8_LAST_CHAR) continue;
		const uint32_t d = font[((c - TEXT_GRID8_FIRST_CHAR) << 3) + k];
		if (!d) continue;
		t.d[0] = (_double_nibble[d >> 4] << 8) | _double_nibble[d & 15];
		sprite_renderer_sprite_16x8_p1(&t, d2, r, x + (i << 4), 0, spriteId);
	}
}

void inv_screen_clear(void) {
	hstx_dvi_sprite_disable_all();
	for (uint32_t i = 0; i < MAX_SPRITES; ++i) {
		hstx_dvi_sprite_set_sprite_collision_mask(i, 0);
	}
}

SpriteId inv_screen_init(SpriteId start, uint32_t slots) {
	_sprite_index = start;
	_slots = slots < INV_SCREEN_MAX_SLOTS ? slots : INV_SCREEN_MAX_SLOTS;
	for (uint32_t i = 0; i < _slots; ++i) {
		InvScreenSlot_t *slot = &_slot[i];
		slot->len = 0;
		slot->grid.w = 0;
		slot->grid.s = slot->shown;
		slot->grid.font = (uint8_t*)font_8x8;
		hstx_dvi_sprite_disable(_sprite_index + i);
	}
	return _sprite_index + _slots;
}

void inv_screen_text(uint32_t slot, int32_t x, int32_t y, const char* s, hstx_dvi_pixel_t* p, bool big) {
	if (slot >= _slots) return;
	InvScreenSlot_t *t = &_slot[slot];
	uint32_t len = strlen(s);
	if (len > INV_SCREEN_TEXT_MAX) len = INV_SCREEN_TEXT_MAX;
	memcpy(t->text, s, len);
	memcpy(t->shown, s, len);
	t->len = len;
	t->grid.w = len;
	const uint32_t cw = big ? 16 : 8;
	init_sprite(
		_sprite_index + slot,
		x,
		y,
		len * cw,
		cw,
		SF_ENABLE,
		&t->grid,
		p,
		big ? text_renderer_16x16_p1 : text_renderer_8x8_p1);
}

void inv_screen_text_centre(uint32_t slot, int32_t y, const char* s, hstx_dvi_pixel_t* p, bool big) {
	const int32_t w = strlen(s) * (big ? 16 : 8);
	// Word aligned, so the text renderer can take its fast path
	inv_screen_text(slot, ((MODE_H_ACTIVE_PIXELS - w) >> 1) & ~3, y, s, p, big);
}

void inv_screen_text_reveal(uint32_t slot, uint32_t n) {
	if (slot >= _slots) return;
	InvScreenSlot_t *t = &_slot[slot];
	for (uint32_t i = 0; i < t->len; ++i) {
		t->shown[i] = i < n ? t->text[i] : ' ';
	}
}

uint32_t inv_screen_text_len(uint32_t slot) {
	return slot < _slots ? _slot[slot].len : 0;
}

void inv_screen_text_show(uint32_t slot, bool show) {
	if (slot >= _slots) return;
	Sprite *sprite = hstx_dvi_sprite_get(_sprite_index + slot);
	if (show) sprite->f |= SF_ENABLE;
	else hstx_dvi_sprite_disable_1(sprite);
}

void inv_screen_text_pallet(uint32_t slot, hstx_dvi_pixel_t* p) {
	if (slot >= _slots) return;
	hstx_dvi_sprite_get(_sprite_index + slot)->d2 = p;
}
