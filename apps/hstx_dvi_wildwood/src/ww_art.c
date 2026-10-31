#include "ww_art.h"
#include "game_render.h"
#include "game_pallet.h"
#include <string.h>

static int32_t _clip = 0;

void ww_canvas_clear(WwCanvas_t* c, uint32_t w, uint32_t h) {
	c->w = w < WW_ART_MAX_W ? w : WW_ART_MAX_W;
	c->h = h < WW_ART_MAX_H ? h : WW_ART_MAX_H;
	for (uint32_t r = 0; r < c->h; ++r) {
		memset(c->c[r], '.', c->w);
		c->c[r][c->w] = 0;
	}
}

void ww_canvas_set(WwCanvas_t* c, int32_t x, int32_t y, char ch) {
	if (x < 0 || y < 0 || x >= (int32_t)c->w || y >= (int32_t)c->h || ch == '.') return;
	c->c[y][x] = ch == ' ' ? '.' : ch;
}

void ww_canvas_stamp(WwCanvas_t* c, const char* const* rows, uint32_t n, int32_t x, int32_t y) {
	for (uint32_t r = 0; r < n; ++r) {
		for (int32_t i = 0; rows[r][i]; ++i) ww_canvas_set(c, x + i, y + r, rows[r][i]);
	}
}

void ww_canvas_mirror(WwCanvas_t* c) {
	for (uint32_t r = 0; r < c->h; ++r) {
		for (uint32_t i = 0, j = c->w - 1; i < j; ++i, --j) {
			const char t = c->c[r][i];
			c->c[r][i] = c->c[r][j];
			c->c[r][j] = t;
		}
	}
}

void ww_art_make(WwArt_t* a, const WwCanvas_t* c, const char* key, const uint8_t* colours) {
	memset(a, 0, sizeof(WwArt_t));
	a->w = c->w;
	a->h = c->h;
	a->parts = c->h > GAME_ART_MAX_H ? 2 : 1;
	const uint32_t nk = strlen(key) < WW_ART_KEY_MAX ? strlen(key) : WW_ART_KEY_MAX;
	for (uint32_t p = 0; p < a->parts; ++p) {
		const uint32_t r0 = p * GAME_ART_MAX_H;
		const uint32_t n = c->h - r0 < GAME_ART_MAX_H ? c->h - r0 : GAME_ART_MAX_H;
		// Only the key characters this half uses
		char k[GAME_ART_LAYERS + 1];
		uint8_t cols[GAME_ART_LAYERS];
		uint32_t layers = 0;
		for (uint32_t i = 0; i < nk && layers < GAME_ART_LAYERS; ++i) {
			bool used = false;
			for (uint32_t r = r0; r < r0 + n && !used; ++r) used = strchr(c->c[r], key[i]) != NULL;
			if (!used) continue;
			k[layers] = key[i];
			cols[layers] = colours[i];
			a->key[p][layers] = i;
			++layers;
		}
		k[layers] = 0;
		const char* rows[GAME_ART_MAX_H];
		for (uint32_t r = 0; r < n; ++r) rows[r] = c->c[r0 + r];
		game_art_make(&a->part[p], rows, n, k, cols);
	}
}

void ww_art_set_clip(int32_t top) {
	_clip = top;
}

// Adds one layer of a picture, leaving off any rows above the clip
static void __not_in_flash_func(add_layer)(const GameArt_t* g, uint32_t l, int32_t x, int32_t y, uint32_t colour) {
	const int32_t k = y < _clip ? _clip - y : 0;
	if (k >= g->h) return;
	const uint32_t stride = g->w == 16 ? sizeof(uint16_t) : sizeof(uint32_t);
	game_spr_add(x, y + k, g->w, g->h - k, (uint8_t*)&g->tile[l] + k * stride, game_pallet(colour),
		g->w == 16 ? sprite_renderer_sprite_16x16_p1 : sprite_renderer_sprite_32x16_p1);
}

void __not_in_flash_func(ww_art_draw)(const WwArt_t* a, int32_t sx, int32_t sy, const uint8_t* colours) {
	for (uint32_t p = 0; p < a->parts; ++p) {
		const GameArt_t* g = &a->part[p];
		for (uint32_t l = 0; l < g->layers; ++l) {
			add_layer(g, l, sx, sy + p * GAME_ART_MAX_H, colours ? colours[a->key[p][l]] : g->colour[l]);
		}
	}
}

void __not_in_flash_func(ww_art_draw_small)(const GameArt_t* a, int32_t sx, int32_t sy, const uint8_t* colours) {
	for (uint32_t l = 0; l < a->layers; ++l) {
		add_layer(a, l, sx, sy, colours ? colours[l] : a->colour[l]);
	}
}
