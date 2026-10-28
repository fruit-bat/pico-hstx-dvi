#include "def_art.h"
#include "def_render.h"
#include "def_pallet.h"
#include <string.h>

static __force_inline uint32_t art_bit(const uint32_t w, const uint32_t c) {
	return 1u << (w - 1 - c);
}

static __force_inline uint32_t art_row(const DefArt_t* a, const uint32_t l, const uint32_t r) {
	return a->w == 16 ? a->tile[l].t16.d[r] : a->tile[l].t32.d[r];
}

static __force_inline void art_set_row(DefArt_t* a, const uint32_t l, const uint32_t r, const uint32_t v) {
	if (a->w == 16) a->tile[l].t16.d[r] = v;
	else a->tile[l].t32.d[r] = v;
}

void def_art_make(DefArt_t* a, const char* const* rows, uint32_t h, const char* key, const uint8_t* colours) {
	memset(a, 0, sizeof(DefArt_t));
	const uint32_t len = strlen(rows[0]);
	a->w = len <= 16 ? 16 : 32;
	a->h = h < DEF_ART_MAX_H ? h : DEF_ART_MAX_H;
	a->layers = strlen(key) < DEF_ART_LAYERS ? strlen(key) : DEF_ART_LAYERS;
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

void def_art_mirror(DefArt_t* dst, const DefArt_t* src) {
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

void __not_in_flash_func(def_art_draw)(const DefArt_t* a, int32_t sx, int32_t sy, const uint8_t* colours) {
	const SpriteRenderer r = a->w == 16 ? sprite_renderer_sprite_16x16_p1 : sprite_renderer_sprite_32x16_p1;
	for (uint32_t l = 0; l < a->layers; ++l) {
		def_spr_add(sx, sy, a->w, a->h, (void*)&a->tile[l], def_pallet(colours ? colours[l] : a->colour[l]), r);
	}
}
