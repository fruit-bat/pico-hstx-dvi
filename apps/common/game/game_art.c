#include "game_art.h"
#include "game_render.h"
#include "game_pallet.h"
#include <string.h>

static __force_inline uint32_t art_bit(const uint32_t w, const uint32_t c) {
	return 1u << (w - 1 - c);
}

static __force_inline uint32_t art_row(const GameArt_t* a, const uint32_t l, const uint32_t r) {
	return a->w == 16 ? a->tile[l].t16.d[r] : a->tile[l].t32.d[r];
}

static __force_inline void art_set_row(GameArt_t* a, const uint32_t l, const uint32_t r, const uint32_t v) {
	if (a->w == 16) a->tile[l].t16.d[r] = v;
	else a->tile[l].t32.d[r] = v;
}

void game_art_make(GameArt_t* a, const char* const* rows, uint32_t h, const char* key, const uint8_t* colours) {
	memset(a, 0, sizeof(GameArt_t));
	const uint32_t len = strlen(rows[0]);
	a->w = len <= 16 ? 16 : 32;
	a->h = h < GAME_ART_MAX_H ? h : GAME_ART_MAX_H;
	a->layers = strlen(key) < GAME_ART_LAYERS ? strlen(key) : GAME_ART_LAYERS;
	for (uint32_t l = 0; l < a->layers; ++l) a->colour[l] = colours[l];
	for (uint32_t r = 0; r < a->h; ++r) {
		const char* row = rows[r];
		for (uint32_t c = 0; c < a->w && row[c]; ++c) {
			const char* k = strchr(key, row[c]);
			if (!k || row[c] == 0) continue;
			const uint32_t l = k - key;
			if (l < a->layers) art_set_row(a, l, r, art_row(a, l, r) | art_bit(a->w, c));
		}
	}
}

void game_art_mirror(GameArt_t* dst, const GameArt_t* src) {
	*dst = *src;
	for (uint32_t l = 0; l < src->layers; ++l) {
		for (uint32_t r = 0; r < src->h; ++r) {
			const uint32_t v = art_row(src, l, r);
			uint32_t m = 0;
			for (uint32_t c = 0; c < src->w; ++c) {
				if (v & art_bit(src->w, c)) m |= art_bit(src->w, src->w - 1 - c);
			}
			art_set_row(dst, l, r, m);
		}
	}
}

static __force_inline SpriteRenderer art_renderer(const GameArt_t* a) {
	return a->w == 16 ? sprite_renderer_sprite_16x16_p1 : sprite_renderer_sprite_32x16_p1;
}

void __not_in_flash_func(game_art_sprites)(SpriteId first, uint32_t n, const GameArt_t* a, int32_t sx, int32_t sy, const uint8_t* colours) {
	for (uint32_t l = 0; l < n; ++l) {
		Sprite* s = hstx_dvi_sprite_get(first + l);
		if (l >= a->layers) {
			hstx_dvi_sprite_disable_1(s);
			continue;
		}
		s->x = sx;
		s->y = sy;
		s->w = a->w;
		s->h = a->h;
		s->d1 = (void*)&a->tile[l];
		s->d2 = game_pallet(colours ? colours[l] : a->colour[l]);
		s->r = art_renderer(a);
		s->f |= SF_ENABLE;
	}
}

void game_art_sprites_hide(SpriteId first, uint32_t n) {
	for (uint32_t l = 0; l < n; ++l) hstx_dvi_sprite_disable(first + l);
}

void game_art_canvas_clear(GameArtCanvas_t* c, uint32_t w, uint32_t h) {
	c->w = w < 32 ? w : 32;
	c->h = h < GAME_ART_MAX_H ? h : GAME_ART_MAX_H;
	for (uint32_t r = 0; r < c->h; ++r) {
		memset(c->c[r], '.', c->w);
		c->c[r][c->w] = 0;
		c->rows[r] = c->c[r];
	}
}

void game_art_canvas_set(GameArtCanvas_t* c, int32_t x, int32_t y, char ch) {
	if (x < 0 || y < 0 || x >= (int32_t)c->w || y >= (int32_t)c->h || ch == '.') return;
	c->c[y][x] = ch == ' ' ? '.' : ch;
}

void game_art_canvas_stamp(GameArtCanvas_t* c, const char* const* rows, uint32_t n, int32_t x, int32_t y) {
	for (uint32_t r = 0; r < n; ++r) {
		for (int32_t i = 0; rows[r][i]; ++i) game_art_canvas_set(c, x + i, y + r, rows[r][i]);
	}
}

void game_art_canvas_make(GameArt_t* a, GameArtCanvas_t* c, const char* key, const uint8_t* colours) {
	game_art_make(a, c->rows, c->h, key, colours);
}

void __not_in_flash_func(game_art_draw)(const GameArt_t* a, int32_t sx, int32_t sy, const uint8_t* colours) {
	const SpriteRenderer r = art_renderer(a);
	for (uint32_t l = 0; l < a->layers; ++l) {
		game_spr_add(sx, sy, a->w, a->h, (void*)&a->tile[l], game_pallet(colours ? colours[l] : a->colour[l]), r);
	}
}
