#include "mm_gfx.h"
#include "game_render.h"
#include <string.h>

#if MODE_BYTES_PER_PIXEL != 1
#error "Moonlit Manor draws one byte per pixel"
#endif

void mm_canvas_clear(MmCanvas_t* c, uint32_t w, uint32_t h) {
	c->w = w < MM_IMG_MAX_W ? w : MM_IMG_MAX_W;
	c->h = h < MM_IMG_MAX_H ? h : MM_IMG_MAX_H;
	for (uint32_t r = 0; r < c->h; ++r) {
		memset(c->c[r], '.', c->w);
		c->c[r][c->w] = 0;
	}
}

void mm_canvas_set(MmCanvas_t* c, int32_t x, int32_t y, char ch) {
	if (x < 0 || y < 0 || x >= (int32_t)c->w || y >= (int32_t)c->h || ch == '.') return;
	c->c[y][x] = ch == ' ' ? '.' : ch;
}

void mm_canvas_stamp(MmCanvas_t* c, const char* const* rows, uint32_t n, int32_t x, int32_t y) {
	for (uint32_t r = 0; r < n; ++r) {
		for (int32_t i = 0; rows[r][i]; ++i) mm_canvas_set(c, x + i, y + r, rows[r][i]);
	}
}

void mm_img_make(MmImg_t* img, const char* const* rows, uint32_t h, const char* key) {
	memset(img, 0, sizeof(MmImg_t));
	const uint32_t len = strlen(rows[0]);
	img->w = len < MM_IMG_MAX_W ? len : MM_IMG_MAX_W;
	img->h = h < MM_IMG_MAX_H ? h : MM_IMG_MAX_H;
	img->words = (img->w + 7) >> 3;
	for (uint32_t r = 0; r < img->h; ++r) {
		for (uint32_t x = 0; x < img->w && rows[r][x]; ++x) {
			const char* k = strchr(key, rows[r][x]);
			if (!k) continue;
			const uint32_t n = (uint32_t)(k - key) + 1;
			if (n >= MM_PAL_SIZE) continue;
			img->d[r * MM_IMG_WORDS + (x >> 3)] |= n << ((x & 7) << 2);
		}
	}
}

void mm_img_from_canvas(MmImg_t* img, const MmCanvas_t* c, const char* key) {
	const char* rows[MM_IMG_MAX_H];
	for (uint32_t r = 0; r < c->h; ++r) rows[r] = c->c[r];
	mm_img_make(img, rows, c->h, key);
}

// Halfway between two colours, a channel at a time
static __force_inline uint32_t blend(const uint32_t a, const uint32_t b) {
	return (a & b) + (((a ^ b) & 0xda) >> 1);
}

static __force_inline void render_img(
	const MmImg_t* img,
	const hstx_dvi_pixel_t* pal,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const uint32_t shift,
	const bool flip,
	const bool glass
) {
	const uint32_t* src = img->d + (row >> shift) * MM_IMG_WORDS;
	const int32_t w = img->w;
	const bool clip = x < 0 || x + (w << shift) > GAME_SCREEN_W;
	for (uint32_t wi = 0; wi < img->words; ++wi) {
		int32_t i = wi << 3;
		for (uint32_t v = src[wi]; v; v >>= 4, ++i) {
			const uint32_t n = v & 15;
			if (!n) continue;
			const uint32_t p = pal[n];
			const int32_t px = x + ((flip ? w - 1 - i : i) << shift);
			for (uint32_t k = 0; k < (1u << shift); ++k) {
				const uint32_t q = (uint32_t)(px + (int32_t)k);
				if (clip && q >= GAME_SCREEN_W) continue;
				r->b[q] = glass ? blend(r->b[q], p) : p;
			}
		}
	}
}

#define MM_RENDERER(name, shift, flip, glass) \
	static void __not_in_flash_func(name)(const void* d1, const void* d2, hstx_dvi_row_t* r, const int32_t x, const int32_t row, const SpriteId id) { \
		render_img((const MmImg_t*)d1, (const hstx_dvi_pixel_t*)d2, r, x, row, shift, flip, glass); \
	}

MM_RENDERER(render_1, 0, false, false)
MM_RENDERER(render_1f, 0, true, false)
MM_RENDERER(render_1g, 0, false, true)
MM_RENDERER(render_1fg, 0, true, true)
MM_RENDERER(render_2, 1, false, false)
MM_RENDERER(render_2f, 1, true, false)
MM_RENDERER(render_2g, 1, false, true)
MM_RENDERER(render_2fg, 1, true, true)
MM_RENDERER(render_4, 2, false, false)
MM_RENDERER(render_4f, 2, true, false)
MM_RENDERER(render_4g, 2, false, true)
MM_RENDERER(render_4fg, 2, true, true)

static const SpriteRenderer _renderers[3][4] = {
	{ render_1, render_1f, render_1g, render_1fg },
	{ render_2, render_2f, render_2g, render_2fg },
	{ render_4, render_4f, render_4g, render_4fg },
};

void __not_in_flash_func(mm_gfx_draw)(const MmImg_t* img, const hstx_dvi_pixel_t* pal, int32_t sx, int32_t sy, uint32_t scale, uint32_t flags) {
	const uint32_t shift = scale >= 4 ? 2 : scale >= 2 ? 1 : 0;
	game_spr_add(sx, sy, img->w << shift, img->h << shift, (void*)img, (void*)pal, _renderers[shift][flags & 3]);
}

// d1 holds the width
static void __not_in_flash_func(rect_renderer)(const void* d1, const void* d2, hstx_dvi_row_t* r, const int32_t x, const int32_t row, const SpriteId id) {
	const int32_t w = (int32_t)(uintptr_t)d1;
	const uint8_t p = ((const hstx_dvi_pixel_t*)d2)[0];
	const int32_t i1 = x + w > GAME_SCREEN_W ? GAME_SCREEN_W : x + w;
	int32_t i = x < 0 ? 0 : x;
	// A word at a time in the middle
	for (; i < i1 && (i & 3); ++i) r->b[i] = p;
	const uint32_t pq = p * 0x01010101u;
	for (; i + 4 <= i1; i += 4) r->w[i >> 2] = pq;
	for (; i < i1; ++i) r->b[i] = p;
}

void mm_gfx_rect(int32_t x, int32_t y, uint32_t w, uint32_t h, const hstx_dvi_pixel_t* colour) {
	game_spr_add(x, y, w, h, (void*)(uintptr_t)w, (void*)colour, rect_renderer);
}

// Moves each channel of a colour towards another
uint32_t mm_gfx_mix(uint32_t a, uint32_t b, uint32_t t) {
	const int32_t r0 = a >> 5, g0 = (a >> 2) & 7, b0 = a & 3;
	const int32_t r1 = b >> 5, g1 = (b >> 2) & 7, b1 = b & 3;
	const int32_t r = r0 + (((r1 - r0) * (int32_t)t + 128) >> 8);
	const int32_t g = g0 + (((g1 - g0) * (int32_t)t + 128) >> 8);
	const int32_t bl = b0 + (((b1 - b0) * (int32_t)t + 128) >> 8);
	return (r << 5) | (g << 2) | bl;
}
