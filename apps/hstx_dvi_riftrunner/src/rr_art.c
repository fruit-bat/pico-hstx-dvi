// Copyright (c) 2024 fruit-bat.

// See rr_art.h.

#include "rr_art.h"
#include "game_render.h"
#include <string.h>
#include <stdio.h>

// Room for all the game's pictures
#define RR_ART_RUN_POOL 6144
#define RR_ART_ROW_POOL 1024

static RrRun_t _runs[RR_ART_RUN_POOL];
static uint16_t _rows[RR_ART_ROW_POOL];
static uint32_t _runs_used = 0;
static uint32_t _rows_used = 0;

// Only the renderer, on the other core, uses this
static SpriteSpan_t _spans[RR_ART_MAX_RUNS];

void rr_art_make(RrArt_t* a, const char* const* rows, uint32_t h, const char* key,
	const hstx_dvi_pixel_t* pixels, uint32_t scale, bool flip) {
	if (h > RR_ART_MAX_H) h = RR_ART_MAX_H;
	a->scale = scale < 1 ? 1 : scale > 8 ? 8 : scale;
	a->h = h;
	a->w = 0;
	if (_rows_used + h + 1 > RR_ART_ROW_POOL) {
		printf("rr_art: out of rows\n");
		a->h = 0;
		a->row = _rows;
		a->run = _runs;
		return;
	}
	uint16_t* row = &_rows[_rows_used];
	_rows_used += h + 1;
	RrRun_t* run = &_runs[_runs_used];
	uint32_t n = 0;
	for (uint32_t r = 0; r < h; ++r) {
		row[r] = n;
		const char* s = rows[flip ? h - 1 - r : r];
		const uint32_t len = strlen(s);
		if (len > a->w) a->w = len > 255 ? 255 : len;
		uint32_t in_row = 0;
		for (uint32_t x = 0; x < len && x < 255; ) {
			const char* k = strchr(key, s[x]);
			if (!s[x] || !k) {
				++x;
				continue;
			}
			uint32_t e = x + 1;
			while (e < len && e < 255 && s[e] == s[x]) ++e;
			if (in_row < RR_ART_MAX_RUNS && _runs_used + n < RR_ART_RUN_POOL) {
				run[n].x0 = x;
				run[n].x1 = e;
				run[n].p = pixels[k - key];
				++n;
				++in_row;
			}
			x = e;
		}
	}
	row[h] = n;
	_runs_used += n;
	a->row = row;
	a->run = run;
}

// d1 is the picture, d2 the flash colour plus one, or 0 for none
static void __not_in_flash_func(art_renderer)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	const RrArt_t* a = (const RrArt_t*)d1;
	const uint32_t sc = a->scale;
	const uint32_t sr = (uint32_t)row / sc;
	if (sr >= a->h) return;
	const uint32_t i0 = a->row[sr], i1 = a->row[sr + 1];
	if (i0 == i1) return;
	const uint32_t flash = (uint32_t)(uintptr_t)d2;
	uint32_t n = 0;
	for (uint32_t i = i0; i < i1; ++i) {
		const RrRun_t* u = &a->run[i];
		_spans[n].x0 = (int16_t)(x + u->x0 * sc);
		_spans[n].x1 = (int16_t)(x + u->x1 * sc);
		_spans[n].p = flash ? (hstx_dvi_pixel_t)(flash - 1) : u->p;
		++n;
	}
	sprite_render_spans(r, _spans, n, spriteId);
}

void __not_in_flash_func(rr_art_draw)(const RrArt_t* a, int32_t sx, int32_t sy) {
	game_spr_add(sx, sy, rr_art_w(a), rr_art_h(a), (void*)a, NULL, art_renderer);
}

void __not_in_flash_func(rr_art_draw_flash)(const RrArt_t* a, int32_t sx, int32_t sy, hstx_dvi_pixel_t flash) {
	game_spr_add(sx, sy, rr_art_w(a), rr_art_h(a), (void*)a, (void*)(uintptr_t)(flash + 1u), art_renderer);
}

// d1 is the width, d2 the colour
static void __not_in_flash_func(rect_renderer)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	sprite_render_span(r, x, x + (int32_t)(uintptr_t)d1, (hstx_dvi_pixel_t)(uintptr_t)d2, spriteId);
}

void __not_in_flash_func(rr_spr_rect)(int32_t x, int32_t y, uint32_t w, uint32_t h, hstx_dvi_pixel_t p) {
	if (!w || !h) return;
	game_spr_add(x, y, w, h, (void*)(uintptr_t)w, (void*)(uintptr_t)p, rect_renderer);
}
