#include "ww_tiles.h"
#include "game_util.h"

// The pictures, one byte per pixel. Bushes and water join up with their
// neighbours, with a picture for each combination of sides that join.
#define WW_GFX_GRASS 0
#define WW_GFX_FLOWERS 2
#define WW_GFX_MUSHROOM 4
#define WW_GFX_ROCK 5
#define WW_GFX_TREE 6
#define WW_GFX_BUSH 10
#define WW_GFX_BUSH_VARIANTS 2
#define WW_GFX_WATER (WW_GFX_BUSH + 16 * WW_GFX_BUSH_VARIANTS)
#define WW_GFX_COUNT (WW_GFX_WATER + 16)
// The water ripples between two sets of pictures
#define WW_GFX_STORE (WW_GFX_COUNT + 16)
#define WW_TILE_WORDS (WW_TILE * WW_TILE / 4)

#define WW_JOIN_N 1
#define WW_JOIN_E 2
#define WW_JOIN_S 4
#define WW_JOIN_W 8

#define RGB(r, g, b) HSTX_DVI_PIXEL_RGB(r, g, b)

static uint32_t _gfx[WW_GFX_STORE][WW_TILE_WORDS];
static const uint32_t* _tile[WW_GFX_COUNT];
static uint32_t _ripple = 0;

static uint32_t hash(uint32_t x, uint32_t y, uint32_t s) {
	uint32_t h = x * 0x9e3779b1u ^ (y + 0x7f4a7c15u) * 0x85ebca6bu ^ s * 0xc2b2ae35u;
	h ^= h >> 15;
	h *= 0x2c1b3c6du;
	h ^= h >> 12;
	h *= 0x297a2d39u;
	return h ^ (h >> 15);
}

static __force_inline void put(uint32_t t, int32_t x, int32_t y, uint8_t p) {
	((uint8_t*)_gfx[t])[(y << WW_TILE_SHIFT) + x] = p;
}

// For patterns that repeat every size pixels, a power of 2
static __force_inline int32_t wrap(int32_t d, int32_t size) {
	return ((d + (size >> 1)) & (size - 1)) - (size >> 1);
}

static uint8_t grass(int32_t x, int32_t y, uint32_t seed) {
	const uint32_t h = hash(x, y, seed);
	// Little blades, two pixels tall
	const uint32_t b = hash(x, y >> 1, seed + 1);
	if ((b & 31) == 0) return (y & 1) ? RGB(32, 96, 0) : RGB(64, 128, 0);
	if ((h & 15) == 0) return RGB(0, 32, 0);
	if ((h & 15) == 1) return RGB(0, 96, 0);
	return RGB(0, 64, 0);
}

static void fill_grass(uint32_t t, uint32_t seed) {
	for (int32_t y = 0; y < WW_TILE; ++y) {
		for (int32_t x = 0; x < WW_TILE; ++x) put(t, x, y, grass(x, y, seed));
	}
}

// A five petal flower centred at cx, cy
static void flower(uint32_t t, int32_t cx, int32_t cy, uint8_t petal) {
	static const int8_t petals[][2] = { {0,-2}, {-1,-2}, {2,-1}, {2,0}, {-2,-1}, {-2,0}, {-1,2}, {1,2}, {0,1}, {-1,1}, {1,-2}, {1,1} };
	for (uint32_t i = 0; i < sizeof(petals) / sizeof(petals[0]); ++i) {
		put(t, cx + petals[i][0], cy + petals[i][1], petal);
	}
	put(t, cx, cy, RGB(255, 224, 0));
	put(t, cx, cy + 3, RGB(0, 160, 0));
	put(t, cx, cy + 4, RGB(0, 160, 0));
}

static void mushroom(uint32_t t, int32_t cx, int32_t cy, int32_t r) {
	for (int32_t y = -r; y <= 0; ++y) {
		for (int32_t x = -r - 1; x <= r + 1; ++x) {
			if (x * x * 4 + y * y * 9 > (r + 1) * (r + 1) * 4) continue;
			const bool dot = ((x + 8) % 4 == 1 && (y + 8) % 3 == 1);
			put(t, cx + x, cy + y, dot ? RGB(255, 255, 255) : RGB(224, 0, 0));
		}
	}
	for (int32_t y = 1; y <= r; ++y) {
		for (int32_t x = -1; x <= 1; ++x) put(t, cx + x, cy + y, RGB(224, 224, 192));
	}
}

// How far each side of a bush is pulled in along its length, so it bulges
// out in a row of round lumps
static const int8_t _scallop[8] = { 4, 2, 1, 0, 0, 1, 2, 4 };
// The gentler wobble of a pond's bank
static const int8_t _bank[8] = { 0, 1, 2, 2, 1, 0, 1, 2 };

// Whether a pixel is inside a bush or pond, given which sides join up.
// The open sides are pulled in by the wobble and their corners rounded.
static bool blob_inside(int32_t x, int32_t y, uint32_t join, int32_t margin, int32_t r, const int8_t* wobble) {
	if (!(join & WW_JOIN_N) && y < margin + wobble[x & 7]) return false;
	if (!(join & WW_JOIN_S) && y > 31 - margin - wobble[(x + 4) & 7]) return false;
	if (!(join & WW_JOIN_W) && x < margin + wobble[(y + 4) & 7]) return false;
	if (!(join & WW_JOIN_E) && x > 31 - margin - wobble[y & 7]) return false;
	const int32_t lo = margin + r;
	const int32_t hi = 31 - margin - r;
	int32_t dx = 0, dy = 0;
	if (!(join & WW_JOIN_W) && x < lo) dx = lo - x;
	if (!(join & WW_JOIN_E) && x > hi) dx = x - hi;
	if (!(join & WW_JOIN_N) && y < lo) dy = lo - y;
	if (!(join & WW_JOIN_S) && y > hi) dy = y - hi;
	// Only round the corner if both sides by it are open
	if (dx && dy && dx * dx + dy * dy > r * r) return false;
	return true;
}

// Round clumps of leaves lit from the top left, repeating every size pixels
// so neighbouring bushes run together. shades are light, middle and dark,
// with the dark showing in the gaps between them.
static uint8_t clumps(int32_t x, int32_t y, uint32_t seed, uint32_t count, int32_t size, const uint8_t* shades) {
	uint8_t c = shades[2];
	for (uint32_t k = 0; k < count; ++k) {
		const int32_t r = 7 + (int32_t)(hash(k, 3, seed) % 3);
		const int32_t dx = wrap(x - (int32_t)(hash(k, 1, seed) % size), size);
		const int32_t dy = wrap(y - (int32_t)(hash(k, 2, seed) % size), size);
		const int32_t d2 = dx * dx + dy * dy;
		if (d2 > r * r) continue;
		if (d2 > (r - 1) * (r - 1) && dx + dy > 0) c = shades[2];
		else if (dx + dy < -r / 2) c = shades[0];
		else c = shades[1];
	}
	return c;
}

static void make_bush(uint32_t t, uint32_t join, uint32_t variant) {
	static const uint8_t shades[2][3] = {
		{ RGB(96, 224, 0), RGB(0, 176, 0), RGB(0, 112, 0) },
		{ RGB(64, 224, 64), RGB(0, 160, 32), RGB(0, 96, 32) },
	};
	const uint32_t seed = 100 + variant * 17;
	static bool in[WW_TILE + 2][WW_TILE + 2];
	for (int32_t y = -1; y <= WW_TILE; ++y) {
		for (int32_t x = -1; x <= WW_TILE; ++x) {
			in[y + 1][x + 1] = x >= 0 && y >= 0 && x < WW_TILE && y < WW_TILE
				? blob_inside(x, y, join, 2, 12, _scallop)
				: true;
		}
	}
	for (int32_t y = 0; y < WW_TILE; ++y) {
		for (int32_t x = 0; x < WW_TILE; ++x) {
			uint8_t p;
			if (!in[y + 1][x + 1]) {
				p = grass(x, y, 7);
			}
			else if (!in[y][x + 1] || !in[y + 2][x + 1] || !in[y + 1][x] || !in[y + 1][x + 2]) {
				p = RGB(0, 32, 0);
			}
			else {
				p = clumps(x, y, seed, 9, WW_TILE, shades[variant]);
			}
			put(t, x, y, p);
		}
	}
}

static void make_water(uint32_t t, uint32_t join, uint32_t phase) {
	for (int32_t y = 0; y < WW_TILE; ++y) {
		for (int32_t x = 0; x < WW_TILE; ++x) {
			uint8_t p;
			if (!blob_inside(x, y, join, 2, 10, _bank)) {
				p = grass(x, y, 7);
			}
			else if (!blob_inside(x, y, join, 4, 10, _bank)) {
				p = RGB(128, 96, 32);
			}
			else {
				p = RGB(0, 64, 192);
				// Ripples drifting across
				const int32_t rx = (x + (int32_t)phase * 3 + (y >> 2) * 11) & 15;
				if ((y & 3) == 1 && rx < 4) p = RGB(64, 160, 255);
				else if ((y & 3) == 2 && rx >= 4 && rx < 6) p = RGB(0, 32, 128);
			}
			put(t, x, y, p);
		}
	}
	// A lily pad in the middle of big ponds
	if (join == 15) {
		for (int32_t y = -4; y <= 4; ++y) {
			for (int32_t x = -5; x <= 5; ++x) {
				if (x * x + y * y * 2 > 30 || (x > 0 && y == 0)) continue;
				put(t, 15 + x, 15 + y, RGB(0, 160, 0));
			}
		}
		flower(t, 13, 13, RGB(255, 128, 192));
		put(t, 13, 16, RGB(0, 160, 0));
		put(t, 13, 17, RGB(0, 160, 0));
	}
}

static void make_rock(uint32_t t) {
	fill_grass(t, 9);
	for (int32_t y = 0; y < WW_TILE; ++y) {
		for (int32_t x = 0; x < WW_TILE; ++x) {
			const int32_t dx = x - 16, dy = y - 18;
			// Its shadow, then the rock
			const int32_t sx = x - 19, sy = y - 21;
			const int32_t e = dx * dx * 100 + dy * dy * 169;
			if (e > 16900) {
				if (sx * sx * 100 + sy * sy * 169 <= 16900) put(t, x, y, RGB(0, 32, 0));
				continue;
			}
			uint8_t p = RGB(128, 128, 128);
			if (e > 13000) p = RGB(32, 32, 32);
			else if (dx + dy < -9) p = RGB(192, 192, 192);
			else if (dx + dy > 8) p = RGB(64, 64, 64);
			// A crack
			if ((x == 20 && y >= 10 && y < 15) || (x == 21 && y >= 15 && y < 19)) p = RGB(64, 64, 64);
			put(t, x, y, p);
		}
	}
}

// A tree in a 64 pixel square, split across 4 tiles. The leaves are a
// brighter, yellower green than the bushes, in a lumpy cloud of a canopy.
#define WW_TREE_LOBES 9

static void make_tree(void) {
	static const uint8_t shades[3] = { RGB(160, 255, 64), RGB(96, 208, 32), RGB(0, 144, 32) };
	int32_t lx[WW_TREE_LOBES], ly[WW_TREE_LOBES], lr[WW_TREE_LOBES];
	for (uint32_t k = 0; k < WW_TREE_LOBES; ++k) {
		const uint32_t a = k * 256 / WW_TREE_LOBES + hash(k, 5, 7) % 12;
		lx[k] = 32 + game_sin(a + 64) * 17 / 127;
		ly[k] = 27 + game_sin(a) * 15 / 127;
		lr[k] = 8 + hash(k, 6, 7) % 3;
	}
	for (uint32_t q = 0; q < 4; ++q) fill_grass(WW_GFX_TREE + q, 11 + q);
	for (int32_t y = 0; y < 2 * WW_TILE; ++y) {
		for (int32_t x = 0; x < 2 * WW_TILE; ++x) {
			const uint32_t t = WW_GFX_TREE + (y >> WW_TILE_SHIFT) * 2 + (x >> WW_TILE_SHIFT);
			const int32_t tx = x & 31, ty = y & 31;
			const int32_t dx = x - 32, dy = y - 27;
			bool leafy = dx * dx + dy * dy <= 19 * 19;
			for (uint32_t k = 0; k < WW_TREE_LOBES && !leafy; ++k) {
				const int32_t ex = x - lx[k], ey = y - ly[k];
				leafy = ex * ex + ey * ey <= lr[k] * lr[k];
			}
			uint8_t p = 0;
			if (leafy) {
				p = clumps(x, y, 300, 30, 2 * WW_TILE, shades);
				// Shaded away from the light
				if (dx + dy > 16) p = shades[2];
			}
			else if (x >= 27 && x <= 36 && y >= 44) {
				// The trunk, with bark lines and roots
				p = (x == 29 || x == 33) && (y & 3) ? RGB(64, 32, 0) : RGB(160, 96, 32);
				if (x == 27 || x == 36) p = RGB(64, 32, 0);
			}
			else if (y >= 58 && x >= 23 && x <= 40 && (x < 27 ? y >= 58 + (27 - x) : x > 36 ? y >= 58 + (x - 36) : 1)) {
				p = RGB(128, 64, 0);
			}
			else if (dx * dx + (y - 57) * (y - 57) * 4 <= 22 * 22) {
				p = RGB(0, 32, 0);
			}
			if (p) put(t, tx, ty, p);
		}
	}
}

void ww_tiles_init(void) {
	fill_grass(WW_GFX_GRASS, 1);
	fill_grass(WW_GFX_GRASS + 1, 2);
	fill_grass(WW_GFX_FLOWERS, 3);
	flower(WW_GFX_FLOWERS, 8, 9, RGB(255, 96, 192));
	flower(WW_GFX_FLOWERS, 22, 6, RGB(255, 255, 255));
	flower(WW_GFX_FLOWERS, 18, 22, RGB(96, 128, 255));
	fill_grass(WW_GFX_FLOWERS + 1, 4);
	flower(WW_GFX_FLOWERS + 1, 10, 20, RGB(255, 255, 255));
	flower(WW_GFX_FLOWERS + 1, 24, 14, RGB(255, 224, 0));
	flower(WW_GFX_FLOWERS + 1, 6, 6, RGB(255, 96, 192));
	fill_grass(WW_GFX_MUSHROOM, 5);
	mushroom(WW_GFX_MUSHROOM, 12, 14, 5);
	mushroom(WW_GFX_MUSHROOM, 22, 22, 3);
	make_rock(WW_GFX_ROCK);
	make_tree();
	for (uint32_t v = 0; v < WW_GFX_BUSH_VARIANTS; ++v) {
		for (uint32_t j = 0; j < 16; ++j) make_bush(WW_GFX_BUSH + v * 16 + j, j, v);
	}
	for (uint32_t j = 0; j < 16; ++j) {
		make_water(WW_GFX_WATER + j, j, 0);
		make_water(WW_GFX_COUNT + j, j, 1);
	}
	for (uint32_t i = 0; i < WW_GFX_COUNT; ++i) _tile[i] = _gfx[i];
}

static uint8_t logic_at(const uint8_t* logic, int32_t c, int32_t r) {
	// Off the edge of the room counts as bush, so the edges stay flat
	if (c < 0 || r < 0 || c >= WW_ROOM_COLS || r >= WW_ROOM_ROWS) return WW_L_BUSH;
	return logic[r * WW_ROOM_COLS + c] & WW_L_TYPE;
}

static uint32_t joins(const uint8_t* logic, int32_t c, int32_t r, uint8_t type) {
	return (logic_at(logic, c, r - 1) == type ? WW_JOIN_N : 0)
		| (logic_at(logic, c + 1, r) == type ? WW_JOIN_E : 0)
		| (logic_at(logic, c, r + 1) == type ? WW_JOIN_S : 0)
		| (logic_at(logic, c - 1, r) == type ? WW_JOIN_W : 0);
}

void ww_tiles_build(uint8_t* gfx, const uint8_t* logic, uint32_t seed) {
	for (int32_t r = 0; r < WW_ROOM_ROWS; ++r) {
		for (int32_t c = 0; c < WW_ROOM_COLS; ++c) {
			const uint8_t l = logic_at(logic, c, r);
			const uint32_t h = hash(c, r, seed);
			uint8_t g;
			switch (l) {
				case WW_L_FLOWERS: g = WW_GFX_FLOWERS + (h & 1); break;
				case WW_L_MUSHROOM: g = WW_GFX_MUSHROOM; break;
				case WW_L_BUSH: g = WW_GFX_BUSH + (h % WW_GFX_BUSH_VARIANTS) * 16 + joins(logic, c, r, WW_L_BUSH); break;
				case WW_L_ROCK: g = WW_GFX_ROCK; break;
				case WW_L_WATER: g = WW_GFX_WATER + joins(logic, c, r, WW_L_WATER); break;
				case WW_L_TREE: case WW_L_TREE + 1: case WW_L_TREE + 2: case WW_L_TREE + 3:
					g = WW_GFX_TREE + (l - WW_L_TREE); break;
				default: g = WW_GFX_GRASS + (h & 1); break;
			}
			gfx[r * WW_ROOM_COLS + c] = g;
		}
	}
}

static void __not_in_flash_func(room_renderer)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	const uint8_t* m = (const uint8_t*)d1 + (row >> WW_TILE_SHIFT) * WW_ROOM_COLS;
	const uint32_t* const* t = (const uint32_t* const*)d2;
	const uint32_t k = (row & (WW_TILE - 1)) << 3;
	uint32_t* dst = r->w;
	for (uint32_t i = 0; i < WW_ROOM_COLS; ++i, dst += 8) {
		const uint32_t* s = t[m[i]] + k;
		dst[0] = s[0]; dst[1] = s[1]; dst[2] = s[2]; dst[3] = s[3];
		dst[4] = s[4]; dst[5] = s[5]; dst[6] = s[6]; dst[7] = s[7];
	}
}

void ww_tiles_draw(const uint8_t* gfx, uint32_t frame) {
	const uint32_t ripple = (frame >> 5) & 1;
	if (ripple != _ripple) {
		_ripple = ripple;
		for (uint32_t j = 0; j < 16; ++j) _tile[WW_GFX_WATER + j] = _gfx[(ripple ? WW_GFX_COUNT : WW_GFX_WATER) + j];
	}
	game_spr_add(0, WW_HUD_H, WW_ROOM_W, WW_ROOM_H, (void*)gfx, (void*)_tile, room_renderer);
}
