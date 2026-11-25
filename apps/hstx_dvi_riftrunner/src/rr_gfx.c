// Copyright (c) 2024 fruit-bat.

// See rr_gfx.h.

#include "rr_gfx.h"
#include "rr_world.h"
#include <string.h>
#include <math.h>

RrGfx_t rr_gfx;

// ----------------------------------------------------------------------------
// The player's ship, facing right, its exhaust, drones and the orb

static const char* const _ship[] = {
	"..KK............................",
	".KLLK...........................",
	".KLBBK..........................",
	"..KLBBKKKKKK....................",
	"..KLBBBBBBBBKKKKK...............",
	"...KLBBBBBBBBBBBBKKCCCK.........",
	".OKKLLLLLLLLLLLLLLLKCWCCKK......",
	"OOKBBBBBBBBBBBBBBBBBBBBBBBKKKK..",
	"OOKRRRRRRRRRRRRRRRRRRRRRRRRRRRWW",
	"OOKBBBBBBBBBBBBBBBBBBBBBBBKKKK..",
	".OKKDBBBBBBBBBBBBBBBBKKKK.......",
	"....KDBBBBBBBKKKKKKK............",
	"...KDBBKKKKKK...................",
	"..KKKK..........................",
};
static const hstx_dvi_pixel_t _ship_px[] = {
	RR_RGB(0, 64, 192), RR_RGB(160, 208, 255), RR_RGB(32, 112, 255), RR_RGB(16, 48, 160),
	RR_RGB(0, 255, 255), RR_RGB(255, 255, 255), RR_RGB(255, 32, 32), RR_RGB(255, 128, 0),
};
#define SHIP_KEY "KLBDCWRO"

static const char* const _flame[2][5] = {
	{
		"....YYYY",
		"..YYWWWW",
		"YYYWWWWW",
		"..YYWWWW",
		"....YYYY",
	},
	{
		"......YY",
		"...YYWWW",
		".YYYWWWW",
		"...YYWWW",
		"......YY",
	},
};
static const hstx_dvi_pixel_t _flame_px[] = { RR_RGB(255, 160, 0), RR_RGB(255, 255, 192) };

static const char* const _drone[] = {
	"..OOOO..",
	".OYYYYO.",
	"OYWWYYYO",
	"OYWYYYYO",
	"OYYYYYRO",
	"OYYYYRRO",
	".ORRRRO.",
	"..OOOO..",
};
static const hstx_dvi_pixel_t _drone_px[2][4] = {
	{ RR_RGB(255, 128, 0), RR_RGB(255, 224, 0), RR_RGB(255, 255, 255), RR_RGB(224, 32, 0) },
	{ RR_RGB(224, 64, 0), RR_RGB(255, 160, 0), RR_RGB(255, 255, 160), RR_RGB(160, 0, 0) },
};

static const char* const _orb[2][12] = {
	{
		".....WW.....",
		"..P..PP..P..",
		"...PMMMMP...",
		"..PMWWMMMP..",
		".PMWWMMMMMP.",
		"WPMMMMMMMMPW",
		"WPMMMMMMMMPW",
		".PMMMMMMMMP.",
		"..PMMMMMMP..",
		"...PMMMMP...",
		"..P..PP..P..",
		".....WW.....",
	},
	{
		"..W......W..",
		"...P.PP.P...",
		"....MMMM....",
		"..PMWWMMMP..",
		".PMWWMMMMMP.",
		"..MMMMMMMM..",
		"..MMMMMMMM..",
		".PMMMMMMMMP.",
		"..PMMMMMMP..",
		"....MMMM....",
		"...P.PP.P...",
		"..W......W..",
	},
};
static const hstx_dvi_pixel_t _orb_px[] = { RR_RGB(255, 255, 255), RR_RGB(255, 96, 224), RR_RGB(192, 0, 192) };

// ----------------------------------------------------------------------------
// What the ship fires

static const char* const _shot[] = {
	"..YYYYYYYY..",
	"YYWWWWWWWWWW",
	"..YYYYYYYY..",
};
static const hstx_dvi_pixel_t _shot_px[] = { RR_RGB(255, 224, 0), RR_RGB(255, 255, 255) };

static const char* const _twin[] = {
	"....WY",
	"...WWY",
	"..WWY.",
	".WWY..",
	"WWY...",
	"YY....",
};

static const char* const _laser[] = {
	"CCCCCCWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWCC",
	"CCCCCCWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWCC",
};
static const hstx_dvi_pixel_t _laser_px[] = { RR_RGB(0, 160, 255), RR_RGB(192, 255, 255) };

// BIG LASER: thicker, hotter
static const char* const _big_laser[] = {
	"..PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP..",
	"PPPPPPWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWPP",
	"PPPPPPWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWPP",
	"..PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP..",
};
static const hstx_dvi_pixel_t _big_laser_px[] = { RR_RGB(255, 64, 224), RR_RGB(255, 224, 255) };

static const char* const _bomb[] = {
	".KOOO.",
	"KOWWOO",
	"KOOOOO",
	".KOOO.",
};
static const hstx_dvi_pixel_t _bomb_px[] = { RR_RGB(160, 160, 160), RR_RGB(255, 128, 0), RR_RGB(255, 255, 255) };

static const char* const _capsule[] = {
	"..RRRRRR..",
	".ROOOOOOR.",
	"ROOWWWWOOR",
	"ROWWWWWWOR",
	"ROOWWWWOOR",
	".ROOOOOOR.",
	"..RRRRRR..",
};
static const hstx_dvi_pixel_t _capsule_px[2][3] = {
	{ RR_RGB(224, 0, 0), RR_RGB(255, 96, 0), RR_RGB(255, 255, 255) },
	{ RR_RGB(255, 96, 0), RR_RGB(255, 224, 0), RR_RGB(255, 255, 255) },
};

// ----------------------------------------------------------------------------
// The enemies, facing left

static const char* const _dart[2][9] = {
	{
		"........YYY.....",
		".....YYYOOY.....",
		"...YYOOOOOY.....",
		".YYOOWWOOOOOYYY.",
		"YOOOOWKOOOOOOOOY",
		".YYOOWWOOOOOYYY.",
		"...YYOOOOOY.....",
		".....YYYOOY.....",
		"........YYY.....",
	},
	{
		"................",
		"........YYY.....",
		"...YYYYYOOY.....",
		".YYOOWWOOOOOYYY.",
		"YOOOOWKOOOOOOOOY",
		".YYOOWWOOOOOYYY.",
		"...YYYYYOOY.....",
		"........YYY.....",
		"................",
	},
};
static const hstx_dvi_pixel_t _dart_px[] = { RR_RGB(255, 224, 0), RR_RGB(255, 128, 0), RR_RGB(255, 255, 255), RR_RGB(0, 0, 0) };
static const hstx_dvi_pixel_t _carrier_px[] = { RR_RGB(255, 64, 64), RR_RGB(192, 0, 0), RR_RGB(255, 255, 255), RR_RGB(0, 0, 0) };
#define DART_KEY "YOWK"

static const char* const _saucer[2][6] = {
	{
		".....CCCCCC.....",
		"....CWWCCCCC....",
		"..GGGGGGGGGGGG..",
		"GGRGGGRGGGRGGGRG",
		"..GGGGGGGGGGGG..",
		"....DD....DD....",
	},
	{
		".....CCCCCC.....",
		"....CWWCCCCC....",
		"..GGGGGGGGGGGG..",
		"GGGGRGGGRGGGRGGG",
		"..GGGGGGGGGGGG..",
		"....DD....DD....",
	},
};
static const hstx_dvi_pixel_t _saucer_px[] = {
	RR_RGB(0, 224, 255), RR_RGB(255, 255, 255), RR_RGB(64, 224, 64), RR_RGB(255, 32, 32), RR_RGB(0, 96, 32),
};

static const char* const _rocket[] = {
	"...RR...",
	"..RRRR..",
	"..WWWW..",
	"..WBBW..",
	"..WWWW..",
	"..WWWW..",
	"..WRRW..",
	"..WWWW..",
	"..WWWW..",
	".RWWWWR.",
	"RRWWWWRR",
	"RR.GG.RR",
};
static const hstx_dvi_pixel_t _rocket_px[] = { RR_RGB(255, 32, 32), RR_RGB(240, 240, 240), RR_RGB(0, 160, 255), RR_RGB(128, 128, 128) };

static const char* const _rocket_flame[2][5] = {
	{
		"..YWWY..",
		"..YWWY..",
		"...YY...",
		"...YY...",
		"....Y...",
	},
	{
		"..YWWY..",
		"...WW...",
		"..YYYY..",
		"...Y....",
		"........",
	},
};

static const char* const _fuel[] = {
	"...GGGGGGGGGGG...",
	".GGWWWWWWWWWWWGG.",
	"RRRRRRRRRRRRRRRRR",
	"RWWWRWRWRWWWRWRRR",
	"RWRRRWRWRWRRRWRRR",
	"RWWRRWRWRWWRRWRRR",
	"RWRRRWRWRWRRRWRRR",
	"RWRRRWWWRWWWRWWWR",
	"RRRRRRRRRRRRRRRRR",
	".GGWWWWWWWWWWWGG.",
	"..WW.........WW..",
	".WW...........WW.",
};
static const hstx_dvi_pixel_t _fuel_px[] = { RR_RGB(192, 192, 192), RR_RGB(255, 255, 255), RR_RGB(224, 0, 0), RR_RGB(96, 96, 96) };

// The barrel, pointing three ways, over the same dome
static const char* const _turret[3][10] = {
	{
		"................",
		"................",
		"................",
		"................",
		"BBBBBGGGGGG.....",
		"BBBBGGWWGGGG....",
		"...GGGGGGGGGG...",
		"..GGRRGGGGRRGG..",
		".DDDDDDDDDDDDDD.",
		"DDDDDDDDDDDDDDDD",
	},
	{
		".BB.............",
		"..BB............",
		"...BB...........",
		"....BB..........",
		".....GGGGGG.....",
		"....GGWWGGGG....",
		"...GGGGGGGGGG...",
		"..GGRRGGGGRRGG..",
		".DDDDDDDDDDDDDD.",
		"DDDDDDDDDDDDDDDD",
	},
	{
		".......BB.......",
		".......BB.......",
		".......BB.......",
		".......BB.......",
		".....GGGGGG.....",
		"....GGWWGGGG....",
		"...GGGGGGGGGG...",
		"..GGRRGGGGRRGG..",
		".DDDDDDDDDDDDDD.",
		"DDDDDDDDDDDDDDDD",
	},
};
static const hstx_dvi_pixel_t _turret_px[] = {
	RR_RGB(224, 224, 224), RR_RGB(128, 128, 192), RR_RGB(255, 255, 255), RR_RGB(255, 32, 32), RR_RGB(64, 64, 112),
};

// The asteroids are made when the game starts, as a lumpy, cratered rock lit
// from the top left and turned a little more in each frame, so it spins
// while the light stays put. A lump is a bump on the outline, a crater a dent
// in the surface, both in the rock's own turning coordinates.
typedef struct { float amp, k, phase; } RrLump_t;
typedef struct { float u, v, r; } RrCrater_t;
typedef struct {
	uint32_t size;          // pixels across
	float radius;           // before the lumps
	const RrLump_t* lump;
	uint32_t lumps;
	const RrCrater_t* crater;
	uint32_t craters;
} RrRock_t;

static const RrLump_t _meteor_lumps[] = { { 0.10f, 2, 0.3f }, { 0.08f, 3, 1.9f }, { 0.05f, 5, 4.0f } };
static const RrCrater_t _meteor_craters[] = {
	{ -3.5f, -2.5f, 3.2f }, { 4.0f, 3.0f, 2.6f }, { 3.5f, -5.0f, 1.8f }, { -4.0f, 5.0f, 2.0f }, { 0.5f, 1.0f, 1.4f },
};
static const RrRock_t _meteor = { 24, 10.6f, _meteor_lumps, 3, _meteor_craters, 5 };

static const RrLump_t _rock_lumps[] = { { 0.13f, 2, 1.0f }, { 0.09f, 3, 3.1f } };
static const RrCrater_t _rock_craters[] = { { -1.5f, -1.0f, 1.8f }, { 2.0f, 1.8f, 1.3f } };
static const RrRock_t _rock = { 12, 5.3f, _rock_lumps, 2, _rock_craters, 2 };

// Dark edge, shadows up to the highlight, the cold light of space round the
// far side, and the floor of a crater in shadow
#define ROCK_KEY "K12345BC"
static const hstx_dvi_pixel_t _rock_px[] = {
	RR_RGB(36, 36, 85), RR_RGB(73, 55, 85), RR_RGB(109, 73, 85), RR_RGB(146, 109, 85),
	RR_RGB(196, 150, 85), RR_RGB(240, 205, 170), RR_RGB(73, 109, 170), RR_RGB(55, 36, 85),
};

#define RR_ROCK_MAX 24

static float rock_edge(const RrRock_t* k, float phi) {
	float r = 1.0f;
	for (uint32_t i = 0; i < k->lumps; ++i) r += k->lump[i].amp * cosf(k->lump[i].k * phi + k->lump[i].phase);
	return k->radius * r;
}

// One frame of a rock, turned by a, into buf
static void rock_frame(const RrRock_t* k, float a, char buf[][RR_ROCK_MAX + 1]) {
	const uint32_t n = k->size;
	const float c = (n - 1) * 0.5f;
	const float ca = cosf(a), sa = sinf(a);
	// The light, from the top left and in front
	const float lx = -0.55f, ly = -0.62f, lz = 0.56f;
	bool in[RR_ROCK_MAX + 2][RR_ROCK_MAX + 2];
	memset(in, 0, sizeof(in));
	for (uint32_t y = 0; y < n; ++y) {
		for (uint32_t x = 0; x < n; ++x) {
			const float dx = x - c, dy = y - c;
			const float r = sqrtf(dx * dx + dy * dy);
			in[y + 1][x + 1] = r < rock_edge(k, atan2f(dy, dx) - a);
		}
	}
	for (uint32_t y = 0; y < n; ++y) {
		for (uint32_t x = 0; x < n; ++x) {
			char* p = &buf[y][x];
			*p = '.';
			if (!in[y + 1][x + 1]) continue;
			if (!in[y][x + 1] || !in[y + 2][x + 1] || !in[y + 1][x] || !in[y + 1][x + 2]) {
				*p = 'K';
				continue;
			}
			const float dx = x - c, dy = y - c;
			const float r = sqrtf(dx * dx + dy * dy) + 0.001f;
			const float s = fminf(r / rock_edge(k, atan2f(dy, dx) - a), 1.0f);
			const float nx = dx / r * s, ny = dy / r * s, nz = sqrtf(1.0f - s * s);
			float b = nx * lx + ny * ly + nz * lz;
			// Round the far side, lit by the stars
			if (s > 0.72f && (nx + ny) > 0.95f) {
				*p = 'B';
				continue;
			}
			bool floor = false;
			for (uint32_t i = 0; i < k->craters; ++i) {
				const RrCrater_t* m = &k->crater[i];
				// Where the crater is now it has turned
				const float mx = m->u * ca - m->v * sa, my = m->u * sa + m->v * ca;
				const float ex = dx - mx, ey = dy - my;
				const float d = sqrtf(ex * ex + ey * ey);
				// Towards the light; the wall that side is in shadow, the far one lit
				const float t = (ex * lx + ey * ly) / m->r;
				if (d < m->r) {
					b = b - 0.25f - 0.45f * t;
					floor = d < m->r * 0.5f && t > -0.2f;
				}
				else if (d < m->r + 1.0f && t > 0.3f) b += 0.3f;
			}
			if (floor && b < 0.1f) *p = 'C';
			else if (b < -0.05f) *p = '1';
			else if (b < 0.25f) *p = '2';
			else if (b < 0.6f) *p = '3';
			else if (b < 0.9f) *p = '4';
			else *p = '5';
		}
		buf[y][n] = 0;
	}
}

// frames of a whole turn
static void make_rock(RrArt_t* art, uint32_t frames, const RrRock_t* k) {
	static char buf[RR_ROCK_MAX][RR_ROCK_MAX + 1];
	const char* rows[RR_ROCK_MAX];
	for (uint32_t i = 0; i < RR_ROCK_MAX; ++i) rows[i] = buf[i];
	for (uint32_t f = 0; f < frames; ++f) {
		rock_frame(k, f * 6.2831853f / frames, buf);
		rr_art_make(&art[f], rows, k->size, ROCK_KEY, _rock_px, 2, false);
	}
}

static const char* const _bullet[] = {
	".PP.",
	"PWWP",
	"PWWP",
	".PP.",
};
static const hstx_dvi_pixel_t _bullet_px[2][2] = {
	{ RR_RGB(255, 64, 160), RR_RGB(255, 255, 255) },
	{ RR_RGB(255, 160, 0), RR_RGB(255, 255, 160) },
};

// ----------------------------------------------------------------------------
// The warship, its jaws open at the front round the core, walled off

static const char* const _hull[] = {
	".............................................KKKKKKKKKKKKKKK....",
	"...........................................KKLLLLLLLLLLLLLLK....",
	".........................................KKLLHHHHHHHHHDHHHHK....",
	".......................................KKLLHHHHHHHRHHHDHHHHK....",
	".............................KKKKKKKKKKLLHDHHHHHHHHHHHDHHHK.....",
	"...........................KKLLLLLLLLLLHHHDHHHHHHHHHHHDHHHLK....",
	".........................KKLLHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHLK...",
	".......................KKLLHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHLK..",
	".....................KKLLHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHLK.",
	"..................KKKLLHHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHLK",
	"................KKLLLHHHHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHOO",
	"KKRKKKKKKKKKKKKKLLHHHHHHHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHOO",
	"KLLLLLLLLLLLLLLLHHHHHHHHHHHHDHHBBHHBBHHBBHDBBHHBBHHBBHDHHHHHHHOO",
	"KHHHHHHHHHHHHHHHHHHHHHHHHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHOO",
	"KKKKKKKKKKKKKKKKKKKKKKDDHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHOO",
	"......................KDHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHOO",
	"......................KDHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHOO",
	"......................KDHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHHK",
	"......................KDHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHHK",
	"......................KDHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHHK",
	"......................KDHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHHK",
	"......................KDHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHHK",
	"......................KDHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHHK",
	"......................KDHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHOO",
	"......................KDHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHOO",
	"KKKKKKKKKKKKKKKKKKKKKKDDHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHOO",
	"KHHHHHHHHHHHHHHHHHHHHHHHHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHOO",
	"KDDDDDDDDDDDDDDDHHHHHHHHHHHHDHHBBHHBBHHBBHDBBHHBBHHBBHDHHHHHHHOO",
	"KKRKKKKKKKKKKKKKDDHHHHHHHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHOO",
	"................KKDDDHHHHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHOO",
	"..................KKKDDHHHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHHDK",
	".....................KKDDHHHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHHDK.",
	".......................KKDDHDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHHDK..",
	".........................KKDDHHHHHHHHHHHHHDHHHHHHHHHHHDHHHHDK...",
	"...........................KKDDDDDDDDDDHHHDHHHHHHHHHHHDHHHDK....",
	".............................KKKKKKKKKKDDHDHHHHHHHHHHHDHHHK.....",
	".......................................KKDDHHHHHHHRHHHDHHHHK....",
	".........................................KKDDHHHHHHHHHDHHHHK....",
	"...........................................KKDDDDDDDDDDDDDDK....",
	".............................................KKKKKKKKKKKKKKK....",
};
static const hstx_dvi_pixel_t _hull_px[] = {
	RR_RGB(48, 48, 80), RR_RGB(192, 192, 224), RR_RGB(128, 128, 160), RR_RGB(80, 80, 112),
	RR_RGB(255, 32, 32), RR_RGB(0, 192, 255), RR_RGB(255, 128, 0),
};

static const char* const _core[] = {
	"....CCCC....",
	"..CCCCCCCC..",
	".CCCWWWWCCC.",
	"CCCWWWWWWCCC",
	"CCWWWWWWWWCC",
	"CCWWWWWWWWCC",
	"CCCWWWWWWCCC",
	".CCCWWWWCCC.",
	"..CCCCCCCC..",
	"....CCCC....",
};
static const hstx_dvi_pixel_t _core_px[3][2] = {
	{ RR_RGB(0, 160, 255), RR_RGB(224, 255, 255) },
	{ RR_RGB(0, 64, 224), RR_RGB(0, 224, 255) },
	{ RR_RGB(160, 0, 224), RR_RGB(255, 128, 255) },
};

static const char* const _wall[] = {
	"LGD", "LGD", "LGD", "LGD", "LGD", "LGD", "LGD", "LGD", "LGD", "LGD",
};
static const hstx_dvi_pixel_t _wall_px[] = { RR_RGB(224, 224, 255), RR_RGB(128, 160, 224), RR_RGB(64, 80, 160) };

// ----------------------------------------------------------------------------
// The giants, from the second time round, drawn three times the size

// A jellyfish, drifting with its bell wide, and squeezed to push up
static const char* const _jelly[2][21] = {
	{
		"......PPPPPPPP......",
		"....PPLLLLLLLLPP....",
		"...PLLWWLLLLLLLLP...",
		"..PLLWWLLLLLLLWLLP..",
		".PLLLLLLLLLLLLLLLLP.",
		".PLLLLLDDLLDDLLLLLP.",
		"PLLLLLDDDDDDDDLLLLLP",
		"PPPPPPPPPPPPPPPPPPPP",
		"P.P..P..P..P..P..P.P",
		"..T..T..T..T..T..T..",
		"..T...T..T..T..T..T.",
		"...T..T...T..T..T...",
		"...T...T..T...T..T..",
		"..T....T...T..T...T.",
		"..T...T....T...T..T.",
		"...T..T...T....T..T.",
		"...T...T..T...T..T..",
		"....T..T...T..T..T..",
		"....T...T..T...T....",
		".....T..T...T..T....",
		".....T...T..T...T...",
	},
	{
		".......PPPPPP.......",
		".....PPLLLLLLPP.....",
		"....PLLWWLLLLLLP....",
		"....PLWWLLLLLWLP....",
		"...PLLLLLLLLLLLLP...",
		"...PLLLDDLLDDLLLP...",
		"...PLLDDDDDDDDLLP...",
		"...PPPPPPPPPPPPPP...",
		"...P.P..P..P..P.P...",
		"....T..T..T..T..T...",
		"....T..T..T..T..T...",
		".....T.T..T..T.T....",
		".....T..T.T.T..T....",
		"......T.T.T.T.T.....",
		"......T.T.T.T.T.....",
		".......T.T.T.T......",
		".......T.T.T.T......",
		"........T.T.T.......",
		"........T.T.T.......",
		"........T...T.......",
		"....................",
	},
};
static const hstx_dvi_pixel_t _jelly_px[] = {
	RR_RGB(200, 0, 160), RR_RGB(255, 128, 224), RR_RGB(255, 255, 255), RR_RGB(160, 0, 224), RR_RGB(128, 224, 255),
};

// A mosquito, facing left, its proboscis out in front
static const char* const _mosquito[2][12] = {
	{
		"............WWW..WWW....",
		"...........WWWWWWWWWWW..",
		"............WWWWWWWWW...",
		".............WW..WW.....",
		".......HHH...BB.........",
		"NNNNNHHRHHBBBBSSBBSSBB..",
		".......HHHBBBBSSBBSSBBBB",
		"........L..L..L.........",
		".......L...L...L........",
		"......L...L.....L.......",
		".....L...L.......L......",
		"....L...L.........L.....",
	},
	{
		"........................",
		"........................",
		"...........WWWWWWWWWWWW.",
		"............WWWWWWWWW...",
		".......HHH...BB.........",
		"NNNNNHHRHHBBBBSSBBSSBB..",
		".......HHHBBBBSSBBSSBBBB",
		"........L..L..L.........",
		".......L....L...L.......",
		"......L......L....L.....",
		".....L.......L.....L....",
		"....L.........L.....L...",
	},
};
static const hstx_dvi_pixel_t _mosquito_px[] = {
	RR_RGB(192, 224, 255), RR_RGB(224, 128, 64), RR_RGB(255, 0, 0), RR_RGB(160, 96, 64),
	RR_RGB(255, 176, 96), RR_RGB(160, 64, 32), RR_RGB(200, 160, 128),
};

// A wasp, facing right, its sting behind
static const char* const _wasp[2][12] = {
	{
		"........WWWW..WWWW......",
		".......WWWWWWWWWWWW.....",
		"........WWWWWWWWWW......",
		".........WW...WW......A.",
		"...KKYYKKYYKKTTTT...HHA.",
		".KYYKKYYKKYYKTTTTTHHHHH.",
		"SKYYKKYYKKYYKTTTTTHHRHHH",
		".KYYKKYYKKYYKTTTTTHHHHH.",
		"...KKYYKKYYKKTTTT..HHH..",
		"..........L..L..L.......",
		".........L..L..L........",
		"........L..L..L.........",
	},
	{
		"........................",
		"........................",
		".....WWWWWWWWWWWWW......",
		".......WWWWWWWWWW.....A.",
		"...KKYYKKYYKKTTTT...HHA.",
		".KYYKKYYKKYYKTTTTTHHHHH.",
		"SKYYKKYYKKYYKTTTTTHHRHHH",
		".KYYKKYYKKYYKTTTTTHHHHH.",
		"...KKYYKKYYKKTTTT..HHH..",
		"...........L..L..L......",
		"..........L..L..L.......",
		".........L..L..L........",
	},
};
static const hstx_dvi_pixel_t _wasp_px[] = {
	RR_RGB(200, 230, 255), RR_RGB(48, 48, 48), RR_RGB(32, 32, 32), RR_RGB(255, 224, 0),
	RR_RGB(200, 120, 0), RR_RGB(255, 176, 0), RR_RGB(255, 0, 0), RR_RGB(64, 64, 64), RR_RGB(48, 48, 48),
};
#define WASP_KEY "WAKYTHRSL"

// A left to right mirror image of rows up to 32 wide, into buf
static void mirror(const char* const* rows, uint32_t h, char buf[][33], const char** out) {
	for (uint32_t r = 0; r < h; ++r) {
		const uint32_t n = strlen(rows[r]) < 32 ? strlen(rows[r]) : 32;
		for (uint32_t i = 0; i < n; ++i) buf[r][i] = rows[r][n - 1 - i];
		buf[r][n] = 0;
		out[r] = buf[r];
	}
}

void rr_gfx_init(void) {
	static bool made = false;
	if (made) return;
	made = true;
	RrGfx_t* g = &rr_gfx;
	rr_art_make(&g->ship, _ship, RR_ROWS(_ship), SHIP_KEY, _ship_px, 2, false);
	rr_art_make(&g->ship_small, _ship, RR_ROWS(_ship), SHIP_KEY, _ship_px, 1, false);
	for (uint32_t i = 0; i < 2; ++i) {
		rr_art_make(&g->flame[i], _flame[i], 5, "YW", _flame_px, 2, false);
		rr_art_make(&g->drone[i], _drone, RR_ROWS(_drone), "OYWR", _drone_px[i], 2, false);
		rr_art_make(&g->orb[i], _orb[i], 12, "WPM", _orb_px, 2, false);
		rr_art_make(&g->capsule[i], _capsule, RR_ROWS(_capsule), "ROW", _capsule_px[i], 2, false);
		rr_art_make(&g->dart[i], _dart[i], 9, DART_KEY, _dart_px, 2, false);
		rr_art_make(&g->carrier[i], _dart[i], 9, DART_KEY, _carrier_px, 2, false);
		rr_art_make(&g->saucer[i], _saucer[i], 6, "CWGRD", _saucer_px, 2, false);
		rr_art_make(&g->rocket_flame[i], _rocket_flame[i], 5, "YW", _flame_px, 2, false);
		rr_art_make(&g->bullet[i], _bullet, RR_ROWS(_bullet), "PW", _bullet_px[i], 2, false);
	}
	rr_art_make(&g->shot, _shot, RR_ROWS(_shot), "YW", _shot_px, 2, false);
	rr_art_make(&g->twin, _twin, RR_ROWS(_twin), "YW", _shot_px, 2, false);
	rr_art_make(&g->triple, _twin, RR_ROWS(_twin), "YW", _shot_px, 2, true);
	rr_art_make(&g->big_laser, _big_laser, RR_ROWS(_big_laser), "PW", _big_laser_px, 2, false);
	rr_art_make(&g->laser, _laser, RR_ROWS(_laser), "CW", _laser_px, 2, false);
	rr_art_make(&g->bomb, _bomb, RR_ROWS(_bomb), "KOW", _bomb_px, 2, false);
	rr_art_make(&g->rocket, _rocket, RR_ROWS(_rocket), "RWBG", _rocket_px, 2, false);
	rr_art_make(&g->fuel, _fuel, RR_ROWS(_fuel), "GWRD", _fuel_px, 2, false);
	for (uint32_t i = 0; i < 3; ++i) {
		rr_art_make(&g->turret[i], _turret[i], 10, "BGWRD", _turret_px, 2, false);
		rr_art_make(&g->turret_c[i], _turret[i], 10, "BGWRD", _turret_px, 2, true);
		rr_art_make(&g->core[i], _core, RR_ROWS(_core), "CW", _core_px[i], 2, false);
	}
	make_rock(g->meteor, RR_METEOR_FRAMES, &_meteor);
	make_rock(g->rock, RR_ROCK_FRAMES, &_rock);
	rr_art_make(&g->hull, _hull, RR_ROWS(_hull), "KLHDRBO", _hull_px, 2, false);
	rr_art_make(&g->wall, _wall, RR_ROWS(_wall), "LGD", _wall_px, 2, false);
	for (uint32_t i = 0; i < 2; ++i) {
		static char buf[12][33];
		const char* rows[12];
		// Giants: three times the size
		rr_art_make(&g->jelly[i], _jelly[i], 21, "PLWDT", _jelly_px, 3, false);
		rr_art_make(&g->mosquito[i], _mosquito[i], 12, "WHRNBSL", _mosquito_px, 3, false);
		rr_art_make(&g->wasp_r[i], _wasp[i], 12, WASP_KEY, _wasp_px, 3, false);
		mirror(_wasp[i], 12, buf, rows);
		rr_art_make(&g->wasp_l[i], rows, 12, WASP_KEY, _wasp_px, 3, false);
	}
}
