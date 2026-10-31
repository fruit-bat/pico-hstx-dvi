#include "inv_alien.h"
#include "game_pallet.h"

// Each alien is put together from parts, so it can look about, blink and move

static const char* const _commander_body[] = {
	".........PPPPPP.........",
	"......PPPPPPPPPPPP......",
	"....PPPPPPPPPPPPPPPP....",
	"...PPPPPPPPPPPPPPPPPP...",
	"..PPPPPPPPPPPPPPPPPPPP..",
	"..PPPPPPPPPPPPPPPPPPPP..",
	"..PPPPPPPPPPPPPPPPPPPP..",
	"PPPPPPPPPPPPPPPPPPPPPPPP",
	"PPPPPPPPPPPPPPPPPPPPPPPP",
	"..PPPPPPPPPPPPPPPPPPPP..",
	"....PPPPPPPPPPPPPPPP....",
};

// One big eye
static const char* const _commander_eye[] = {
	"..WWWWWW..",
	".WWWWWWWW.",
	"WWWWWWWWWW",
	".WWWWWWWW.",
	"..WWWWWW..",
};

static const char* const _crab_body[] = {
	"......BB........BB......",
	"........B......B........",
	".....BBBBBBBBBBBBBB.....",
	"....BBBBBBBBBBBBBBBB....",
	"...BBBBBBBBBBBBBBBBBB...",
	"...BBBBBBBBBBBBBBBBBB...",
	"...BBBBBBBBBBBBBBBBBB...",
	"...BBBBBBBBBBBBBBBBBB...",
	"....BBBBBBBBBBBBBBBB....",
	".....BBBBBBBBBBBBBB.....",
};

static const char* const _crab_claw[2][2][5] = {
	{
		{ "RR.", "R..", "RRR", "R..", "RR." },
		{ ".RR", "..R", "RRR", "..R", ".RR" },
	},
	{
		{ "...", "RR.", "RRR", "RR.", "..." },
		{ "...", ".RR", "RRR", ".RR", "..." },
	},
};

static const char* const _crab_mouth[2][2] = {
	{ "RRRRRR", "......" },
	{ "R    R", "RRRRRR" },
};

static const char* const _jelly_body[] = {
	"........GGGGGGGG........",
	".....GGGGGGGGGGGGGG.....",
	"...GGGGGGGGGGGGGGGGGG...",
	"..GGGGGGGGGGGGGGGGGGGG..",
	"..GGGGGGGGGGGGGGGGGGGG..",
	"..GGGGGGGGGGGGGGGGGGGG..",
	"..GGGGGGGGGGGGGGGGGGGG..",
	"..GGGGGGGGGGGGGGGGGGGG..",
};

// Two eyes, for the crab and the jelly. ' ' is a see through pupil.
static const char* const _eye[INV_ALIEN_LOOKS][4] = {
	{ ".WWWW.", "W  WWW", "W  WWW", ".WWWW." },
	{ ".WWWW.", "WW  WW", "WW  WW", ".WWWW." },
	{ ".WWWW.", "WWW  W", "WWW  W", ".WWWW." },
	{ "......", "......", ".    .", "......" },
};

// How the tentacles wave, a row at a time
static const int8_t _wave[4] = { 0, 1, 0, -1 };

static const uint8_t _commander_colours[] = { GAME_PURPLE, GAME_WHITE, GAME_YELLOW };
static const uint8_t _crab_colours[] = { GAME_BLUE, GAME_WHITE, GAME_RED };
static const uint8_t _jelly_colours[] = { GAME_GREEN, GAME_WHITE, GAME_PINK };

static GameArt_t _art[INV_ALIEN_TYPE_COUNT][INV_ALIEN_ANIM][INV_ALIEN_LOOKS];

static void make_commander(GameArtCanvas_t* c, const uint32_t anim, const uint32_t look) {
	game_art_canvas_clear(c, INV_ALIEN_W, 13);
	game_art_canvas_stamp(c, _commander_body, GAME_ART_ROWS(_commander_body), 0, 0);
	if (look == INV_ALIEN_BLINK) {
		static const char* const lid[] = { "WWWWWWWWWW" };
		game_art_canvas_stamp(c, lid, 1, 7, 4);
	}
	else {
		game_art_canvas_stamp(c, _commander_eye, GAME_ART_ROWS(_commander_eye), 7, 2);
		static const char* const pupil[] = { "  ", "  ", "  " };
		game_art_canvas_stamp(c, pupil, 3, 8 + look * 3, 3);
	}
	// Lights that run round the rim
	for (int32_t x = anim; x < INV_ALIEN_W; x += 4) {
		game_art_canvas_set(c, x, 7, 'Y');
		game_art_canvas_set(c, INV_ALIEN_W - 1 - x, 8, 'Y');
	}
	// Feet that paddle
	static const int32_t feet[] = { 4, 8, 14, 18 };
	for (uint32_t i = 0; i < 4; ++i) {
		const int32_t x = feet[i] + ((anim & 1) ? (i < 2 ? -1 : 1) : 0);
		game_art_canvas_set(c, feet[i], 11, 'P');
		game_art_canvas_set(c, feet[i] + 1, 11, 'P');
		game_art_canvas_set(c, x, 12, 'P');
		game_art_canvas_set(c, x + 1, 12, 'P');
	}
}

static void make_crab(GameArtCanvas_t* c, const uint32_t anim, const uint32_t look) {
	game_art_canvas_clear(c, INV_ALIEN_W, 14);
	game_art_canvas_stamp(c, _crab_body, GAME_ART_ROWS(_crab_body), 0, 0);
	game_art_canvas_stamp(c, _eye[look], 4, 5, 3);
	game_art_canvas_stamp(c, _eye[look], 4, 13, 3);
	// Snapping claws and a chomping mouth
	const uint32_t snap = anim & 1;
	game_art_canvas_stamp(c, _crab_claw[snap][0], 5, 0, 2);
	game_art_canvas_stamp(c, _crab_claw[snap][1], 5, 21, 2);
	game_art_canvas_stamp(c, _crab_mouth[(anim >> 1) & 1], 2, 9, 8);
	// Scuttling legs
	static const int32_t legs[] = { 6, 9, 14, 17 };
	for (uint32_t i = 0; i < 4; ++i) {
		const int32_t out = i < 2 ? -1 : 1;
		for (int32_t r = 0; r < 4; ++r) {
			const int32_t dx = snap ? out * r : (r & 1) * out;
			game_art_canvas_set(c, legs[i] + dx, 10 + r, 'B');
		}
	}
}

static void make_jelly(GameArtCanvas_t* c, const uint32_t anim, const uint32_t look) {
	game_art_canvas_clear(c, INV_ALIEN_W, 14);
	game_art_canvas_stamp(c, _jelly_body, GAME_ART_ROWS(_jelly_body), 0, 0);
	game_art_canvas_stamp(c, _eye[look], 4, 5, 3);
	game_art_canvas_stamp(c, _eye[look], 4, 13, 3);
	// Glowing spots that drift along
	for (int32_t x = 4 + anim; x < 21; x += 4) game_art_canvas_set(c, x, 7, 'K');
	// Tentacles with a wave running down them, the two sides mirrored
	static const int32_t tentacles[] = { 3, 8, 14, 19 };
	for (uint32_t i = 0; i < 4; ++i) {
		const int32_t side = i < 2 ? 1 : -1;
		for (int32_t r = 0; r < 6; ++r) {
			const int32_t x = tentacles[i] + side * _wave[((r >> 1) + 4 - anim) & 3];
			game_art_canvas_set(c, x, 8 + r, 'G');
			game_art_canvas_set(c, x + 1, 8 + r, 'G');
		}
	}
}

void inv_alien_init(void) {
	GameArtCanvas_t c;
	for (uint32_t anim = 0; anim < INV_ALIEN_ANIM; ++anim) {
		for (uint32_t look = 0; look < INV_ALIEN_LOOKS; ++look) {
			make_commander(&c, anim, look);
			game_art_canvas_make(&_art[INV_ALIEN_COMMANDER][anim][look], &c, "PWY", _commander_colours);
			make_crab(&c, anim, look);
			game_art_canvas_make(&_art[INV_ALIEN_CRAB][anim][look], &c, "BWR", _crab_colours);
			make_jelly(&c, anim, look);
			game_art_canvas_make(&_art[INV_ALIEN_JELLY][anim][look], &c, "GWK", _jelly_colours);
		}
	}
}

const GameArt_t* __not_in_flash_func(inv_alien_art)(uint32_t type, uint32_t anim, uint32_t look) {
	return &_art[type % INV_ALIEN_TYPE_COUNT][anim % INV_ALIEN_ANIM][look % INV_ALIEN_LOOKS];
}

uint32_t inv_alien_height(uint32_t type) {
	return _art[type % INV_ALIEN_TYPE_COUNT][0][0].h;
}

uint32_t inv_alien_colour(uint32_t type) {
	return _art[type % INV_ALIEN_TYPE_COUNT][0][0].colour[0];
}

uint32_t inv_alien_detail(uint32_t type) {
	return _art[type % INV_ALIEN_TYPE_COUNT][0][0].colour[2];
}
