#include "mm_tiles.h"
#include "mm_gfx.h"
#include "game_util.h"
#include "game_art.h"
#include <string.h>

// Tile pictures are made as they are first asked for and kept. Flickering
// ones have a second picture, swapped in every few frames.
// The house needs about 75
#define MM_TILE_STORES 100
#define MM_TILE_WORDS (MM_TILE * MM_TILE / 4)
#define MM_TILE_LOGICAL 16
#define MM_FLICKER_SHIFT 3
// The sky repeats every 64 pixels, with the start of each row copied on the end
// so a tile's worth can be copied from anywhere along it
#define MM_SKY 64
#define MM_SKY_ROW (MM_SKY + MM_TILE)

#define RGB(r, g, b) HSTX_DVI_PIXEL_RGB(r, g, b)

typedef struct {
	uint16_t key;
	uint8_t store;
	uint8_t flicker; // The second store, or 0xff
} MmTileEntry_t;

typedef struct {
	const uint8_t* cell;
	uint32_t stride;
	// How far the top row of tiles starts above the view
	int32_t fy;
	int32_t sky_x;
	int32_t sky_y;
} MmView_t;

static uint32_t _store[MM_TILE_STORES][MM_TILE_WORDS];
static uint32_t _stores = 0;
// Entry 0 is the sky, drawn from _sky rather than a picture
static MmTileEntry_t _entry[256];
static uint32_t _entries = 1;
static const uint32_t* _tile[256];
static uint8_t _sky[MM_SKY][MM_SKY_ROW] __attribute__((aligned(4)));
static uint32_t _flicker = 0;
// One for the frame being drawn and one for the frame being made
static MmView_t _view[2];
static uint32_t _view_i = 0;

typedef uint32_t __attribute__((aligned(1), may_alias)) mm_u32u_t;

// Colours for the characters in the pictures below
static uint8_t _ink[128];

static void ink_init(void) {
	_ink['K'] = RGB(0, 0, 0);
	_ink['k'] = RGB(32, 32, 64);
	_ink['g'] = RGB(96, 96, 128);
	_ink['G'] = RGB(160, 160, 192);
	_ink['W'] = RGB(255, 255, 255);
	_ink['d'] = RGB(64, 32, 0);
	_ink['b'] = RGB(128, 64, 0);
	_ink['B'] = RGB(192, 128, 64);
	_ink['n'] = RGB(224, 192, 128);
	_ink['Y'] = RGB(255, 224, 0);
	_ink['y'] = RGB(255, 255, 128);
	_ink['O'] = RGB(255, 128, 0);
	_ink['R'] = RGB(224, 32, 64);
	_ink['r'] = RGB(128, 0, 64);
	_ink['P'] = RGB(160, 64, 192);
	_ink['p'] = RGB(64, 0, 64);
	_ink['N'] = RGB(0, 32, 128);
	_ink['L'] = RGB(64, 96, 192);
	_ink['C'] = RGB(128, 224, 255);
	_ink['E'] = RGB(32, 160, 64);
	_ink['e'] = RGB(0, 96, 0);
	_ink['M'] = RGB(255, 128, 192);
	_ink['m'] = RGB(192, 64, 128);
	_ink['S'] = RGB(255, 192, 128);
}

static uint32_t hash(uint32_t x, uint32_t y, uint32_t s) {
	uint32_t h = x * 0x9e3779b1u ^ (y + 0x7f4a7c15u) * 0x85ebca6bu ^ s * 0xc2b2ae35u;
	h ^= h >> 15;
	h *= 0x2c1b3c6du;
	h ^= h >> 12;
	return h ^ (h >> 15);
}

// ---------------------------------------------------------------------------
// Pictures drawn over the wallpaper, 16 pixels square

static const char* const _shelf[] = {
	"nnnnnnnnnnnnnnnn",
	"BBBBBBBBBBBBBBBB",
	"bbbbbbbbbbbbbbbb",
	"..dd........dd..",
	"..ddd.......ddd.",
	"..dd........dd..",
	"..d.........d...",
};

static const char* const _table_l[] = {
	"MMMMMMMMMMMMMMMM",
	"MWMMMMMWMMMMMMWM",
	"mmmmmmmmmmmmmmmm",
	"mWmmmWmmmmWmmmmW",
	".bbbbbbbbbbbbbbb",
	".bbbbbbbbbbbbbbb",
	"..bB............",
	"..bB............",
	"..bB............",
	"..bB............",
	"..bB............",
	"..bB............",
	"..bB............",
	"..bB............",
	"..bB............",
	".dddd...........",
};

static const char* const _table_m[] = {
	"MMMMMMMMMMMMMMMM",
	"MMMMMWMMMMMMMWMM",
	"mmmmmmmmmmmmmmmm",
	"mmmWmmmmmWmmmmmm",
	"bbbbbbbbbbbbbbbb",
	"bbbbbbbbbbbbbbbb",
};

static const char* const _table_r[] = {
	"MMMMMMMMMMMMMMMM",
	"MWMMMMMMMWMMMMMM",
	"mmmmmmmmmmmmmmmm",
	"mmmWmmmmmmmWmmm.",
	"bbbbbbbbbbbbbbb.",
	"bbbbbbbbbbbbbbb.",
	"............bB..",
	"............bB..",
	"............bB..",
	"............bB..",
	"............bB..",
	"............bB..",
	"............bB..",
	"............bB..",
	"............bB..",
	"...........dddd.",
};

static const char* const _bed_l[] = {
	"dd..............",
	"dbd.............",
	"dbdWWWWWW.......",
	"dbWWWWWWWWLLLLLL",
	"dbWWWWWWWWLLLLLL",
	"dbdLLLLLLLLLCLLL",
	"dbdLLCLLLLLLLLLL",
	"dbdLLLLLLLLLLLLL",
	"dbdNNNNNNNNNNNNN",
	"dbdbbbbbbbbbbbbb",
	"dbddddddddddddd.",
	"dbd.............",
	"dbd.............",
	"dbd.............",
	"dbd.............",
	"ddd.............",
};

static const char* const _bed_m[] = {
	"................",
	"................",
	"................",
	"LLLLLLLLLLLLLLLL",
	"LLLLCLLLLLLLLCLL",
	"LLLLLLLLLLLLLLLL",
	"LLLLLLLLCLLLLLLL",
	"LCLLLLLLLLLLLLLL",
	"NNNNNNNNNNNNNNNN",
	"bbbbbbbbbbbbbbbb",
	"dddddddddddddddd",
};

static const char* const _bed_r[] = {
	"................",
	"................",
	".............dd.",
	"LLLLLLLLLLLLdbd.",
	"LLLLLLLLCLLLdbd.",
	"LLLLLLLLLLLLdbd.",
	"LLLCLLLLLLLLdbd.",
	"LLLLLLLLLLLLdbd.",
	"NNNNNNNNNNNNdbd.",
	"bbbbbbbbbbbbdbd.",
	"ddddddddddddddd.",
	"............dbd.",
	"............dbd.",
	"............dbd.",
	"............dbd.",
	"............ddd.",
};

static const char* const _window_top[] = {
	"................",
	"......dddd......",
	"....ddNNNNdd....",
	"...dNNNNNNNNd...",
	"..dNNNNNNNNWNd..",
	"..dNNNNNNNNNNd..",
	".dNNWNNNdNNNNNd.",
	".dNNNNNNdNNNNNd.",
	".dNNNNNNdNNNNNd.",
	".dNNNNNNdNNyyNd.",
	".dNNNNNNdNyyNNd.",
	".dNNNNNNdNyyNNd.",
	".dNNNNNNdNNyyNd.",
	".dddddddddddddd.",
	".dNNNNNNdNNNNNd.",
	".dNNNNNNdNNNNNd.",
};

static const char* const _window_bottom[] = {
	".dNNNWNNdNNNNNd.",
	".dNNNNNNdNNNNNd.",
	".dNNNNNNdNNNNNd.",
	".dNNNNNNdNNNWNd.",
	".dNNNNNNdNNNNNd.",
	".dNNNNNNdNNNNNd.",
	".dNNNNNNdNNNNNd.",
	".dddddddddddddd.",
	"nnnnnnnnnnnnnnnn",
	"bbbbbbbbbbbbbbbb",
	".dddddddddddddd.",
};

static const char* const _portrait[] = {
	"................",
	"................",
	"..OOOOOOOOOOOO..",
	"..OYYYYYYYYYYO..",
	"..OYppppppppYO..",
	"..OYppWWWWppYO..",
	"..OYpWWWWWWpYO..",
	"..OYpWKWWKWpYO..",
	"..OYpMWWWWMpYO..",
	"..OYpWWKKWWpYO..",
	"..OYpWWWWWWpYO..",
	"..OYpWpWWpWpYO..",
	"..OYppppppppYO..",
	"..OYYYYYYYYYYO..",
	"..OOOOOOOOOOOO..",
	"................",
};

static const char* const _cobweb[] = {
	"GgGGGGgGGGGGgGG.",
	"Gg..g....g...g..",
	"G.g..g...g..g...",
	"G..g.ggggggg....",
	"Gggg..g..g.g....",
	"G..g...g.g.g....",
	"G...g..gggg.....",
	"G...gggg.g......",
	"Gg..g..g.g......",
	"G.g.g..gg.......",
	"G..ggggg........",
	"G..g.g..........",
	"Gggg............",
	"G.g.............",
	"Gg..............",
	"G...............",
};

static const char* const _books[] = {
	"dddddddddddddddd",
	"d..R.L.....P...d",
	"dRRRLLEE..PPbRRd",
	"dRRRLLEEYYPPbRRd",
	"dRnRLLEnYYPPbRnd",
	"dRRRLLEEYYPPbRRd",
	"dRRRLLEEYYPPbRRd",
	"dddddddddddddddd",
	"dLL.E....RR..Y.d",
	"dLLPEEbbYRRLLYYd",
	"dLLPEEbbYRRLLYYd",
	"dLnPEnbbYRnLLYYd",
	"dLLPEEbbYRRLLYYd",
	"dLLPEEbbYRRLLYYd",
	"dddddddddddddddd",
	"bbbbbbbbbbbbbbbb",
};

static const char* const _doorway_top[] = {
	"bbbbbbbbbbbbbbbb",
	"bBBBBBBBBBBBBBBb",
	"bBdddddddddddBBb",
	"bBd..........dBb",
	"bBd..........dBb",
	"bBd..........dBb",
	"bBd..........dBb",
	"bBd..........dBb",
	"bBd..........dBb",
	"bBd..........dBb",
	"bBd..........dBb",
	"bBd..........dBb",
	"bBd..........dBb",
	"bBd..........dBb",
	"bBd..........dBb",
	"bBd..........dBb",
};

static const char* const _doorway[] = {
	"bBd..........dBb",
};

static const char* const _lantern[] = {
	"................",
	"................",
	"................",
	"................",
	".......kk.......",
	"......k..k......",
	".....kkkkkk.....",
	"....kkkkkkkk....",
	"....kNNNNNNk....",
	"....kNNNNNNk....",
	"....kNNNNNNk....",
	"....kNNNNNNk....",
	"....kNNNNNNk....",
	"....kkkkkkkk....",
	".....kkkkkk.....",
	"....kkkkkkkk....",
};

static const char* const _lantern_lit[2][16] = { {
	"................",
	"................",
	"................",
	"................",
	".......kk.......",
	"......k..k......",
	".....kkkkkk.....",
	"....kkkkkkkk....",
	"....kyyyyyyk....",
	"....kyyOyyyk....",
	"....kyOYOyyk....",
	"....kyOYOyyk....",
	"....kyyyyyyk....",
	"....kkkkkkkk....",
	".....kkkkkk.....",
	"....kkkkkkkk....",
}, {
	"................",
	"................",
	"................",
	"................",
	".......kk.......",
	"......k..k......",
	".....kkkkkk.....",
	"....kkkkkkkk....",
	"....kyyyyyyk....",
	"....kyyyOyyk....",
	"....kyyOYOyk....",
	"....kyyOYOyk....",
	"....kyyyyyyk....",
	"....kkkkkkkk....",
	".....kkkkkk.....",
	"....kkkkkkkk....",
} };

static const char* const _candle[] = {
	"................",
	"................",
	"................",
	"................",
	"................",
	".......KK.......",
	"......nWWn......",
	"......nWWn......",
	"......nWnn......",
	"......nnnn......",
	"......nnnn......",
	"....OYYYYYYO....",
	".....OYYYYO.....",
	".......OO.......",
	"......OOOO......",
	"................",
};

static const char* const _flame[2][5] = {
	{ "................", ".......YY.......", "......YyyY......", "......YyyY......", ".......OO......." },
	{ "........Y.......", ".......YY.......", "......YyyY......", ".......yY.......", ".......OO......." },
};

static const char* const _lever[] = {
	"................",
	"................",
	"................",
	"..RR............",
	".RRRR...........",
	".RRRR...........",
	"..RRk...........",
	"....kk..........",
	".....kk.........",
	"......kk........",
	"....gggggggg....",
	"....gGGGGGGg....",
	"....gGkkkkGg....",
	"....gGGGGGGg....",
	"....gggggggg....",
	"................",
};

static const char* const _plate[] = {
	"kk............kk",
	"k..............k",
	"k..............k",
	"k..............k",
	"k..............k",
	"k..............k",
	"k..............k",
	"k..............k",
	"k..............k",
	"k..............k",
	"k..RRRRRRRRRR..k",
	"k.RRRRRRRRRRRR.k",
	"k.rrrrrrrrrrrr.k",
	"k.gggggggggggg.k",
	"kggggggggggggggk",
	"kkkkkkkkkkkkkkkk",
};

static const char* const _plate_down[] = {
	"kk............kk",
	"k..............k",
	"k..............k",
	"k..............k",
	"k..............k",
	"k..............k",
	"k..............k",
	"k..............k",
	"k..............k",
	"k..............k",
	"k..............k",
	"k..............k",
	"k..............k",
	"k.EEEEEEEEEEEE.k",
	"kggggggggggggggk",
	"kkkkkkkkkkkkkkkk",
};

static const char* const _bars[] = {
	"kkkkkkkkkkkkkkkk",
	"gGGGGGGGGGGGGGGg",
	".kg..kg..kg..kg.",
	".kg..kg..kg..kg.",
	".kg..kg..kg..kg.",
	".kg..kg..kg..kg.",
	".kg..kg..kg..kg.",
	".kg..kg..kg..kg.",
	".kg..kg..kg..kg.",
	".kg..kg..kg..kg.",
	".kg..kg..kg..kg.",
	".kg..kg..kg..kg.",
	".kg..kg..kg..kg.",
	".kg..kg..kg..kg.",
	".kg..kg..kg..kg.",
	".kg..kg..kg..kg.",
};

static const char* const _trapdoor[] = {
	"nnnnnnnnnnnnnnnn",
	"dBBBBBBdBBBBBBBd",
	"dbbbbbbdbbbbbbbd",
	"dBBBBBBdBBBBBBBd",
	"dbbbbbbdbbbkkbbd",
	"dBBBBBBdBBkBBkBd",
	"dbbbbbbdbbkbbkbd",
	"dBBBBBBdBBBkkBBd",
	"dbbbbbbdbbbbbbbd",
	"dBBBBBBdBBBBBBBd",
	"dbbbbbbdbbbbbbbd",
	"dBBBBBBdBBBBBBBd",
	"dbbbbbbdbbbbbbbd",
	"dBBBBBBdBBBBBBBd",
	"dbbbbbbdbbbbbbbd",
	"dddddddddddddddd",
};

static const char* const _door_top[] = {
	"....dddddddd....",
	"..ddBbBbBbBbdd..",
	".dBbBbBbBbBbBbd.",
	".dBbBbBbBbBbBbd.",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dkkkkkkkkkkkkkkd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
};

static const char* const _door_mid[] = {
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBYYYYbd",
	"dBbBbBbBbBYKKYbd",
	"dBbBbBbBbBYKKYbd",
	"dBbBbBbBbBYYKYbd",
	"dBbBbBbBbBYYYYbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dkkkkkkkkkkkkkkd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
};

static const char* const _door_bottom[] = {
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dkkkkkkkkkkkkkkd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dBbBbBbBbBbBbBbd",
	"dddddddddddddddd",
};

// ---------------------------------------------------------------------------
// Making the pictures

typedef uint8_t MmLogical_t[MM_TILE_LOGICAL][MM_TILE_LOGICAL];

static uint8_t wallpaper(uint32_t style, int32_t x, int32_t y) {
	switch (style) {
	case MM_STYLE_ATTIC: {
		// Upright boards, each a slightly different shade
		const int32_t board = x >> 2;
		if ((x & 3) == 3) return RGB(32, 0, 0);
		if (board == 1 && y == 9) return RGB(32, 32, 0);
		return (board & 1) ? RGB(96, 64, 0) : RGB(96, 32, 0);
	}
	case MM_STYLE_UPPER: {
		// Diamonds on purple
		const int32_t d = game_abs(x - 8) + game_abs(y - 8);
		if (d == 7) return RGB(128, 64, 128);
		if (d == 0 || d == 1) return RGB(192, 128, 64);
		return RGB(64, 32, 64);
	}
	case MM_STYLE_GROUND: {
		// Stripes, with little flowers in the wide ones
		if (x == 7 || x == 15) return RGB(64, 96, 64);
		if (x < 7 && ((x == 3 && (y == 4 || y == 6)) || (y == 5 && (x == 2 || x == 4)))) return RGB(160, 96, 128);
		if (x < 7 && x == 3 && y == 5) return RGB(224, 192, 64);
		return x < 7 ? RGB(0, 64, 64) : RGB(32, 64, 64);
	}
	case MM_STYLE_CELLAR: {
		// Big dark blocks
		const int32_t off = (y < 8) ? 0 : 8;
		if (y == 7 || y == 15 || ((x + off) & 15) == 0) return RGB(0, 0, 0);
		return (hash(x >> 2, y >> 2, (x + off) >> 4) & 7) == 0 ? RGB(64, 64, 64) : RGB(32, 32, 64);
	}
	default:
		return RGB(0, 0, 64);
	}
}

static uint8_t stone(int32_t x, int32_t y) {
	const int32_t off = (y & 8) ? 4 : 0;
	if ((y & 7) == 7 || ((x + off) & 7) == 7) return RGB(32, 32, 64);
	if ((y & 7) == 0) return RGB(160, 160, 192);
	return (hash((x + off) >> 3, y >> 3, 7) & 3) == 0 ? RGB(96, 96, 160) : RGB(96, 96, 128);
}

static uint8_t floorboards(int32_t x, int32_t y) {
	if (y == 0) return RGB(224, 160, 64);
	if (y == 1) return RGB(192, 128, 64);
	if (y == 15) return RGB(32, 0, 0);
	if (y == 8 || ((y < 8 ? x : x + 8) & 15) == 0) return RGB(64, 32, 0);
	return (y & 1) ? RGB(128, 64, 0) : RGB(160, 96, 32);
}

static uint8_t roof(int32_t x, int32_t y) {
	// Rounded tiles in rows, each row half a tile along
	const int32_t off = (y & 4) ? 2 : 0;
	const int32_t tx = (x + off) & 3, ty = y & 3;
	if (ty == 3 && (tx == 0 || tx == 3)) return RGB(32, 0, 32);
	if (ty == 0) return RGB(160, 64, 96);
	return RGB(96, 32, 64);
}

static uint8_t grass(int32_t x, int32_t y) {
	const uint32_t h = hash(x, 0, 3);
	const int32_t blade = 2 + (int32_t)(h & 1);
	if (y < blade) return (h & 6) ? 0 : RGB(32, 160, 32);
	if (y < 5) return y == blade ? RGB(64, 192, 64) : RGB(32, 128, 32);
	if (y == 5) return RGB(0, 64, 0);
	return (hash(x >> 1, y >> 1, 4) & 3) == 0 ? RGB(96, 64, 32) : RGB(64, 32, 0);
}

static uint8_t earth(int32_t x, int32_t y) {
	const uint32_t h = hash(x >> 1, y >> 1, 5);
	if ((h & 15) == 0) return RGB(128, 96, 64);
	return (h & 16) ? RGB(64, 32, 0) : RGB(96, 64, 32);
}

static void overlay(MmLogical_t l, const char* const* rows, uint32_t n, bool mirror) {
	for (uint32_t y = 0; y < n && y < MM_TILE_LOGICAL; ++y) {
		for (uint32_t x = 0; x < MM_TILE_LOGICAL && rows[y][x]; ++x) {
			const char ch = rows[y][x];
			if (ch == '.') continue;
			l[y][mirror ? MM_TILE_LOGICAL - 1 - x : x] = _ink[ch & 127];
		}
	}
}

// Warms the wallpaper round a light
static void glow(MmLogical_t l, int32_t cx, int32_t cy) {
	for (int32_t y = 0; y < MM_TILE_LOGICAL; ++y) {
		for (int32_t x = 0; x < MM_TILE_LOGICAL; ++x) {
			const int32_t d2 = (x - cx) * (x - cx) + (y - cy) * (y - cy);
			if (d2 < 64) l[y][x] = mm_gfx_mix(l[y][x], RGB(255, 192, 64), (64 - d2) * 2);
		}
	}
}

static void compose(uint32_t* dst, uint32_t style, uint32_t decor, bool shadow, uint32_t flicker) {
	MmLogical_t l;
	for (int32_t y = 0; y < MM_TILE_LOGICAL; ++y) {
		for (int32_t x = 0; x < MM_TILE_LOGICAL; ++x) {
			uint8_t p = wallpaper(style, x, y);
			switch (decor) {
			case MM_DECOR_STONE: p = stone(x, y); break;
			case MM_DECOR_FLOOR: p = floorboards(x, y); break;
			case MM_DECOR_ROOF: p = roof(x, y); break;
			case MM_DECOR_EARTH: p = earth(x, y); break;
			case MM_DECOR_GRASS: {
				const uint8_t q = grass(x, y);
				if (q) p = q;
				break;
			}
			default: break;
			}
			l[y][x] = p;
		}
	}
	if (shadow) {
		for (int32_t y = 0; y < 3; ++y) {
			for (int32_t x = 0; x < MM_TILE_LOGICAL; ++x) l[y][x] = mm_gfx_mix(l[y][x], 0, 160 - y * 48);
		}
	}
	switch (decor) {
	case MM_DECOR_SHELF: overlay(l, _shelf, GAME_ART_ROWS(_shelf), false); break;
	case MM_DECOR_TABLE_L: overlay(l, _table_l, GAME_ART_ROWS(_table_l), false); break;
	case MM_DECOR_TABLE_M: overlay(l, _table_m, GAME_ART_ROWS(_table_m), false); break;
	case MM_DECOR_TABLE_R: overlay(l, _table_r, GAME_ART_ROWS(_table_r), false); break;
	case MM_DECOR_BED_L: overlay(l, _bed_l, GAME_ART_ROWS(_bed_l), false); break;
	case MM_DECOR_BED_M: overlay(l, _bed_m, GAME_ART_ROWS(_bed_m), false); break;
	case MM_DECOR_BED_R: overlay(l, _bed_r, GAME_ART_ROWS(_bed_r), false); break;
	case MM_DECOR_WINDOW_TOP: overlay(l, _window_top, GAME_ART_ROWS(_window_top), false); break;
	case MM_DECOR_WINDOW_BOTTOM: overlay(l, _window_bottom, GAME_ART_ROWS(_window_bottom), false); break;
	case MM_DECOR_PORTRAIT: overlay(l, _portrait, GAME_ART_ROWS(_portrait), false); break;
	case MM_DECOR_COBWEB_L: overlay(l, _cobweb, GAME_ART_ROWS(_cobweb), false); break;
	case MM_DECOR_COBWEB_R: overlay(l, _cobweb, GAME_ART_ROWS(_cobweb), true); break;
	case MM_DECOR_BOOKS: overlay(l, _books, GAME_ART_ROWS(_books), false); break;
	case MM_DECOR_DOORWAY_TOP: overlay(l, _doorway_top, GAME_ART_ROWS(_doorway_top), false); break;
	case MM_DECOR_DOORWAY:
		for (uint32_t y = 0; y < MM_TILE_LOGICAL; ++y) overlay(l + y, _doorway, 1, false);
		break;
	case MM_DECOR_LANTERN: overlay(l, _lantern, GAME_ART_ROWS(_lantern), false); break;
	case MM_DECOR_LANTERN_LIT:
		glow(l, 7, 10);
		overlay(l, _lantern_lit[flicker], 16, false);
		break;
	case MM_DECOR_CANDLE: overlay(l, _candle, GAME_ART_ROWS(_candle), false); break;
	case MM_DECOR_CANDLE_LIT:
		glow(l, 7, 3);
		overlay(l, _candle, GAME_ART_ROWS(_candle), false);
		overlay(l, _flame[flicker], 5, false);
		break;
	case MM_DECOR_LEVER: overlay(l, _lever, GAME_ART_ROWS(_lever), false); break;
	case MM_DECOR_LEVER_ON: {
		overlay(l, _lever, GAME_ART_ROWS(_lever), true);
		// A green knob once it has been pulled
		for (int32_t y = 3; y < 7; ++y) {
			for (int32_t x = 10; x < 16; ++x) if (l[y][x] == _ink['R']) l[y][x] = _ink['E'];
		}
		break;
	}
	case MM_DECOR_PLATE: overlay(l, _plate, GAME_ART_ROWS(_plate), false); break;
	case MM_DECOR_PLATE_DOWN: overlay(l, _plate_down, GAME_ART_ROWS(_plate_down), false); break;
	case MM_DECOR_BARS: overlay(l, _bars, GAME_ART_ROWS(_bars), false); break;
	case MM_DECOR_TRAPDOOR: overlay(l, _trapdoor, GAME_ART_ROWS(_trapdoor), false); break;
	case MM_DECOR_DOOR_TOP: overlay(l, _door_top, GAME_ART_ROWS(_door_top), false); break;
	case MM_DECOR_DOOR_MID: overlay(l, _door_mid, GAME_ART_ROWS(_door_mid), false); break;
	case MM_DECOR_DOOR_BOTTOM: overlay(l, _door_bottom, GAME_ART_ROWS(_door_bottom), false); break;
	default: break;
	}
	// Each pixel of the picture is 2 by 2 on the screen
	uint8_t* b = (uint8_t*)dst;
	for (int32_t y = 0; y < MM_TILE; ++y) {
		for (int32_t x = 0; x < MM_TILE; ++x) b[(y << MM_TILE_SHIFT) + x] = l[y >> 1][x >> 1];
	}
}

static void sky_init(void) {
	for (int32_t y = 0; y < MM_SKY; ++y) {
		for (int32_t x = 0; x < MM_SKY; ++x) {
			// Stars, a few of them twice the size
			const uint32_t h = hash(x >> 1, y >> 1, 11);
			uint8_t p = RGB(0, 0, 64);
			if ((h & 255) == 0) p = RGB(255, 255, 255);
			else if ((h & 255) == 1) p = RGB(255, 255, 128);
			else if ((hash(x, y, 12) & 511) == 0) p = RGB(128, 128, 192);
			_sky[y][x] = p;
		}
		for (int32_t x = 0; x < MM_TILE; ++x) _sky[y][MM_SKY + x] = _sky[y][x];
	}
}

void mm_tiles_init(void) {
	ink_init();
	sky_init();
	_entry[0].key = 0;
	_entry[0].flicker = 0xff;
	_tile[0] = NULL;
}

static bool flickers(uint32_t decor) {
	return decor == MM_DECOR_CANDLE_LIT || decor == MM_DECOR_LANTERN_LIT;
}

static bool opaque(uint32_t decor) {
	return decor == MM_DECOR_STONE || decor == MM_DECOR_EARTH || decor == MM_DECOR_FLOOR || decor == MM_DECOR_ROOF || decor == MM_DECOR_TRAPDOOR;
}

uint8_t mm_tiles_get(uint32_t style, uint32_t decor, bool shadow) {
	if (style == MM_STYLE_SKY && decor == MM_DECOR_NONE) return 0;
	// Solid pictures look the same on any wallpaper
	if (opaque(decor)) {
		style = 0;
		shadow = false;
	}
	const uint16_t key = (uint16_t)((style << 8) | (decor << 1) | (shadow ? 1 : 0));
	for (uint32_t i = 1; i < _entries; ++i) {
		if (_entry[i].key == key) return i;
	}
	const uint32_t need = flickers(decor) ? 2 : 1;
	if (_entries >= 256 || _stores + need > MM_TILE_STORES) return 0;
	MmTileEntry_t* e = &_entry[_entries];
	e->key = key;
	e->store = _stores++;
	e->flicker = need > 1 ? _stores++ : 0xff;
	compose(_store[e->store], style, decor, shadow, 0);
	if (e->flicker != 0xff) compose(_store[e->flicker], style, decor, shadow, 1);
	_tile[_entries] = _store[e->store];
	return _entries++;
}

// Copies a row of a tile to dx along the row, clipping it at the sides
static __force_inline void copy_row(uint8_t* row, int32_t dx, const uint8_t* src) {
	if (dx >= 0 && dx <= GAME_SCREEN_W - MM_TILE) {
		mm_u32u_t* d = (mm_u32u_t*)(row + dx);
		const mm_u32u_t* s = (const mm_u32u_t*)src;
		d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3];
		d[4] = s[4]; d[5] = s[5]; d[6] = s[6]; d[7] = s[7];
		return;
	}
	const int32_t i0 = dx < 0 ? -dx : 0;
	const int32_t i1 = dx + MM_TILE > GAME_SCREEN_W ? GAME_SCREEN_W - dx : MM_TILE;
	for (int32_t i = i0; i < i1; ++i) row[dx + i] = src[i];
}

// x is how far the first tile starts to the left of the screen, and row
// is from the top of the view
static void __not_in_flash_func(house_renderer)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	const MmView_t* v = (const MmView_t*)d1;
	const int32_t ty = row + v->fy;
	const uint8_t* m = v->cell + (ty >> MM_TILE_SHIFT) * v->stride;
	const uint32_t k = (ty & (MM_TILE - 1)) << MM_TILE_SHIFT;
	const uint8_t* sky = _sky[(row + v->sky_y) & (MM_SKY - 1)];
	for (int32_t i = 0, dx = -x; dx < GAME_SCREEN_W; ++i, dx += MM_TILE) {
		const uint32_t t = m[i];
		const uint8_t* src = t ? (const uint8_t*)_tile[t] + k : sky + ((dx + v->sky_x) & (MM_SKY - 1));
		copy_row(r->b, dx, src);
	}
}

void mm_tiles_draw(const uint8_t* gfx, uint32_t stride, int32_t cam_x, int32_t cam_y, uint32_t frame) {
	const uint32_t flicker = (frame >> MM_FLICKER_SHIFT) & 1;
	if (flicker != _flicker) {
		_flicker = flicker;
		for (uint32_t i = 1; i < _entries; ++i) {
			const MmTileEntry_t* e = &_entry[i];
			if (e->flicker != 0xff) _tile[i] = _store[flicker ? e->flicker : e->store];
		}
	}
	_view_i ^= 1;
	MmView_t* v = &_view[_view_i];
	v->cell = gfx + (cam_y >> MM_TILE_SHIFT) * stride + (cam_x >> MM_TILE_SHIFT);
	v->stride = stride;
	v->fy = cam_y & (MM_TILE - 1);
	// The stars are far away, so move a quarter as much
	v->sky_x = cam_x >> 2;
	v->sky_y = cam_y >> 2;
	game_spr_add(cam_x & (MM_TILE - 1), MM_HUD_H, GAME_SCREEN_W, MM_VIEW_H, v, NULL, house_renderer);
}
