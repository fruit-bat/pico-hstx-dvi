#include "ww_animal.h"
#include "ww_art.h"
#include "ww_world.h"
#include "ww_witch.h"
#include "ww_hud.h"
#include "ww_items.h"
#include "game_pallet.h"
#include "game_util.h"
#include "game_fx.h"
#include <string.h>

#define WW_ANIMAL_MAX 160
// Most in one room, with the wolf and a follower
#define WW_HERE_MAX 8
#define WW_WANDER GAME_PX(1) / 2
#define WW_FOLLOW_SPEED (GAME_PX(2) + GAME_PX(1) / 4)
#define WW_FOLLOW_GAP 26
#define WW_NOTICE 200
#define WW_REUNITE 44
#define WW_SPEAK 130
// A hop takes this many frames, half in the air
#define WW_HOP 32
#define WW_HOP_H 6
#define WW_STUN_FRAMES 30
#define WW_WOLF_STUN 100
#define WW_JOY_FRAMES 150
#define WW_DIAG 181
#define WW_STUCK 20

typedef struct {
	const char* name;
	const char* key;
	uint8_t colours[4];
	// The colour of its eyes when they are shut
	uint8_t shut;
	uint8_t w;
	uint8_t body_h;
	uint8_t legs_h;
	const char* const* body;
	const char* const* legs[2];
	uint8_t hw; // half the width of its feet
	bool hop;
	uint8_t heal;
	uint8_t calm;
	int16_t speed; // when grumpy
	uint16_t heal_score;
	uint16_t calm_score;
	uint8_t wake;  // how near the witch can come while it sleeps
} WwAnimalInfo_t;

static const char* const _bunny[] = {
	"..............WW.WW.....",
	".............WPWWPW.....",
	".............WPWWPW.....",
	".............WPWWPW.....",
	"..............WWWW......",
	"............WWWWWWWW....",
	"...........WWWWWWWWWW...",
	"...........WWWWWWEWWWW..",
	"...........WWWWWWEWWWWP.",
	"...WW......WWWWWWWWWWW..",
	"..WWWW....WWWWWWWWWWW...",
	"..WWWWWWWWWWWWWWWWWW....",
	"...WWWWWWWWWWWWWWWWW....",
	"....WWWWWWWWWWWWWWWW....",
	"....WWWWWWWWWWWWWWW.....",
};
static const char* const _bunny_legs[2][3] = {
	{ ".....WWWWWWWWWWWW.......", ".....WWW.....WWWW.......", "....WWWW....WWWW........" },
	{ "....WWWWWWWWWWWW........", "..WWWW.........WWW......", ".WWW............WWW....." },
};

static const char* const _frog[] = {
	"..WWW....WWW....",
	".WWEEW..WWEEW...",
	".WWEEW..WWEEW...",
	"GGWWWGGGGWWWGG..",
	"GGGGGGGGGGGGGG..",
	"GEGGGGGGGGGGEG..",
	"GGEEEEEEEEEEGG..",
	".GGGGYYYYGGGG...",
	"..GGYYYYYYGG....",
	".GGGYYYYYYGGG...",
	"GGGGGYYYYGGGGG..",
};
static const char* const _frog_legs[2][3] = {
	{ "GGG.GGGGGG.GGG..", "GG..GG..GG..GG..", "................" },
	{ ".GG.GYYYYG.GG...", "GG...G..G...GG..", "G...GG..GG...G.." },
};

static const char* const _hedgehog[] = {
	"......N..N..N...........",
	"....N.NN.NN.NN.N........",
	"...NNNBNNBNNBNNNN.......",
	"..NNBNNNBNNBNNBNNN......",
	".NNNNBNNNBNNNBNNNSS.....",
	".NBNNNNBNNNNBNNNSSSS....",
	"NNNNBNNNNBNNNNNSSESSS...",
	"NNBNNNNBNNNNBNSSSSSSSS..",
	"NNNNNBNNNNBNNNSSSSSSSNN.",
	".NNBNNNNBNNNNSSSSSSSSN..",
	".NNNNNBNNNNNSSSSSSSSS...",
	"..NNNNNNNNNSSSSSSSS.....",
	"...SSSSSSSSSSSSSSS......",
};
static const char* const _hedgehog_legs[2][2] = {
	{ "....SS..SS...SS..SS.....", "....NN..NN...NN..NN....." },
	{ ".....SS..SS.SS..SS......", ".....NN..NN.NN..NN......" },
};

static const char* const _fox[] = {
	"........................K...K...",
	".......................OK..OK...",
	".......................OO..OO...",
	".......................OOOOOO...",
	"......................OOOOOOO...",
	"WWW...................OOOEOOOO..",
	"WWOO..................OOOEOOOOO.",
	".OOOO.................WWOOOOOWWK",
	"..OOOO................WWWWWWWW..",
	"...OOOO..............WWWWWWW....",
	"....OOOO.OOOOOOOOOOOOWWWWW......",
	".....OOOOOOOOOOOOOOOOOWWW.......",
	"......OOOOOOOOOOOOOOOOOWW.......",
	"........OOOOOOOOOOOOOOO.........",
	"........OOOOOOOOOOOOOO..........",
	"........OOOOOOOOOOOOOO..........",
	"........OOOOWWWWWWOOOO..........",
};
static const char* const _fox_legs[2][3] = {
	{ "........OO.OO.....OO.OO.........", "........OO.OO.....OO.OO.........", "........KK.KK.....KK.KK........." },
	{ ".......OO...OO...OO...OO........", "......OO.....OO.OO.....OO.......", "......KK.....KK.KK.....KK......." },
};

static const char* const _badger[] = {
	"........................WW......",
	"....................GGKKWWKK....",
	"...............GGGGGGKKWWWWKK...",
	"..........GGGGGGGGGGGKKWWWWWKK..",
	"......GGGGGGGGGGGGGGGKWWEWWWWWK.",
	"....GGGGGGGGGGGGGGGGGGKKWWWWWWWK",
	"...GGGGGGGGGGGGGGGGGGGKKWWWWWW..",
	"..GGGGGGGGGGGGGGGGGGGGGKKWWW....",
	".GGGGGGGGGGGGGGGGGGGGGGGG.......",
	".GGGGGGGGGGGGGGGGGGGGGGG........",
	"GGGGGGGGGGGGGGGGGGGGGGG.........",
	".GGGGGGGGGGGGGGGGGGGGGG.........",
	"..KKKKKKKKKKKKKKKKKKKKK.........",
};
static const char* const _badger_legs[2][3] = {
	{ "...KKK.KKK.....KKK.KKK..........", "...KKK.KKK.....KKK.KKK..........", "...KK..KK......KK..KK..........." },
	{ "..KKK...KKK...KKK...KKK.........", ".KKK.....KKK.KKK.....KKK........", ".KK.......KK.KK.......KK........" },
};

static const char* const _deer[] = {
	".....................BB...BB....",
	"......................BB.BB.....",
	".......................BBBB.....",
	".......................BBBBB....",
	"......................BBBBBBB...",
	"......................BBBEBBBB..",
	"......................BBBBBBBBB.",
	"......................BBBBBWWWK.",
	"......................BBBBWWWW..",
	".....................BBBBB......",
	".....................BBBB.......",
	"....WW..............BBBB........",
	"...BWWBBBBBBBBBBBBBBBBBB........",
	"...BBBBBBBWBBBBWBBBBBBBB........",
	"...BBBBWBBBBBWBBBBBWBBBB........",
	"...BBBBBBBBBBBBBBBBBBBBB........",
	"....BBBBWBBBBBBWBBBBBBBB........",
	".....BBBBBBBBBBBBBBBBBB.........",
	".....BBWWWWWWWWWWWWBBBB.........",
};
static const char* const _deer_legs[2][9] = {
	{
		".....BB.BB.......BB.BB..........",
		".....BB.BB.......BB.BB..........",
		".....BB.BB.......BB.BB..........",
		".....BB.BB.......BB.BB..........",
		".....BB.BB.......BB.BB..........",
		".....BB.BB.......BB.BB..........",
		".....BB.BB.......BB.BB..........",
		".....BB.BB.......BB.BB..........",
		".....KK.KK.......KK.KK..........",
	},
	{
		".....BB..BB.....BB..BB..........",
		"....BB....BB...BB....BB.........",
		"....BB....BB...BB....BB.........",
		"...BB......BB.BB......BB........",
		"...BB......BB.BB......BB........",
		"...BB......BB.BB......BB........",
		"..BB........BBB........BB.......",
		"..BB........BBB........BB.......",
		"..KK........KKK........KK.......",
	},
};

static const char* const _boar[] = {
	"...B....B.B.B.B.B...............",
	"..B.B.BBNNNNNNNNNNN.....NN......",
	"...BB.NNNNNNNNNNNNNNNN.NNN......",
	"....NNNNNNNNNNNNNNNNNNNNNNN.....",
	"...NNNNNNNNNNNNNNNNNNNNNENNN....",
	"..NNNNNNNNNNNNNNNNNNNNNNNNNNNBB.",
	"..NNNNNNNNNNNNNNNNNNNNNNNNNNBBBB",
	".NNNNNNNNNNNNNNNNNNNNNNNNNWNBEBE",
	".NNNNNNNNNNNNNNNNNNNNNNNNNWWBBBB",
	"NNNNNNNNNNNNNNNNNNNNNNNNNN.WW...",
	"NNNNNNNNNNNNNNNNNNNNNNNNN.......",
	".NNNNNNNNNNNNNNNNNNNNNNNN.......",
	"..NNNNNNNNNNNNNNNNNNNNNN........",
	"...NNNNNNNNNNNNNNNNNNNN.........",
};
static const char* const _boar_legs[2][3] = {
	{ "...NNN.NNN.......NNN.NNN........", "...NNN.NNN.......NNN.NNN........", "...EEE.EEE.......EEE.EEE........" },
	{ "..NNN...NNN.....NNN...NNN.......", ".NNN.....NNN...NNN.....NNN......", ".EEE.....EEE...EEE.....EEE......" },
};

static const char* const _bear[] = {
	"...................BB....BB.....",
	"..................BSSB..BSSB....",
	"..................BBBBBBBBBB....",
	".................BBBBBBBBBBBB...",
	".................BBBBBBBEBBBB...",
	".................BBBBBBBBBBSSS..",
	".................BBBBBBBBBSSSSK.",
	"........BBBBBBBBBBBBBBBBBBSSSS..",
	".....BBBBBBBBBBBBBBBBBBBBBBSS...",
	"...BBBBBBBBBBBBBBBBBBBBBBBBB....",
	"..BBBBBBBBBBBBBBBBBBBBBBBBB.....",
	".BBBBBBBBBBBBBBBBBBBBBBBBBB.....",
	".BBBBBBBBBBBBBBBBBBBBBBBBBB.....",
	"BBBBBBBBBBBBBBBBBBBBBBBBBBB.....",
	"BBBBBBBBBBBBBBBBBBBBBBBBBB......",
	"BBBBBBBBBBBBBBBBBBBBBBBBBB......",
	".BBBBBBBBBBBBBBBBBBBBBBBBB......",
	".BBBBBSSSSSSSSSSSSSSSBBBB.......",
	"..BBBBBSSSSSSSSSSSSSBBBB........",
};
static const char* const _bear_legs[2][5] = {
	{
		"..BBBBB..........BBBBBB.........",
		"..BBBB...........BBBBB..........",
		"..BBBB...........BBBBB..........",
		"..BBBB...........BBBBB..........",
		"..KBKB...........KBKBK..........",
	},
	{
		".BBBBB............BBBBBB........",
		"BBBB..............BBBBB.........",
		"BBB................BBBBB........",
		"BBB.................BBBB........",
		"KBK.................KBKB........",
	},
};

static const char* const _owl[] = {
	"...BB..............BB...",
	"...BBB............BBB...",
	"....BBBBBBBBBBBBBBBB....",
	"...BBBBBBBBBBBBBBBBBB...",
	"..BBSSSSSSBBBBSSSSSSBB..",
	"..BSSYYYYSSBBSSYYYYSSB..",
	"..BSYYEEYYSSSSYYEEYYSB..",
	"..BSYYEEYYSSSSYYEEYYSB..",
	"..BSSYYYYSSYYSSYYYYSSB..",
	"..BBSSSSSSSYYSSSSSSSBB..",
	"..BBBBBBBBBBYBBBBBBBBB..",
	"..BBBBSSBBBBBBBBSSBBBB..",
	".BBBBSSSSBBBBBBSSSSBBBB.",
	".BBBBBSSBBBSSBBBSSBBBBB.",
	".BBBBBBBBBSSSSBBBBBBBBB.",
	".BBBBBBBBBBSSBBBBBBBBBB.",
	"..BBBBBBBBBBBBBBBBBBBB..",
	"..BBBBBBBBBBBBBBBBBBBB..",
	"...BBBBBBBBBBBBBBBBBB...",
	"....BBBBBBBBBBBBBBBB....",
};
static const char* const _owl_legs[2][2] = {
	{ ".....YY.YY....YY.YY.....", ".....Y..Y......Y..Y....." },
	{ "......YYYY....YYYY......", "......Y..Y....Y..Y......" },
};

static const char* const _owlet[] = {
	".G..........G...",
	".GGGGGGGGGGGG...",
	"GGWWWWGGWWWWGG..",
	"GWWYYWWWWYYWWG..",
	"GWYEEYWWYEEYWG..",
	"GWWYYWYYWYYWWG..",
	"GGWWWWWYWWWWGG..",
	"GGGGGGGGGGGGGG..",
	"GGGWWGGGGWWGGG..",
	"GGWWWWGGWWWWGG..",
	".GGWWGGGGWWGG...",
	"..GGGGGGGGGG....",
};
static const char* const _owlet_legs[2][2] = {
	{ "...YY....YY.....", "..Y.Y....Y.Y...." },
	{ "....YY..YY......", "...Y.Y..Y.Y....." },
};

static const char* const _wolf[] = {
	"......................G...G.....",
	"......................GG..GG....",
	".....................GGGGGGGG...",
	".....................GGGGGGGGG..",
	".....................GGEGGGGGG..",
	".....................GGGGGGWWWW.",
	"WW...................GGGGGWWWWWK",
	"WGG.................GGGGGWWWWW..",
	"GGGG...............GGGGGWWW.....",
	".GGGG..GGGGGGGGGGGGGGGGGWW......",
	"..GGGGGGGGGGGGGGGGGGGGGWW.......",
	"...GGGGGGGGGGGGGGGGGGGGW........",
	"....GGGGGGGGGGGGGGGGGGGG........",
	"......GGGGGGGGGGGGGGGGGG........",
	"......GGGGGWWWWWWWWWGGGG........",
};
static const char* const _wolf_legs[2][4] = {
	{ "......GG.GG.......GG.GG.........", "......GG.GG.......GG.GG.........", "......GG.GG.......GG.GG.........", "......KK.KK.......KK.KK........." },
	{ ".....GG...GG.....GG...GG........", "....GG.....GG...GG.....GG.......", "....GG.....GG...GG.....GG.......", "....KK.....KK...KK.....KK......." },
};

static const WwAnimalInfo_t _info[WW_ANIMAL_TYPES] = {
	{ "BUNNY", "WPE", { GAME_WHITE, GAME_PINK, GAME_BLACK }, GAME_WHITE,
		24, GAME_ART_ROWS(_bunny), 3, _bunny, { _bunny_legs[0], _bunny_legs[1] }, 8, true, 1, 1, GAME_PX(1) + 64, 100, 50, 60 },
	{ "FROG", "GYWE", { GAME_GREEN, GAME_YELLOW, GAME_WHITE, GAME_BLACK }, GAME_GREEN,
		16, GAME_ART_ROWS(_frog), 3, _frog, { _frog_legs[0], _frog_legs[1] }, 6, true, 1, 1, GAME_PX(1), 100, 50, 60 },
	{ "HEDGEHOG", "NBSE", { GAME_DARK_BROWN, GAME_BROWN, GAME_PEACH, GAME_BLACK }, GAME_PEACH,
		24, GAME_ART_ROWS(_hedgehog), 2, _hedgehog, { _hedgehog_legs[0], _hedgehog_legs[1] }, 9, false, 1, 1, GAME_PX(1) * 3 / 4, 100, 50, 70 },
	{ "FOX", "OWKE", { GAME_ORANGE, GAME_WHITE, GAME_BLACK, GAME_BLACK }, GAME_ORANGE,
		32, GAME_ART_ROWS(_fox), 3, _fox, { _fox_legs[0], _fox_legs[1] }, 10, false, 1, 2, GAME_PX(1) + 150, 100, 100, 80 },
	{ "BADGER", "GWKE", { GAME_GREY, GAME_WHITE, GAME_BLACK, GAME_BLACK }, GAME_WHITE,
		32, GAME_ART_ROWS(_badger), 3, _badger, { _badger_legs[0], _badger_legs[1] }, 11, false, 1, 2, GAME_PX(1) + 64, 100, 100, 80 },
	{ "DEER", "BWKE", { GAME_BROWN, GAME_WHITE, GAME_BLACK, GAME_BLACK }, GAME_BROWN,
		32, GAME_ART_ROWS(_deer), 9, _deer, { _deer_legs[0], _deer_legs[1] }, 11, false, 3, 1, GAME_PX(1), 500, 100, 80 },
	{ "BOAR", "NBWE", { GAME_DARK_BROWN, GAME_BROWN, GAME_WHITE, GAME_BLACK }, GAME_DARK_BROWN,
		32, GAME_ART_ROWS(_boar), 3, _boar, { _boar_legs[0], _boar_legs[1] }, 11, false, 1, 4, GAME_PX(1) + 192, 100, 500, 80 },
	{ "BEAR", "BSKE", { GAME_BROWN, GAME_PEACH, GAME_BLACK, GAME_BLACK }, GAME_BROWN,
		32, GAME_ART_ROWS(_bear), 5, _bear, { _bear_legs[0], _bear_legs[1] }, 12, false, 1, 5, GAME_PX(1) + 100, 100, 500, 100 },
	{ "OWL", "BSYE", { GAME_DARK_BROWN, GAME_PEACH, GAME_YELLOW, GAME_BLACK }, GAME_DARK_BROWN,
		24, GAME_ART_ROWS(_owl), 2, _owl, { _owl_legs[0], _owl_legs[1] }, 10, false, 1, 1, 0, 100, 50, 60 },
	{ "OWLET", "GWYE", { GAME_GREY, GAME_WHITE, GAME_YELLOW, GAME_BLACK }, GAME_GREY,
		16, GAME_ART_ROWS(_owlet), 2, _owlet, { _owlet_legs[0], _owlet_legs[1] }, 6, true, 1, 1, WW_FOLLOW_SPEED, 100, 50, 60 },
	{ "OLD WOLF", "GWKE", { GAME_BLUE, GAME_WHITE, GAME_BLACK, GAME_BLACK }, GAME_BLUE,
		32, GAME_ART_ROWS(_wolf), 4, _wolf, { _wolf_legs[0], _wolf_legs[1] }, 11, false, 1, 1, GAME_PX(1) + 150, 0, 2000, 0 },
};

static WwArt_t _art[WW_ANIMAL_TYPES][2][2];
static GameArt_t _mood_art[WW_MOODS];

static WwAnimal_t _animals[WW_ANIMAL_MAX];
static uint32_t _count = 0;
static WwAnimal_t _wolf_animal;
static bool _wolf_active = false;
static bool _wolf_calm = false;
static WwAnimal_t* _here[WW_HERE_MAX];
static uint32_t _here_n = 0;
static uint32_t _room = 0;
static uint32_t _helped = 0;
static uint32_t _owl_room = WW_NO_ROOM;

static const char* const _plaster_rows[] = {
	"....WWWWWW......",
	"...WWWWWWWW.....",
	"..WWWWRRWWWW....",
	"..WWWWRRWWWW....",
	"..WWRRRRRRWW....",
	"..WWRRRRRRWW....",
	"..WWWWRRWWWW....",
	"..WWWWRRWWWW....",
	"...WWWWWWWW.....",
	"....WWWWWW......",
};
static const char* const _cloud_rows[] = {
	".....GGGG.......",
	"...GGGGGGGG.....",
	"..GGGGGGGGGGG...",
	".GGGGGGGGGGGGG..",
	"GGGGGGGGGGGGGGG.",
	".GGGGGGYYGGGGG..",
	"......YY........",
	".....YYYYY......",
	".......YY.......",
	"......YY........",
	".....Y..........",
};
static const char* const _zed_rows[] = {
	"..........WWWW..",
	"............W...",
	"...........W....",
	"..........WWWW..",
	"WWWWWW..........",
	"....W...........",
	"...W............",
	"..W.............",
	".W..............",
	"WWWWWW..........",
};
static const char* const _query_rows[] = {
	"....YYYYY.......",
	"...YY...YY......",
	"........YY......",
	".......YY.......",
	"......YY........",
	"......YY........",
	"................",
	"......YY........",
	"......YY........",
};
static const char* const _heart_rows[] = {
	".PP.PP..........",
	"PPPPPPP.........",
	"PPPPPPP.........",
	".PPPPP..........",
	"..PPP...........",
	"...P............",
};
static const uint8_t _plaster_colours[] = { GAME_WHITE, GAME_RED };
static const uint8_t _cloud_colours[] = { GAME_GREY, GAME_YELLOW };
static const uint8_t _white[] = { GAME_WHITE };
static const uint8_t _yellow[] = { GAME_YELLOW };
static const uint8_t _cyan[] = { GAME_CYAN };
static const uint8_t _pink[] = { GAME_PINK };
static const uint8_t _all_white[] = { GAME_WHITE, GAME_WHITE, GAME_WHITE, GAME_WHITE };

static void make_art(uint32_t type) {
	const WwAnimalInfo_t* in = &_info[type];
	for (uint32_t f = 0; f < 2; ++f) {
		for (uint32_t left = 0; left < 2; ++left) {
			WwCanvas_t c;
			ww_canvas_clear(&c, in->w, in->body_h + in->legs_h);
			ww_canvas_stamp(&c, in->body, in->body_h, 0, 0);
			ww_canvas_stamp(&c, in->legs[f], in->legs_h, 0, in->body_h);
			if (left) ww_canvas_mirror(&c);
			ww_art_make(&_art[type][f][left], &c, in->key, in->colours);
		}
	}
}

void ww_animal_init(void) {
	for (uint32_t t = 0; t < WW_ANIMAL_TYPES; ++t) make_art(t);
	game_art_make(&_mood_art[WW_MOOD_HAPPY], _heart_rows, GAME_ART_ROWS(_heart_rows), "P", _pink);
	game_art_make(&_mood_art[WW_MOOD_HURT], _plaster_rows, GAME_ART_ROWS(_plaster_rows), "WR", _plaster_colours);
	game_art_make(&_mood_art[WW_MOOD_GRUMPY], _cloud_rows, GAME_ART_ROWS(_cloud_rows), "GY", _cloud_colours);
	game_art_make(&_mood_art[WW_MOOD_ASLEEP], _zed_rows, GAME_ART_ROWS(_zed_rows), "W", _white);
	game_art_make(&_mood_art[WW_MOOD_LOST], _query_rows, GAME_ART_ROWS(_query_rows), "Y", _yellow);
	game_art_make(&_mood_art[WW_MOOD_WAITING], _query_rows, GAME_ART_ROWS(_query_rows), "Y", _cyan);
	_mood_art[WW_MOOD_FOLLOW] = _mood_art[WW_MOOD_HAPPY];
}

uint32_t ww_animal_width(uint32_t type) {
	return _info[type].w;
}

uint32_t ww_animal_height(uint32_t type) {
	return _info[type].body_h + _info[type].legs_h;
}

const char* ww_animal_name(uint32_t type) {
	return _info[type].name;
}

static void set_mood(WwAnimal_t* a, uint32_t mood) {
	const WwAnimalInfo_t* in = &_info[a->type];
	a->mood = mood;
	a->need = mood == WW_MOOD_HURT ? in->heal : in->calm;
}

static WwAnimal_t* add(uint32_t type, uint32_t mood, uint32_t room, int32_t x, int32_t y) {
	if (_count >= WW_ANIMAL_MAX) return NULL;
	WwAnimal_t* a = &_animals[_count++];
	memset(a, 0, sizeof(WwAnimal_t));
	a->type = type;
	a->room = room;
	a->x = a->hx = GAME_PX(x);
	a->y = a->hy = GAME_PX(y);
	a->phase = game_rand();
	a->left = game_rand() & 1;
	set_mood(a, mood);
	if (mood == WW_MOOD_ASLEEP) a->flags |= WW_AF_SLEEPER;
	return a;
}

// Somewhere in the room not too close to anyone already there
static bool free_spot(uint32_t room, int32_t* x, int32_t* y) {
	for (uint32_t tries = 0; tries < 20; ++tries) {
		if (!ww_world_random_spot(room, x, y)) return false;
		bool ok = true;
		for (uint32_t i = 0; i < _count && ok; ++i) {
			const WwAnimal_t* a = &_animals[i];
			if (a->room != room) continue;
			ok = game_abs((a->hx >> GAME_FP) - *x) + game_abs((a->hy >> GAME_FP) - *y) > 72;
		}
		if (ok) return true;
	}
	return false;
}

static WwAnimal_t* add_somewhere(uint32_t type, uint32_t mood, uint32_t room) {
	int32_t x, y;
	if (!free_spot(room, &x, &y)) return NULL;
	return add(type, mood, room, x, y);
}

// A room at least min_dist from the start that has nobody special in it yet
static uint32_t pick_room(const uint8_t* dist, uint32_t min_dist, const bool* taken) {
	uint32_t best = WW_NO_ROOM;
	for (uint32_t tries = 0; tries < 200; ++tries) {
		const uint32_t room = game_rand() % WW_ROOMS;
		if (taken[room] || dist[room] == 0xff) continue;
		if (dist[room] >= min_dist) return room;
		if (best == WW_NO_ROOM || dist[room] > dist[best]) best = room;
	}
	return best;
}

void ww_animal_populate(uint32_t night) {
	_count = 0;
	_helped = 0;
	_wolf_active = false;
	_wolf_calm = false;
	uint8_t dist[WW_ROOMS];
	bool taken[WW_ROOMS] = { false };
	const uint32_t start = ww_world_start();
	for (uint32_t i = 0; i < WW_ROOMS; ++i) dist[i] = ww_world_distance(start, i);
	taken[start] = true;

	// The four who hold pieces of the charm, well away from the start
	static const uint8_t special[4][2] = {
		{ WW_DEER, WW_MOOD_HURT },
		{ WW_BOAR, WW_MOOD_GRUMPY },
		{ WW_BEAR, WW_MOOD_ASLEEP },
		{ WW_OWL, WW_MOOD_WAITING },
	};
	for (uint32_t i = 0; i < 4; ++i) {
		const uint32_t room = pick_room(dist, 4, taken);
		if (room == WW_NO_ROOM) continue;
		taken[room] = true;
		WwAnimal_t* a = add_somewhere(special[i][0], special[i][1], room);
		if (!a) continue;
		if (a->type != WW_BEAR) a->flags |= WW_AF_PIECE;
		if (a->type == WW_OWL) _owl_room = room;
	}
	// The owlet is lost a few rooms away from its mother
	uint8_t owl_dist[WW_ROOMS];
	for (uint32_t i = 0; i < WW_ROOMS; ++i) owl_dist[i] = _owl_room == WW_NO_ROOM ? dist[i] : ww_world_distance(_owl_room, i);
	const uint32_t owlet_room = pick_room(owl_dist, 3, taken);
	if (owlet_room != WW_NO_ROOM) {
		taken[owlet_room] = true;
		add_somewhere(WW_OWLET, WW_MOOD_LOST, owlet_room);
	}

	// Some easy ones to start with
	add_somewhere(WW_BUNNY, WW_MOOD_HAPPY, start);
	add_somewhere(WW_HEDGEHOG, WW_MOOD_HURT, start);

	const uint32_t harder = night > 5 ? 4 : night - 1;
	const uint32_t grumpy = 25 + 6 * harder;
	for (uint32_t room = 0; room < WW_ROOMS; ++room) {
		if (room == start) continue;
		const uint32_t k = game_rand() % 100;
		uint32_t n = k < 20 ? 0 : k < 65 ? 1 : 2;
		if (harder >= 2 && (game_rand() & 1)) ++n;
		for (uint32_t i = 0; i < n; ++i) {
			const uint32_t type = game_rand() % (WW_BADGER + 1);
			const uint32_t r = game_rand() % 100;
			uint32_t mood = WW_MOOD_HAPPY;
			if (r < grumpy) mood = WW_MOOD_GRUMPY;
			else if (r < grumpy + 35) mood = WW_MOOD_HURT;
			else if (r < grumpy + 50 && type >= WW_HEDGEHOG) mood = WW_MOOD_ASLEEP;
			add_somewhere(type, mood, room);
		}
	}
}

bool ww_animal_bear(uint32_t* room, int32_t* x, int32_t* y) {
	for (uint32_t i = 0; i < _count; ++i) {
		const WwAnimal_t* a = &_animals[i];
		if (a->type != WW_BEAR) continue;
		*room = a->room;
		*x = a->hx >> GAME_FP;
		*y = a->hy >> GAME_FP;
		return true;
	}
	return false;
}

static void gather_here(void) {
	_here_n = 0;
	for (uint32_t i = 0; i < _count && _here_n < WW_HERE_MAX - 1; ++i) {
		if (_animals[i].room == _room) _here[_here_n++] = &_animals[i];
	}
	if (_wolf_active) _here[_here_n++] = &_wolf_animal;
}

void ww_animal_enter(uint32_t room, int32_t x, int32_t y) {
	_room = room;
	for (uint32_t i = 0; i < _count; ++i) {
		WwAnimal_t* a = &_animals[i];
		if (a->mood != WW_MOOD_FOLLOW) continue;
		a->room = room;
		a->x = GAME_PX(x);
		a->y = GAME_PX(y);
	}
	gather_here();
}

uint32_t ww_animal_leave(void) {
	uint32_t crept = 0;
	for (uint32_t i = 0; i < _here_n; ++i) {
		WwAnimal_t* a = _here[i];
		if (a == &_wolf_animal || a->mood == WW_MOOD_FOLLOW) continue;
		if (a->mood == WW_MOOD_ASLEEP && (a->flags & WW_AF_NEAR) && !(a->flags & WW_AF_CREPT)) {
			a->flags |= WW_AF_CREPT;
			++crept;
		}
		// Those woken up drop off again once she has gone
		if ((a->flags & WW_AF_SLEEPER) && a->mood == WW_MOOD_GRUMPY) set_mood(a, WW_MOOD_ASLEEP);
		a->x = a->hx;
		a->y = a->hy;
		a->flags &= ~(WW_AF_NEAR | WW_AF_SPOKE);
		a->stun = a->flash = a->joy = 0;
		a->moving = false;
	}
	ww_animal_wolf_remove();
	return crept;
}

void ww_animal_reset_room(void) {
	for (uint32_t i = 0; i < _here_n; ++i) {
		WwAnimal_t* a = _here[i];
		if (a == &_wolf_animal || a->mood == WW_MOOD_FOLLOW) continue;
		a->x = a->hx;
		a->y = a->hy;
		a->stun = WW_STUN_FRAMES * 2;
	}
	ww_animal_wolf_remove();
}

static void say(const WwAnimal_t* a, const char* s, uint32_t colour) {
	ww_hud_say(a->x >> GAME_FP, (a->y >> GAME_FP) + WW_HUD_H - ww_animal_height(a->type) - 14, s, colour);
}

static void points(const WwAnimal_t* a, uint32_t p) {
	if (!p) return;
	ww_hud_add_score(p);
	game_fx_popup(a->x - GAME_PX(8), (a->y >> GAME_FP) + WW_HUD_H - ww_animal_height(a->type) - 26, p);
}

static void hearts(const WwAnimal_t* a, uint32_t n) {
	ww_world_burst(a->x >> GAME_FP, (a->y >> GAME_FP) - (ww_animal_height(a->type) >> 1), GAME_PINK, n, GAME_PX(2));
}

static int32_t dist_to_witch(const WwAnimal_t* a) {
	return game_abs((a->x >> GAME_FP) - ww_witch_x()) + game_abs((a->y >> GAME_FP) - ww_witch_y());
}

// Heads one way or another for a while, or stops for a rest
static void wander(WwAnimal_t* a, int32_t speed) {
	if (a->timer) {
		--a->timer;
		return;
	}
	a->timer = 40 + game_rand() % 80;
	const uint32_t d = game_rand() % 12;
	if (d >= 8) {
		a->vx = a->vy = 0;
		return;
	}
	int32_t dx, dy;
	ww_witch_facing_step(d, &dx, &dy);
	const int32_t s = dx && dy ? speed * WW_DIAG >> GAME_FP : speed;
	a->vx = dx * s;
	a->vy = dy * s;
}

static void head_for(WwAnimal_t* a, int32_t x, int32_t y, int32_t speed) {
	const int32_t dx = x - (a->x >> GAME_FP), dy = y - (a->y >> GAME_FP);
	const int32_t sx = game_abs(dx) > 3 ? game_sign(dx) : 0;
	const int32_t sy = game_abs(dy) > 3 ? game_sign(dy) : 0;
	const int32_t s = sx && sy ? speed * WW_DIAG >> GAME_FP : speed;
	a->vx = sx * s;
	a->vy = sy * s;
}

static void move(WwAnimal_t* a, uint32_t frame, bool solid) {
	const WwAnimalInfo_t* in = &_info[a->type];
	int32_t vx = a->vx, vy = a->vy;
	// Hoppers only move while in the air
	if (in->hop && ((a->anim + a->phase) % WW_HOP) >= WW_HOP / 2) vx = vy = 0;
	a->moving = a->vx || a->vy;
	if (a->moving) {
		++a->anim;
		if (a->vx) a->left = a->vx < 0;
	}
	else {
		// Finish the hop
		if (in->hop && ((a->anim + a->phase) % WW_HOP) < WW_HOP / 2) ++a->anim;
	}
	if (!vx && !vy) return;
	if (!solid) {
		a->x += vx;
		a->y += vy;
	}
	else if (ww_world_move(&a->x, &a->y, vx, vy, in->hw, 8)) {
		++a->stuck;
	}
	else {
		a->stuck = 0;
	}
	// They stay in their own room
	const int32_t m = GAME_PX(in->hw + 4);
	a->x = game_clamp(a->x, m, GAME_PX(WW_ROOM_W) - m);
	a->y = game_clamp(a->y, GAME_PX(14), GAME_PX(WW_ROOM_H - 4));
}

static WwAnimal_t* mother_here(void) {
	for (uint32_t i = 0; i < _here_n; ++i) {
		if (_here[i]->mood == WW_MOOD_WAITING) return _here[i];
	}
	return NULL;
}

static void reunite(WwAnimal_t* owlet, WwAnimal_t* owl) {
	owlet->mood = owl->mood = WW_MOOD_HAPPY;
	owlet->flags |= WW_AF_DONE;
	owl->flags |= WW_AF_DONE;
	owlet->room = owl->room;
	owlet->hx = owl->hx + GAME_PX(20);
	owlet->hy = owl->hy;
	owlet->joy = owl->joy = WW_JOY_FRAMES;
	owlet->vx = owlet->vy = 0;
	hearts(owl, 24);
	say(owl, "MY BABY! THANK YOU!", GAME_PINK);
	points(owl, 500);
	++_helped;
	if (owl->flags & WW_AF_PIECE) ww_items_award_piece(WW_PIECE_HELP, owl->x >> GAME_FP, (owl->y >> GAME_FP) - 20);
}

// Says something the first time she comes near
static void greet(WwAnimal_t* a) {
	if ((a->flags & WW_AF_SPOKE) || dist_to_witch(a) > WW_SPEAK) return;
	a->flags |= WW_AF_SPOKE;
	switch (a->mood) {
		case WW_MOOD_HURT: say(a, a->type == WW_DEER ? "OH, MY POOR LEG..." : "OUCH!", GAME_WHITE); break;
		case WW_MOOD_GRUMPY: say(a, a == &_wolf_animal ? "GRRRRR!" : a->type == WW_BOAR ? "SNORT! GO AWAY!" : "GRR!", GAME_RED); break;
		case WW_MOOD_LOST: say(a, "I'M LOST!", GAME_YELLOW); break;
		case WW_MOOD_WAITING: say(a, "HAVE YOU SEEN MY OWLET?", GAME_CYAN); break;
		default: break;
	}
}

static void update_one(WwAnimal_t* a, uint32_t frame) {
	const WwAnimalInfo_t* in = &_info[a->type];
	const bool wolf = a == &_wolf_animal;
	if (a->flash) --a->flash;
	if (a->joy) --a->joy;
	if (ww_witch_alive()) greet(a);

	if (a->stun) {
		--a->stun;
		// Pushed back a little, slowing down
		ww_world_move(&a->x, &a->y, a->vx, a->vy, in->hw, 8);
		a->vx = a->vx * 7 / 8;
		a->vy = a->vy * 7 / 8;
		a->moving = false;
		return;
	}

	switch (a->mood) {
		case WW_MOOD_HAPPY:
			if (a->type == WW_OWL || ((a->flags & WW_AF_DONE) && a->type == WW_OWLET)) {
				a->vx = a->vy = 0;
			}
			else {
				wander(a, WW_WANDER);
			}
			if (a->joy && (frame & 15) == 0) hearts(a, 1);
			move(a, frame, true);
			break;
		case WW_MOOD_HURT:
			a->vx = a->vy = 0;
			a->moving = false;
			// A tear now and then
			if (((frame + a->phase) & 63) == 0) {
				const int32_t ex = (a->x >> GAME_FP) + (a->left ? -1 : 1) * (int32_t)(in->w >> 2);
				game_fx_explode(GAME_PX(ex), GAME_PX((a->y >> GAME_FP) + WW_HUD_H - ww_animal_height(a->type) + 8), GAME_CYAN, 1, GAME_PX(1) >> 2);
			}
			break;
		case WW_MOOD_ASLEEP: {
			a->vx = a->vy = 0;
			a->moving = false;
			if (!ww_witch_alive()) break;
			const int32_t d = dist_to_witch(a);
			if (d < in->wake + 70) a->flags |= WW_AF_NEAR;
			if (d < in->wake && !ww_witch_quiet()) {
				set_mood(a, WW_MOOD_GRUMPY);
				a->flags |= WW_AF_SPOKE;
				say(a, a->type == WW_BEAR ? "ROAR! WHO WOKE ME?" : "HEY! I WAS ASLEEP!", GAME_RED);
				a->stun = WW_STUN_FRAMES;
			}
			break;
		}
		case WW_MOOD_GRUMPY: {
			const int32_t d = dist_to_witch(a);
			const bool chase = ww_witch_alive() && (wolf || a->type == WW_BEAR || (!ww_witch_hushed() && d < WW_NOTICE));
			const int32_t speed = wolf || a->type == WW_BEAR ? in->speed : in->speed * (8 + (int32_t)a->phase % 3) / 8;
			// Stuck behind something, so tries another way for a bit
			if (a->stuck >= WW_STUCK) {
				a->stuck = 0;
				a->timer = 0;
				a->detour = 40 + game_rand() % 60;
			}
			if (a->detour) {
				--a->detour;
				wander(a, speed);
			}
			else if (chase) {
				head_for(a, ww_witch_x(), ww_witch_y(), speed);
			}
			else {
				wander(a, WW_WANDER);
			}
			move(a, frame, true);
			if (ww_witch_alive() && ww_witch_overlaps((a->x >> GAME_FP) - in->hw, (a->y >> GAME_FP) - ww_animal_height(a->type) * 2 / 3, (a->x >> GAME_FP) + in->hw, a->y >> GAME_FP)) {
				if (ww_witch_hurt(a->x >> GAME_FP, a->y >> GAME_FP)) {
					// Bounces off
					a->vx = -a->vx * 2;
					a->vy = -a->vy * 2;
					a->stun = WW_STUN_FRAMES;
				}
			}
			break;
		}
		case WW_MOOD_LOST:
			a->vx = a->vy = 0;
			move(a, frame, true);
			if (ww_witch_alive() && ww_witch_overlaps((a->x >> GAME_FP) - 12, (a->y >> GAME_FP) - 20, (a->x >> GAME_FP) + 12, a->y >> GAME_FP)) {
				a->mood = WW_MOOD_FOLLOW;
				say(a, "TAKE ME TO MUMMY!", GAME_YELLOW);
				hearts(a, 8);
			}
			break;
		case WW_MOOD_FOLLOW: {
			// Flutters along behind her, over anything in the way
			int32_t fx, fy;
			ww_witch_facing_step(ww_witch_facing(), &fx, &fy);
			const int32_t tx = ww_witch_x() - fx * WW_FOLLOW_GAP, ty = ww_witch_y() - fy * WW_FOLLOW_GAP + 2;
			if (game_abs(tx - (a->x >> GAME_FP)) + game_abs(ty - (a->y >> GAME_FP)) > 6) head_for(a, tx, ty, in->speed);
			else a->vx = a->vy = 0;
			move(a, frame, false);
			WwAnimal_t* owl = mother_here();
			if (owl && game_abs((owl->x - a->x) >> GAME_FP) + game_abs((owl->y - a->y) >> GAME_FP) < WW_REUNITE + 30) reunite(a, owl);
			break;
		}
		case WW_MOOD_WAITING:
			a->vx = a->vy = 0;
			// Flaps about now and then
			a->moving = ((frame + a->phase * 4) & 127) < 24;
			if (a->moving) ++a->anim;
			break;
	}
}

void ww_animal_update(uint32_t frame) {
	for (uint32_t i = 0; i < _here_n; ++i) update_one(_here[i], frame);
}

static void calmed(WwAnimal_t* a) {
	const WwAnimalInfo_t* in = &_info[a->type];
	a->mood = WW_MOOD_HAPPY;
	a->flags |= WW_AF_DONE;
	a->joy = WW_JOY_FRAMES;
	a->timer = 0;
	hearts(a, 16);
	if (a == &_wolf_animal) {
		_wolf_calm = true;
		say(a, "THE MOON... SO PRETTY...", GAME_CYAN);
		points(a, in->calm_score);
		return;
	}
	say(a, a->type == WW_BOAR ? "SORRY... THANK YOU" : a->type == WW_BEAR ? "YAWN... SORRY" : "AWW, SORRY", GAME_PINK);
	points(a, in->calm_score);
	++_helped;
	if (a->flags & WW_AF_PIECE) ww_items_award_piece(WW_PIECE_CALM, a->x >> GAME_FP, (a->y >> GAME_FP) - 20);
}

static void healed(WwAnimal_t* a) {
	const WwAnimalInfo_t* in = &_info[a->type];
	a->mood = WW_MOOD_HAPPY;
	a->flags |= WW_AF_DONE;
	a->joy = WW_JOY_FRAMES;
	a->timer = 0;
	hearts(a, 16);
	say(a, "THANK YOU!", GAME_PINK);
	points(a, in->heal_score);
	++_helped;
	if (a->flags & WW_AF_PIECE) ww_items_award_piece(WW_PIECE_HEAL, a->x >> GAME_FP, (a->y >> GAME_FP) - 20);
}

bool ww_animal_spell(int32_t x, int32_t y) {
	for (uint32_t i = 0; i < _here_n; ++i) {
		WwAnimal_t* a = _here[i];
		const WwAnimalInfo_t* in = &_info[a->type];
		const int32_t ax = a->x >> GAME_FP, ay = a->y >> GAME_FP;
		const int32_t half = (in->w >> 1) - 2;
		if (x < ax - half || x > ax + half || y < ay - (int32_t)ww_animal_height(a->type) || y > ay + 2) continue;
		a->flash = 8;
		switch (a->mood) {
			case WW_MOOD_HURT:
				if (--a->need == 0) healed(a);
				else {
					say(a, "A LITTLE BETTER...", GAME_WHITE);
					hearts(a, 4);
				}
				break;
			case WW_MOOD_GRUMPY:
				if (a == &_wolf_animal && !ww_hud_charm_whole()) {
					// Only the whole moon charm will calm him, but it does make him back off
					say(a, "YELP!", GAME_WHITE);
					a->stun = WW_WOLF_STUN;
					a->vx = game_sign(ax - ww_witch_x()) * GAME_PX(3);
					a->vy = game_sign(ay - ww_witch_y()) * GAME_PX(3);
					break;
				}
				if (a->need) --a->need;
				if (a->need == 0) calmed(a);
				else {
					say(a, "HMPH!", GAME_WHITE);
					a->stun = WW_STUN_FRAMES;
					a->vx = game_sign(ax - ww_witch_x()) * GAME_PX(2);
					a->vy = game_sign(ay - ww_witch_y()) * GAME_PX(2);
				}
				break;
			case WW_MOOD_ASLEEP:
				set_mood(a, WW_MOOD_GRUMPY);
				a->flags |= WW_AF_SPOKE;
				say(a, a->type == WW_BEAR ? "ROAR! WHO WOKE ME?" : "HEY! I WAS ASLEEP!", GAME_RED);
				a->stun = WW_STUN_FRAMES;
				break;
			case WW_MOOD_HAPPY:
				if (!(a->flags & (WW_AF_FRIEND | WW_AF_DONE))) {
					a->flags |= WW_AF_FRIEND;
					points(a, 10);
				}
				a->joy = WW_JOY_FRAMES / 2;
				hearts(a, 6);
				break;
			default:
				hearts(a, 6);
				break;
		}
		return true;
	}
	return false;
}

void ww_animal_wolf_spawn(int32_t x, int32_t y, uint32_t night) {
	WwAnimal_t* a = &_wolf_animal;
	memset(a, 0, sizeof(WwAnimal_t));
	a->type = WW_WOLF;
	a->room = _room;
	a->x = a->hx = GAME_PX(x);
	a->y = a->hy = GAME_PX(y);
	a->left = x > ww_witch_x();
	a->mood = WW_MOOD_GRUMPY;
	a->need = 1;
	a->stun = WW_STUN_FRAMES;
	_wolf_active = true;
	_wolf_calm = false;
	gather_here();
	game_fx_implode(GAME_PX(x), GAME_PX(y + WW_HUD_H - 10), GAME_BLUE, 24, 30);
}

bool ww_animal_wolf_here(void) {
	return _wolf_active;
}

void ww_animal_wolf_remove(void) {
	if (!_wolf_active) return;
	_wolf_active = false;
	gather_here();
}

bool ww_animal_wolf_calmed(void) {
	return _wolf_active && _wolf_calm;
}

uint32_t ww_animal_helped(void) {
	return _helped;
}

uint32_t ww_animal_owl_room(void) {
	return _owl_room;
}

bool ww_animal_following(void) {
	for (uint32_t i = 0; i < _here_n; ++i) {
		if (_here[i]->mood == WW_MOOD_FOLLOW) return true;
	}
	return false;
}

uint32_t ww_animal_here(void) {
	return _here_n;
}

const WwAnimal_t* ww_animal_get(uint32_t i) {
	return i < _here_n ? _here[i] : NULL;
}

void ww_animal_draw_at(uint32_t type, uint32_t mood, int32_t sx, int32_t sy, uint32_t frame, bool left) {
	const WwAnimalInfo_t* in = &_info[type];
	uint8_t colours[4];
	for (uint32_t i = 0; i < 4; ++i) colours[i] = in->colours[i];
	// Eyes shut while asleep
	if (mood == WW_MOOD_ASLEEP) colours[strlen(in->key) - 1] = in->shut;
	ww_art_draw(&_art[type][frame & 1][left ? 1 : 0], sx, sy, colours);
}

static void draw_one(const WwAnimal_t* a, uint32_t frame) {
	const WwAnimalInfo_t* in = &_info[a->type];
	const uint32_t h = ww_animal_height(a->type);
	int32_t sx = (a->x >> GAME_FP) - (in->w >> 1);
	int32_t sy = (a->y >> GAME_FP) - h + WW_HUD_H;
	uint32_t f = 0;
	if (in->hop) {
		// Up in the air for the first half of each hop
		const uint32_t t = (a->anim + a->phase) % WW_HOP;
		if (t < WW_HOP / 2 && (a->moving || t)) {
			sy -= game_sin(t * (128 / (WW_HOP / 2))) * WW_HOP_H / 127;
			f = 1;
		}
	}
	else if (a->moving) {
		f = (a->anim >> 3) & 1;
	}
	// Shivers while hurt
	if (a->mood == WW_MOOD_HURT && ((frame + a->phase) & 8)) ++sx;
	if (a->flash) {
		ww_art_draw(&_art[a->type][f][a->left ? 1 : 0], sx, sy, _all_white);
		return;
	}
	ww_animal_draw_at(a->type, a->mood, sx, sy, f, a->left);
}

void __not_in_flash_func(ww_animal_draw)(uint32_t frame, int32_t split_y, bool front) {
	// Nearest the front first, as the first sprite drawn wins
	const WwAnimal_t* order[WW_HERE_MAX];
	uint32_t n = 0;
	for (uint32_t i = 0; i < _here_n; ++i) {
		const WwAnimal_t* a = _here[i];
		if (((a->y >> GAME_FP) >= split_y) != front) continue;
		uint32_t j = n++;
		while (j > 0 && order[j - 1]->y < a->y) {
			order[j] = order[j - 1];
			--j;
		}
		order[j] = a;
	}
	for (uint32_t i = 0; i < n; ++i) draw_one(order[i], frame);
}

void ww_animal_draw_mood(uint32_t mood, int32_t cx, int32_t sy, uint32_t frame) {
	if (mood == WW_MOOD_ASLEEP) {
		// Drifts up and away
		const uint32_t t = frame & 63;
		if (t >= 48) return;
		ww_art_draw_small(&_mood_art[mood], cx - 2 + (t >> 3), sy - 12 - (t >> 2), NULL);
		return;
	}
	const int32_t bob = game_sin(frame << 2) * 2 / 127;
	ww_art_draw_small(&_mood_art[mood], cx - 7, sy - 12 + bob, NULL);
}

void __not_in_flash_func(ww_animal_draw_moods)(uint32_t frame) {
	for (uint32_t i = 0; i < _here_n; ++i) {
		const WwAnimal_t* a = _here[i];
		uint32_t mood = a->mood;
		if (mood == WW_MOOD_HAPPY && !a->joy) continue;
		if (mood == WW_MOOD_FOLLOW && ((frame >> 5) & 3)) continue;
		if (a == &_wolf_animal && mood == WW_MOOD_GRUMPY && a->stun) continue;
		const int32_t cx = a->x >> GAME_FP;
		const int32_t top = (a->y >> GAME_FP) - ww_animal_height(a->type) + WW_HUD_H;
		ww_animal_draw_mood(mood, cx, top, frame + a->phase);
	}
}
