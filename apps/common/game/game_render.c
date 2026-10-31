#include "game_render.h"
#include "font_inv.h"
#include <string.h>

#define GAME_LASER_HEAD_LEFT 0x10000
// The length of the solid part of a laser beam
#define GAME_LASER_SOLID 64

static uint32_t _next = 0;
static uint32_t _last = 0;

void game_spr_begin(void) {
	game_spr_begin_at(0);
}

void game_spr_begin_at(SpriteId first) {
	_next = first;
	if (_last < first) _last = first;
}

Sprite* __not_in_flash_func(game_spr_add)(int32_t x, int32_t y, uint32_t w, uint32_t h, void* d1, void* d2, SpriteRenderer r) {
	if (_next >= MAX_SPRITES) return NULL;
	const uint32_t i = _next++;
	init_sprite(i, x, y, w, h, SF_ENABLE, d1, d2, r);
	return hstx_dvi_sprite_get(i);
}

void game_spr_end(void) {
	for (uint32_t i = _next; i < _last; ++i) {
		hstx_dvi_sprite_disable(i);
	}
	_last = _next;
}

// Clips a run of len pixels at x to the screen, returning false if none are visible
static __force_inline bool clip_run(int32_t* const i0, int32_t* const i1, const int32_t x, const int32_t len) {
	*i0 = x < 0 ? -x : 0;
	*i1 = x + len > GAME_SCREEN_W ? GAME_SCREEN_W - x : len;
	return *i0 < *i1;
}

static void __not_in_flash_func(fill_renderer)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	const uint32_t pq = ((const hstx_dvi_pixel_t*)d2)[0] * 0x01010101u;
	for (uint32_t i = 0; i < (GAME_SCREEN_W >> 2); ++i) r->w[i] = pq;
}

// d1 holds the length
static void __not_in_flash_func(hline_renderer)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	const int32_t len = (int32_t)(uintptr_t)d1;
	const hstx_dvi_pixel_t p = ((const hstx_dvi_pixel_t*)d2)[0];
	int32_t i0, i1;
	if (!clip_run(&i0, &i1, x, len)) return;
	for (int32_t i = i0; i < i1; ++i) hstx_dvi_row_set_pixel(r, x + i, p);
}

// d1 holds the length, with GAME_LASER_HEAD_LEFT set if the head is at the left
static void __not_in_flash_func(laser_renderer)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	const uint32_t v = (uint32_t)(uintptr_t)d1;
	const int32_t len = v & 0xffff;
	const bool head_left = (v & GAME_LASER_HEAD_LEFT) != 0;
	const hstx_dvi_pixel_t p = ((const hstx_dvi_pixel_t*)d2)[0];
	int32_t i0, i1;
	if (!clip_run(&i0, &i1, x, len)) return;
	for (int32_t i = i0; i < i1; ++i) {
		const int32_t from_head = head_left ? i : len - 1 - i;
		// Gaps that open up further back along the beam
		if (from_head >= GAME_LASER_SOLID) {
			int32_t gap = (from_head - GAME_LASER_SOLID) >> 6;
			if (gap > 3) gap = 3;
			if (((from_head >> 2) & 3) < gap) continue;
		}
		hstx_dvi_row_set_pixel(r, x + i, p);
	}
}

// d1 points to the bitmap
static void __not_in_flash_func(bitmap_renderer)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	const uint32_t* b = (const uint32_t*)d1 + row * GAME_BITMAP_WORDS;
	const hstx_dvi_pixel_t p = ((const hstx_dvi_pixel_t*)d2)[0];
	for (uint32_t wi = 0; wi < GAME_BITMAP_WORDS; ++wi) {
		uint32_t m = b[wi];
		while (m) {
			const uint32_t i = __builtin_clz(m);
			m ^= 0x80000000u >> i;
			hstx_dvi_row_set_pixel(r, (wi << 5) + i, p);
		}
	}
}

// d1 points to a GamePixels_t
static void __not_in_flash_func(pixels_renderer)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	const GamePixels_t* pixels = (const GamePixels_t*)d1;
	const uint32_t* src = pixels->data + row * pixels->words;
	uint32_t* dst = &r->w[(uint32_t)x >> 2];
	for (uint32_t i = 0; i < pixels->words; ++i) dst[i] = src[i];
}

// d1 points to a GameText_t, drawn scale times the size of the font
static void __not_in_flash_func(text_scaled_renderer)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	const GameText_t* t = (const GameText_t*)d1;
	const int32_t s = t->scale;
	const uint8_t* font = t->grid.font;
	const uint32_t k = (row / s) & 7;
	const hstx_dvi_pixel_t p = ((const hstx_dvi_pixel_t*)d2)[0];
	for (uint32_t i = 0; i < t->grid.w; ++i) {
		const uint32_t c = t->s[i];
		if (c < TEXT_GRID8_FIRST_CHAR || c > TEXT_GRID8_LAST_CHAR) continue;
		const uint32_t d = font[((c - TEXT_GRID8_FIRST_CHAR) << 3) + k];
		if (!d) continue;
		const int32_t cx = x + (int32_t)(i << 3) * s;
		for (uint32_t b = 0; b < 8; ++b) {
			if (!(d & (0x80 >> b))) continue;
			int32_t i0, i1;
			const int32_t px = cx + (int32_t)b * s;
			if (!clip_run(&i0, &i1, px, s)) continue;
			for (int32_t j = i0; j < i1; ++j) hstx_dvi_row_set_pixel(r, px + j, p);
		}
	}
}

void game_spr_fill(hstx_dvi_pixel_t* p) {
	game_spr_add(0, 0, GAME_SCREEN_W, GAME_SCREEN_H, NULL, p, fill_renderer);
}

void game_spr_hline(int32_t x, int32_t y, uint32_t len, hstx_dvi_pixel_t* p) {
	game_spr_add(x, y, len, 1, (void*)(uintptr_t)len, p, hline_renderer);
}

void game_spr_laser(int32_t x, int32_t y, uint32_t len, bool head_left, hstx_dvi_pixel_t* p) {
	if (len > 0xffff) len = 0xffff;
	game_spr_add(x, y, len, 1, (void*)(uintptr_t)(len | (head_left ? GAME_LASER_HEAD_LEFT : 0)), p, laser_renderer);
}

void game_spr_bitmap(int32_t y, uint32_t rows, const uint32_t* bits, hstx_dvi_pixel_t* p) {
	game_spr_add(0, y, GAME_SCREEN_W, rows, (void*)bits, p, bitmap_renderer);
}

void game_spr_pixels(int32_t x, int32_t y, uint32_t w, uint32_t h, const GamePixels_t* pixels) {
	game_spr_add(x, y, w, h, (void*)pixels, NULL, pixels_renderer);
}

void game_text_set_n(GameText_t* t, const char* s, uint32_t n, uint32_t scale) {
	uint32_t len = strlen(s);
	if (len > GAME_TEXT_MAX) len = GAME_TEXT_MAX;
	for (uint32_t i = 0; i < len; ++i) {
		t->s[i] = i < n ? s[i] : ' ';
	}
	t->grid.w = len;
	t->grid.s = t->s;
	t->grid.font = (uint8_t*)font_8x8;
	t->scale = scale ? scale : 1;
}

void game_text_set(GameText_t* t, const char* s, uint32_t scale) {
	game_text_set_n(t, s, GAME_TEXT_MAX, scale);
}

int32_t game_text_width(const GameText_t* t) {
	return t->grid.w * 8 * t->scale;
}

void game_spr_text(GameText_t* t, int32_t x, int32_t y, hstx_dvi_pixel_t* p) {
	if (!t->grid.w) return;
	const uint32_t s = t->scale;
	if (s == 1) {
		game_spr_add(x, y, t->grid.w << 3, 8, &t->grid, p, text_renderer_8x8_p1);
	}
	else {
		game_spr_add(x, y, (t->grid.w << 3) * s, 8 * s, t, p, text_scaled_renderer);
	}
}

void game_spr_text_centre(GameText_t* t, int32_t y, hstx_dvi_pixel_t* p) {
	// Word aligned, so the text renderer can take its fast path
	game_spr_text(t, ((GAME_SCREEN_W - game_text_width(t)) >> 1) & ~3, y, p);
}
