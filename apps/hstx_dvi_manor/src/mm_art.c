#include "mm_art.h"
#include "game_art.h"

#define RGB(r, g, b) HSTX_DVI_PIXEL_RGB(r, g, b)

MmImg_t mm_art_hazel[MM_HAZEL_FRAMES];
MmImg_t mm_art_ghost[MM_MOODS][2];
MmImg_t mm_art_bat[MM_MOODS][2];
MmImg_t mm_art_spider[MM_MOODS][2];
MmImg_t mm_art_pumpkin[MM_MOODS][2];
MmImg_t mm_art_kitten[2];
MmImg_t mm_art_sweet;
MmImg_t mm_art_heart;
MmImg_t mm_art_key;
MmImg_t mm_art_feather;
MmImg_t mm_art_crate;
MmImg_t mm_art_star[2];
MmImg_t mm_art_moon;
MmImg_t mm_art_hat;
MmImg_t mm_art_kitten_face;

// Too big for the stack
static MmCanvas_t _canvas;

// ---------------------------------------------------------------------------
// Hazel

#define HAZEL_KEY "KPpLOoSsWYB"
MmPal_t mm_pal_hazel = {
	0,
	RGB(0, 0, 0), // K
	RGB(160, 64, 192), // P
	RGB(96, 32, 128), // p
	RGB(224, 160, 255), // L
	RGB(255, 128, 0), // O
	RGB(192, 64, 0), // o
	RGB(255, 192, 128), // S
	RGB(255, 128, 160), // s
	RGB(255, 255, 255), // W
	RGB(255, 224, 0), // Y
	RGB(128, 64, 0), // B
};

static const char* const _hazel_side[] = {
	"......PP................",
	".......PPP..............",
	"........PPP.............",
	"........PPPP............",
	".........PPLP...........",
	".........PPPLP..........",
	"........PPPPPLP.........",
	"........PPPPPPLP........",
	".......PPPPPPPPP........",
	".......KKKKKKKKYYK......",
	".......KKKKKKKKYYK......",
	"..PPPPPPPPPPPPPPPPPPPPP.",
	".ppppppppppppppppppppppp",
	"....OOOOOOOOOOOOOOO.....",
	"...OOOOOOOOOOOOOOOOO....",
	"...OOOOOOOOOOOOOOOOOO...",
	"..OOOOOOOOSSSSSSSSSS....",
	"..OOOOOOOSSSSSSSKKSS....",
	"..OOOOOOOSSSSSSSKWSSS...",
	"..OOOOOOOSSSSSSSKKSSSS..",
	"..OOOoOOOSSSSssSSSSSS...",
	"..OOo.OOOSSSSSSSSKKS....",
	"..Oo...OOoSSSSSSSSS.....",
	"..O.....PPPPPPPPPP......",
	"........PPPPPPPPPPP.....",
	".......PPPPPPPPPPSSS....",
	".......pPPPPPPPPPSS.....",
	".......pPPPPPPPPPP......",
	"......ppPPPPPPPPPPP.....",
	".....pppPPPPPPPPPPP.....",
};

// The two rows under the side view
static const char* const _hazel_feet[][2] = {
	{ ".........KK.KK..........", "........KKK.KKK........." },
	{ "......KKK.....KKK.......", ".....KKK.......KKKK....." },
	{ "........KKK.KKK.........", "........KKKKKKK........." },
	{ "........KKKKKK..........", "........................" },
	{ ".......KK.....KK........", "......KK.......KK......." },
};

// The wand held out in front, from row 22
static const char* const _hazel_cast[] = {
	"..Oo...OOoSSSSSSSSS....Y",
	"..O.....PPPPPPPPPP...YYY",
	"........PPPPPPPPPPPPSBY.",
	".......PPPPPPPPPPPPP....",
	".......pPPPPPPPPPP......",
};

// Seeing stars, from row 17
static const char* const _hazel_dizzy[] = {
	"................KSK.....",
	"................SKS.....",
	"................KSK.....",
};

static const char* const _hazel_hat[] = {
	"...........PP...........",
	"..........PPP...........",
	"..........PPPP..........",
	".........PPPLP..........",
	".........PPPPLP.........",
	"........PPPPPLP.........",
	"........PPPPPPLP........",
	".......PPPPPPPPP........",
	".......PPPPPPPPPP.......",
	"......KKKKKYYKKKKK......",
	"......KKKKKYYKKKKK......",
	"..PPPPPPPPPPPPPPPPPPPP..",
	".pppppppppppppppppppppp.",
	"...OOOOOOOOOOOOOOOOOO...",
	"..OOOOOOOOOOOOOOOOOOOO..",
	"..OOOOOOOOOOOOOOOOOOOO..",
};

static const char* const _hazel_front[] = {
	"..OOSSSSSSSSSSSSSSSSOO..",
	"..OOSSSKWSSSSSSKWSSSOO..",
	"..OOSSSKKSSSSSSKKSSSOO..",
	"..OOSssSSSSSSSSSSssSOO..",
	"..OOOSSSSKSSSSKSSSSOOO..",
	"...OO.SSSSKKKKSSSS.OO...",
	"..OO....PPPPPPPP....OO..",
	"..O....PPPPPPPPPP....O..",
	".....SPPPPPPPPPPPPS.....",
	".....SPPPPPPPPPPPPS.....",
	"......PPPPPPPPPPPP......",
	".....pPPPPPPPPPPPPp.....",
	"....ppPPPPPPPPPPPPpp....",
	"....ppPPPPPPPPPPPPpp....",
	".......KK......KK.......",
	"......KKK......KKK......",
};

// Happy eyes and arms in the air, over the front view from row 16
static const char* const _hazel_cheer[] = {
	"..OOSSSSSSSSSSSSSSSSOO..",
	"..OOSSSKKSSSSSSKKSSSOO..",
	"..OOSSKSSKSSSSKSSKSSOO..",
	"..OOSssSSSSSSSSSSssSOO..",
	"..OOOSSSSKSSSSKSSSSOOO..",
	"...OO.SSSSKKKKSSSS.OO...",
	"..OOS...PPPPPPPP...SOO..",
	"..O.SP.PPPPPPPPPP.PS.O..",
	".....PPPPPPPPPPPPPP.....",
	"......PPPPPPPPPPPP......",
};

static void make_hazel(void) {
	MmCanvas_t* c = &_canvas;
	for (uint32_t f = 0; f <= MM_HAZEL_DIZZY; ++f) {
		mm_canvas_clear(c, 24, 32);
		mm_canvas_stamp(c, _hazel_side, GAME_ART_ROWS(_hazel_side), 0, 0);
		uint32_t feet = 0;
		switch (f) {
		case MM_HAZEL_WALK1: feet = 1; break;
		case MM_HAZEL_WALK2: feet = 2; break;
		case MM_HAZEL_JUMP: feet = 3; break;
		case MM_HAZEL_FALL: feet = 4; break;
		case MM_HAZEL_CAST: mm_canvas_stamp(c, _hazel_cast, GAME_ART_ROWS(_hazel_cast), 0, 22); break;
		case MM_HAZEL_DIZZY: mm_canvas_stamp(c, _hazel_dizzy, GAME_ART_ROWS(_hazel_dizzy), 0, 17); break;
		default: break;
		}
		mm_canvas_stamp(c, _hazel_feet[feet], 2, 0, 30);
		mm_img_from_canvas(&mm_art_hazel[f], c, HAZEL_KEY);
	}
	mm_canvas_clear(c, 24, 32);
	mm_canvas_stamp(c, _hazel_hat, GAME_ART_ROWS(_hazel_hat), 0, 0);
	mm_canvas_stamp(c, _hazel_front, GAME_ART_ROWS(_hazel_front), 0, 16);
	mm_img_from_canvas(&mm_art_hazel[MM_HAZEL_FRONT], c, HAZEL_KEY);
	mm_canvas_stamp(c, _hazel_cheer, GAME_ART_ROWS(_hazel_cheer), 0, 16);
	mm_img_from_canvas(&mm_art_hazel[MM_HAZEL_CHEER], c, HAZEL_KEY);
}

// ---------------------------------------------------------------------------
// The ghost, see through while grumpy

#define GHOST_KEY "WGKM"
MmPal_t mm_pal_ghost[MM_MOODS] = {
	{ 0, RGB(224, 224, 255), RGB(128, 160, 255), RGB(0, 0, 64), RGB(255, 128, 192) },
	{ 0, RGB(255, 255, 255), RGB(255, 192, 224), RGB(64, 0, 64), RGB(255, 96, 160) },
};

static const char* const _ghost_body[] = {
	"........WWWWWWW.........",
	"......WWWWWWWWWWW.......",
	".....WWWWWWWWWWWWW......",
	"....WWWWWWWWWWWWWWW.....",
	"...WWWWWWWWWWWWWWWWW....",
	"...WWWWWWWWWWWWWWWWW....",
	"..WWWWWWWWWWWWWWWWWWW...",
	"..WWWWWWWWWWWWWWWWWWW...",
	"..WWWWWWWWWWWWWWWWWWW...",
	"..WWWWWWWWWWWWWWWWWWW...",
	"..WWWWWWWWWWWWWWWWWWW...",
	"..WWWWWWWWWWWWWWWWWWWG..",
	"..WWWWWWWWWWWWWWWWWWWG..",
	".WWWWWWWWWWWWWWWWWWWWG..",
	".WWWWWWWWWWWWWWWWWWWWGG.",
	".WWWWWWWWWWWWWWWWWWWWGG.",
	".WWWWWWWWWWWWWWWWWWWWGG.",
	"WWWWWWWWWWWWWWWWWWWWWGG.",
	"WWWWWWWWWWWWWWWWWWWWWGG.",
	"GWWWWWWWWWWWWWWWWWWWGGG.",
};

static const char* const _ghost_tail[2][3] = {
	{ ".GWWWWGWWWWWGWWWWWWGG...", "..GWWG.GWWWG..GWWWGG....", "...GG...GGG....GGG......" },
	{ "GWWWWWWGWWWWWGWWWWWGG...", ".GWWWG..GWWWWG..GWWG....", "..GGG....GGGG....GG....." },
};

static const char* const _ghost_face[MM_MOODS][7] = { {
	".....KK........KK.......",
	".......KK....KK.........",
	"......KKK....KKK........",
	"......KKK....KKK........",
	"........................",
	"..........KKKK..........",
	".........K....K.........",
}, {
	"........................",
	"......KKK....KKK........",
	".....K...K..K...K.......",
	"........................",
	"....MM...K....K....MM...",
	"..........KKKK..........",
	"........................",
} };

// ---------------------------------------------------------------------------
// The bat, asleep once cheered

#define BAT_KEY "KPpRWM"
MmPal_t mm_pal_bat = { 0, RGB(32, 0, 64), RGB(128, 64, 192), RGB(64, 32, 128), RGB(255, 64, 64), RGB(255, 255, 255), RGB(255, 128, 192) };

static const char* const _bat[3][16] = { {
	"..........K..K..........",
	"K........KMKKMK........K",
	"KK......KPPPPPPK......KK",
	"KpK....KPPPPPPPPK....KpK",
	"KppK...KPRRPPRRPK...KppK",
	"KpppK..KPRWPPRWPK..KpppK",
	"KppppKKKPPPPPPPPKKKppppK",
	".KpppppKPPWPPWPPKpppppK.",
	".KppppppKPPPPPPKppppppK.",
	"..KpppKpKPPPPPPKpKpppK..",
	"...KKK.K.KPPPPK.K.KKK...",
	"..........KKKK..........",
	"........................",
	"........................",
	"........................",
	"........................",
}, {
	"..........K..K..........",
	".........KMKKMK.........",
	"........KPPPPPPK........",
	".......KPPPPPPPPK.......",
	".......KPRRPPRRPK.......",
	".......KPRWPPRWPK.......",
	"...KKKKKPPPPPPPPKKKKK...",
	".KKpppppKPWPPWPKpppppKK.",
	"KppppppKPPPPPPPPKppppppK",
	"KpppppK.KPPPPPPK.KpppppK",
	"KppppK...KPPPPK...KppppK",
	"KpppK.....KKKK.....KpppK",
	"KppK................KppK",
	"KpK..................KpK",
	"KK....................KK",
	"K......................K",
}, {
	"..........K..K..........",
	".........KMKKMK.........",
	"........KPPPPPPK........",
	".......KPPPPPPPPK.......",
	".......KPPPPPPPPK.......",
	".......KPKKPPKKPK.......",
	"......KpPPPPPPPPpK......",
	"......KppPPMMPPppK......",
	"......KpppPPPPpppK......",
	"......KppppppppppK......",
	".......KppppppppK.......",
	"........KKKKKKKK........",
	"........................",
	"........................",
	"........................",
	"........................",
} };

// ---------------------------------------------------------------------------
// The spider, on a thread

#define SPIDER_KEY "kWKMLY"
MmPal_t mm_pal_spider = { 0, RGB(64, 32, 96), RGB(255, 255, 255), RGB(0, 0, 0), RGB(255, 128, 192), RGB(32, 32, 64), RGB(255, 224, 0) };

static const char* const _spider_body[] = {
	"................",
	"......kkkk......",
	"....kkkkkkkk....",
	"...kkkkkkkkkk...",
	"...kWWkkkkWWk...",
	"..kkWKkkkkWKkk..",
	"..kkkkkkkkkkkk..",
	"..kkkkkkkkkkkk..",
	"...kkkkkkkkkk...",
	"....kkkkkkkk....",
	"......kkkk......",
};

static const char* const _spider_legs[2][13] = { {
	"................",
	"................",
	"................",
	"L..............L",
	".L............L.",
	"..L..........L..",
	"L.............L.",
	".L............L.",
	"..L..........L..",
	"L..L........L..L",
	".LL..........LL.",
	"...L........L...",
	"..L..........L..",
}, {
	"................",
	"................",
	"................",
	".L............L.",
	"L..............L",
	"..L..........L..",
	".L............L.",
	"L..............L",
	"..L..........L..",
	".L.L........L.L.",
	"L.L..........L.L",
	"...L........L...",
	"....L......L....",
} };

static const char* const _spider_face[MM_MOODS][4] = { {
	"...KK......KK...",
	"................",
	"................",
	".......KK.......",
}, {
	"....WWWWWWWW....",
	"...kkKkkkkKkk...",
	"..kMkkkkkkkkMk..",
	"......KkkK......",
} };

// ---------------------------------------------------------------------------
// The pumpkin, which lights up when cheered

#define PUMPKIN_KEY "OoKEe"
MmPal_t mm_pal_pumpkin[MM_MOODS] = {
	{ 0, RGB(224, 96, 0), RGB(160, 64, 0), RGB(32, 0, 0), RGB(64, 160, 0), RGB(0, 96, 0) },
	{ 0, RGB(255, 160, 0), RGB(224, 96, 0), RGB(255, 255, 96), RGB(64, 160, 0), RGB(0, 96, 0) },
};

static const char* const _pumpkin_body[] = {
	"........E.......",
	".......EE.e.....",
	"......EEee......",
	"...OOOoOOoOOO...",
	"..OOOOoOOoOOOO..",
	".OOOOoOOOOoOOOO.",
	".OOOOoOOOOoOOOO.",
	"OOOOOoOOOOoOOOOO",
	"OOOOOoOOOOoOOOOO",
	"OOOOOoOOOOoOOOOO",
	"OOOOOoOOOOoOOOOO",
	"OOOOOoOOOOoOOOOO",
	".OOOOoOOOOoOOOO.",
	".OOOOOoOOoOOOOO.",
	"..OOOOOooOOOOO..",
	"....OOOOOOOO....",
};

static const char* const _pumpkin_face[MM_MOODS][6] = { {
	".KK..........KK.",
	"..KKK......KKK..",
	"...KK......KK...",
	"................",
	"...K.KK..KK.K...",
	"....KK.KK.KK....",
}, {
	"...KK......KK...",
	"..KKKK....KKKK..",
	"................",
	"................",
	"..KK........KK..",
	"...KKKKKKKKKK...",
} };

// ---------------------------------------------------------------------------
// Kittens, sitting and waiting to be found

#define KITTEN_KEY "FfWKM"
MmPal_t mm_pal_kitten[MM_KITTEN_COATS] = {
	{ 0, RGB(255, 160, 64), RGB(192, 96, 0), RGB(255, 255, 255), RGB(0, 0, 0), RGB(255, 128, 192) },
	{ 0, RGB(160, 160, 192), RGB(96, 96, 128), RGB(255, 255, 255), RGB(0, 0, 0), RGB(255, 128, 192) },
	{ 0, RGB(64, 64, 64), RGB(32, 32, 32), RGB(255, 255, 255), RGB(96, 224, 64), RGB(255, 128, 192) },
	{ 0, RGB(255, 224, 192), RGB(224, 160, 96), RGB(255, 255, 255), RGB(0, 64, 192), RGB(255, 128, 192) },
	{ 0, RGB(160, 96, 32), RGB(96, 32, 0), RGB(255, 224, 160), RGB(0, 0, 0), RGB(255, 128, 192) },
};

static const char* const _kitten[] = {
	"................",
	"..F.....F.......",
	"..FF...FF.......",
	"..FMFFFMF.......",
	".FFFfFfFFF......",
	".FKWFFFKWF......",
	".FKKFFFKKF......",
	".FMFWMWFMF......",
	"..FFWWWFF.......",
	"...FFFFF......F.",
	"..FFWWWFF....FF.",
	".FFFWWWFFF...F..",
	".FfFWWWFfF..FF..",
	".FFfWWWfFFFFF...",
	".FFFFFFFFFFF....",
	"..WW...WW.......",
};

static const char* const _kitten_blink[] = {
	".FFFFFFFFF......",
	".FKKFFFKKF......",
};

static const char* const _kitten_tail[] = {
	"..............FF",
	"..............F.",
	".............FF.",
	"..FFWWWFF...FF..",
	".FFFWWWFFF.FF...",
};

// ---------------------------------------------------------------------------
// Things to pick up

MmPal_t mm_pal_sweet[MM_SWEET_COLOURS] = {
	{ 0, RGB(255, 64, 128), RGB(255, 192, 224), RGB(255, 255, 255) },
	{ 0, RGB(64, 192, 255), RGB(192, 224, 255), RGB(255, 255, 255) },
	{ 0, RGB(255, 224, 0), RGB(255, 160, 0), RGB(255, 255, 255) },
	{ 0, RGB(64, 224, 64), RGB(192, 255, 128), RGB(255, 255, 255) },
};

static const char* const _sweet[] = {
	"........",
	"M..CC..M",
	"MMCCCCMM",
	"MMCWCCMM",
	"MMCCCCMM",
	"M..CC..M",
	"........",
	"........",
};

MmPal_t mm_pal_heart = { 0, RGB(255, 32, 96), RGB(255, 255, 255), RGB(160, 0, 64) };

static const char* const _heart[] = {
	".RR.RR..",
	"RWRRRRR.",
	"RRRRRRR.",
	"RRRRRRd.",
	".RRRRd..",
	"..RRd...",
	"...d....",
	"........",
};

MmPal_t mm_pal_key = { 0, RGB(255, 192, 0), RGB(255, 255, 128), RGB(160, 96, 0) };

static const char* const _key[] = {
	"................",
	".YYY............",
	"YyYYY...........",
	"YY.YYYYYYYYYYYY.",
	"YYYYYooooooYoYo.",
	".YYYo......Y.Y..",
	"..oo.......o.o..",
	"................",
};

MmPal_t mm_pal_feather = { 0, RGB(96, 224, 255), RGB(255, 255, 255), RGB(255, 224, 0) };

static const char* const _feather[] = {
	"............CC..",
	"...........CWWC.",
	"..........CWWWC.",
	".........CWWWWC.",
	"........CWWWWC..",
	".......CWWWWC...",
	"......CWWWWC....",
	".....CWWWWC.....",
	"....CWWWWC......",
	"....CWWWC.......",
	"...CWWWC........",
	"...CWWC.........",
	"..YCC...........",
	".Y..............",
	"Y...............",
	"................",
};

MmPal_t mm_pal_crate = { 0, RGB(64, 32, 0), RGB(224, 160, 64), RGB(160, 96, 32) };

static const char* const _crate[] = {
	"dddddddddddddddd",
	"dnnnnnnnnnnnnnnd",
	"dnbbbbbbbbbbbbnd",
	"dnnbbbbbbbbbbnnd",
	"dnbnnbbbbbbnnbnd",
	"dnbbnnbbbbnnbbnd",
	"dnbbbnnbbnnbbbnd",
	"dnbbbbnnnnbbbbnd",
	"dnbbbbnnnnbbbbnd",
	"dnbbbnnbbnnbbbnd",
	"dnbbnnbbbbnnbbnd",
	"dnbnnbbbbbbnnbnd",
	"dnnbbbbbbbbbbnnd",
	"dnbbbbbbbbbbbbnd",
	"dnnnnnnnnnnnnnnd",
	"dddddddddddddddd",
};

MmPal_t mm_pal_star = { 0, RGB(255, 224, 0), RGB(255, 255, 255) };

static const char* const _star[2][8] = { {
	"...Y....",
	"...Y....",
	"..YWY...",
	"YYWWWYY.",
	"..YWY...",
	"...Y....",
	"...Y....",
	"........",
}, {
	"Y.....Y.",
	".Y...Y..",
	"..YWY...",
	"..WWW...",
	"..YWY...",
	".Y...Y..",
	"Y.....Y.",
	"........",
} };

static const char* const _hat[] = {
	"....PP..",
	"...PPP..",
	"...PPLP.",
	"..PPPPP.",
	"..YYYYY.",
	"PPPPPPPP",
	".pppppp.",
	"........",
};

static const char* const _kitten_face[] = {
	"F.....F.",
	"FF...FF.",
	"FFFFFFF.",
	"FKFFFKF.",
	"FFWMWFF.",
	".FFFFF..",
	"........",
	"........",
};

MmPal_t mm_pal_kitten_none = { 0, RGB(64, 64, 96), RGB(64, 64, 96), RGB(64, 64, 96), RGB(32, 32, 64), RGB(64, 64, 96) };

MmPal_t mm_pal_moon = { 0, RGB(255, 255, 192), RGB(192, 192, 160), RGB(224, 224, 192) };

MmPal_t mm_pal_white;

// A full moon with a few craters
static void make_moon(void) {
	static const int8_t craters[][3] = { { -5, -4, 3 }, { 4, -6, 2 }, { 5, 4, 3 }, { -3, 6, 2 }, { -8, 2, 1 } };
	MmCanvas_t* c = &_canvas;
	mm_canvas_clear(c, 32, 32);
	for (int32_t y = 0; y < 32; ++y) {
		for (int32_t x = 0; x < 32; ++x) {
			const int32_t dx = 2 * x - 31, dy = 2 * y - 31;
			if (dx * dx + dy * dy > 28 * 28) continue;
			char ch = (dx * dx + dy * dy > 24 * 24 && dx + dy > 0) ? 'C' : 'M';
			for (uint32_t k = 0; k < sizeof(craters) / sizeof(craters[0]); ++k) {
				const int32_t cx = dx - 2 * craters[k][0], cy = dy - 2 * craters[k][1];
				if (cx * cx + cy * cy <= 4 * craters[k][2] * craters[k][2]) ch = 'G';
			}
			mm_canvas_set(c, x, y, ch);
		}
	}
	mm_img_from_canvas(&mm_art_moon, c, "MGC");
}

void mm_art_init(void) {
	MmCanvas_t* c = &_canvas;
	make_hazel();

	for (uint32_t m = 0; m < MM_MOODS; ++m) {
		for (uint32_t f = 0; f < 2; ++f) {
			mm_canvas_clear(c, 24, 23);
			mm_canvas_stamp(c, _ghost_body, GAME_ART_ROWS(_ghost_body), 0, 0);
			mm_canvas_stamp(c, _ghost_tail[f], 3, 0, 20);
			mm_canvas_stamp(c, _ghost_face[m], 7, 0, 6);
			mm_img_from_canvas(&mm_art_ghost[m][f], c, GHOST_KEY);

			mm_canvas_clear(c, 16, 13);
			mm_canvas_stamp(c, _spider_legs[f], 13, 0, 0);
			mm_canvas_stamp(c, _spider_body, GAME_ART_ROWS(_spider_body), 0, 0);
			mm_canvas_stamp(c, _spider_face[m], 4, 0, 4);
			mm_img_from_canvas(&mm_art_spider[m][f], c, SPIDER_KEY);

			mm_canvas_clear(c, 16, 16);
			mm_canvas_stamp(c, _pumpkin_body, GAME_ART_ROWS(_pumpkin_body), 0, f);
			mm_canvas_stamp(c, _pumpkin_face[m], 6, 0, 6 + f);
			mm_img_from_canvas(&mm_art_pumpkin[m][f], c, PUMPKIN_KEY);
		}
	}
	mm_img_make(&mm_art_bat[MM_MOOD_GRUMPY][0], _bat[0], 16, BAT_KEY);
	mm_img_make(&mm_art_bat[MM_MOOD_GRUMPY][1], _bat[1], 16, BAT_KEY);
	mm_img_make(&mm_art_bat[MM_MOOD_HAPPY][0], _bat[2], 16, BAT_KEY);
	mm_img_make(&mm_art_bat[MM_MOOD_HAPPY][1], _bat[2], 16, BAT_KEY);

	mm_canvas_clear(c, 16, 16);
	mm_canvas_stamp(c, _kitten, GAME_ART_ROWS(_kitten), 0, 0);
	mm_img_from_canvas(&mm_art_kitten[0], c, KITTEN_KEY);
	mm_canvas_stamp(c, _kitten_blink, GAME_ART_ROWS(_kitten_blink), 0, 5);
	mm_canvas_stamp(c, _kitten_tail, GAME_ART_ROWS(_kitten_tail), 0, 7);
	// The tail's old place
	mm_canvas_set(c, 14, 9, ' ');
	mm_canvas_set(c, 13, 10, ' ');
	mm_canvas_set(c, 14, 10, ' ');
	mm_img_from_canvas(&mm_art_kitten[1], c, KITTEN_KEY);

	mm_img_make(&mm_art_sweet, _sweet, GAME_ART_ROWS(_sweet), "CMW");
	mm_img_make(&mm_art_heart, _heart, GAME_ART_ROWS(_heart), "RWd");
	mm_img_make(&mm_art_key, _key, GAME_ART_ROWS(_key), "Yyo");
	mm_img_make(&mm_art_feather, _feather, GAME_ART_ROWS(_feather), "CWY");
	mm_img_make(&mm_art_crate, _crate, GAME_ART_ROWS(_crate), "dnb");
	mm_img_make(&mm_art_star[0], _star[0], 8, "YW");
	mm_img_make(&mm_art_star[1], _star[1], 8, "YW");
	mm_img_make(&mm_art_hat, _hat, GAME_ART_ROWS(_hat), HAZEL_KEY);
	mm_img_make(&mm_art_kitten_face, _kitten_face, GAME_ART_ROWS(_kitten_face), KITTEN_KEY);
	make_moon();

	for (uint32_t i = 1; i < MM_PAL_SIZE; ++i) mm_pal_white[i] = RGB(255, 255, 255);
}
