#include "mm_attract.h"
#include "hstx_dvi_sprite.h"
#include "mm_input.h"
#include "mm_game.h"
#include "mm_camera.h"
#include "mm_tiles.h"
#include "mm_hud.h"
#include "mm_art.h"
#include "mm_gfx.h"
#include "game_render.h"
#include "game_pallet.h"
#include "game_fx.h"
#include "game_hiscore.h"
#include "game_initials.h"
#include "game_util.h"
#include <stdio.h>
#include <string.h>

#define MM_ATTRACT_TITLE "MOONLIT MANOR"
#define MM_ATTRACT_TITLE_LEN 13
#define MM_ATTRACT_TITLE_FRAMES 1200
#define MM_ATTRACT_ROLL_FRAMES 900
#define MM_ATTRACT_HISCORE_FRAMES 600
#define MM_ATTRACT_HIGHLIGHT_FRAMES 900
// Give up waiting for initials after 30 seconds
#define MM_ATTRACT_ENTRY_FRAMES (60 * 30)
#define MM_ATTRACT_MATERIALISE 40
#define MM_ATTRACT_ROLL_START 30
#define MM_ATTRACT_ROLL_GAP 50
#define MM_ATTRACT_ROLL_Y 58
#define MM_ATTRACT_ROLL_H 54
#define MM_ATTRACT_STARS 48
#define MM_ATTRACT_TOUR_SPEED (GAME_PX(3) / 2)
// Where the floor goes in the view, so it is just above the band at the bottom
#define MM_ATTRACT_TOUR_FLOOR 336

typedef struct {
	uint8_t what;
	const char* line1;
	const char* line2;
} MmRoll_t;

enum { MM_ROLL_GHOST, MM_ROLL_BAT, MM_ROLL_SPIDER, MM_ROLL_PUMPKIN, MM_ROLL_KITTEN, MM_ROLL_KEY, MM_ROLL_FEATHER };

static const MmRoll_t _roll[] = {
	{ MM_ROLL_GHOST, "GHOSTS", "CHEER THEM WITH A SPELL, THEN STAND ON" },
	{ MM_ROLL_BAT, "BATS", "A SPELL SENDS THEM OFF TO SLEEP" },
	{ MM_ROLL_SPIDER, "SPIDERS", "A SPELL SENDS THEM BACK UP THE THREAD" },
	{ MM_ROLL_PUMPKIN, "PUMPKINS", "A SPELL LIGHTS THEM UP, THEN HOP ON" },
	{ MM_ROLL_KITTEN, "KITTENS", "FIVE ARE LOST IN THE HOUSE. FIND THEM!" },
	{ MM_ROLL_KEY, "LEVERS CANDLES AND KEYS", "SPELLS PULL LEVERS AND LIGHT CANDLES" },
	{ MM_ROLL_FEATHER, "THE MAGIC FEATHER", "HOLD JUMP TO FLOAT OVER BIG GAPS" },
};
#define MM_ATTRACT_ROLL_COUNT (sizeof(_roll) / sizeof(_roll[0]))

// Places for the title to show off: house pixels across, and the row of the
// floor to stand the view on, so it pans along each floor in turn
static const int16_t _tour[][2] = {
	{ 320, 35 }, { 900, 35 }, { 1500, 35 },
	{ 1100, 24 }, { 1800, 24 }, { 2500, 24 },
	{ 2500, 13 }, { 1500, 13 }, { 800, 13 },
	{ 1600, 24 }, { 2300, 35 }, { 1700, 35 },
};
#define MM_ATTRACT_TOUR_STOPS (sizeof(_tour) / sizeof(_tour[0]))

// Text has to outlive the screen that shows it, as the renderer keeps
// drawing the last frame until the next screen has been set up
static GameText_t _text[20];
static GameText_t _roll_text[MM_ATTRACT_ROLL_COUNT][2];
static hstx_dvi_pixel_t _shade[1];

typedef struct {
	uint16_t x;
	uint16_t y;
	uint8_t c;
} MmStar_t;

static MmStar_t _stars[MM_ATTRACT_STARS];
static bool _stars_made = false;
static Tile8x8p2_t _tile_star = {{ 0b10000000 }};

static void attract_init(void) {
	game_fx_set_view(NULL);
	game_fx_init();
	_shade[0] = HSTX_DVI_PIXEL_RGB(0, 0, 32);
	if (!_stars_made) {
		for (uint32_t i = 0; i < MM_ATTRACT_STARS; ++i) {
			_stars[i].x = game_rand() % GAME_SCREEN_W;
			_stars[i].y = game_rand() % GAME_SCREEN_H;
			_stars[i].c = game_rand();
		}
		_stars_made = true;
	}
	// Forget about any buttons still held down from before
	game_input_pressed();
}

// Waits for the next frame. Returns true if the player wants to play.
static bool attract_frame(void) {
	hstx_dvi_sprite_wait_for_frame();
	game_fx_update();
	return is_mm_input_play(game_input_pressed());
}

// Stars are ordinary sprites, which don't draw over the pictures, so they go first
static void draw_stars(uint32_t frame) {
	for (uint32_t i = 0; i < MM_ATTRACT_STARS; ++i) {
		const MmStar_t* s = &_stars[i];
		const uint32_t c = ((s->c + (frame >> 3)) & 15) == 0 ? GAME_WHITE : (s->c & 1) ? GAME_GREY : GAME_NAVY;
		game_spr_add(s->x, s->y, 8, 1, &_tile_star, game_pallet(c), sprite_renderer_sprite_8x8_p1);
	}
}

// Hazel walking along with the kittens and a friendly ghost behind her
static void draw_parade(uint32_t frame, int32_t y) {
	const int32_t span = GAME_SCREEN_W + 480;
	const int32_t x = (int32_t)((frame * 3 / 2) % span) - 48;
	static const uint8_t steps[4] = { MM_HAZEL_WALK1, MM_HAZEL_STAND, MM_HAZEL_WALK2, MM_HAZEL_STAND };
	mm_gfx_draw(&mm_art_hazel[steps[(frame >> 3) & 3]], mm_pal_hazel, x, y - 64, 2, 0);
	for (uint32_t i = 0; i < MM_KITTEN_COATS; ++i) {
		const int32_t hop = ((frame >> 3) + i) & 1 ? 4 : 0;
		mm_gfx_draw(&mm_art_kitten[0], mm_pal_kitten[i], x - 44 - (int32_t)i * 40, y - 32 - hop, 2, 0);
	}
	const int32_t bob = game_sin(frame << 2) * 6 / 127;
	mm_gfx_draw(&mm_art_ghost[MM_MOOD_HAPPY][(frame >> 4) & 1], mm_pal_ghost[MM_MOOD_HAPPY], x - 280, y - 76 + bob, 2, 0);
}

static void draw_roll_picture(uint32_t what, int32_t x, int32_t y, uint32_t frame) {
	const uint32_t f = (frame >> 4) & 1;
	// Each shown grumpy, then cheered up
	const uint32_t m = (frame >> 7) & 1;
	switch (what) {
	case MM_ROLL_GHOST:
		mm_gfx_draw(&mm_art_ghost[m][f], mm_pal_ghost[m], x - 24, y - 46, 2, m ? 0 : MM_GFX_GLASS);
		break;
	case MM_ROLL_BAT: mm_gfx_draw(&mm_art_bat[m][(frame >> 3) & 1], mm_pal_bat, x - 24, y - 32, 2, 0); break;
	case MM_ROLL_SPIDER: mm_gfx_draw(&mm_art_spider[m][f], mm_pal_spider, x - 16, y - 28, 2, 0); break;
	case MM_ROLL_PUMPKIN: mm_gfx_draw(&mm_art_pumpkin[m][f], mm_pal_pumpkin[m], x - 16, y - 32, 2, 0); break;
	case MM_ROLL_KITTEN:
		mm_gfx_draw(&mm_art_kitten[((frame >> 5) % 5) == 0], mm_pal_kitten[(frame >> 6) % MM_KITTEN_COATS], x - 16, y - 32, 2, 0);
		break;
	case MM_ROLL_KEY: mm_gfx_draw(&mm_art_key, mm_pal_key, x - 16, y - 20, 2, 0); break;
	case MM_ROLL_FEATHER: mm_gfx_draw(&mm_art_feather, mm_pal_feather, x - 16, y - 32, 2, 0); break;
	default: break;
	}
}

bool mm_attract_title(void) {
	attract_init();
	for (uint32_t i = 0; i < MM_ATTRACT_TITLE_LEN; ++i) {
		const char s[2] = { MM_ATTRACT_TITLE[i], 0 };
		game_text_set(&_text[i], s, 4);
	}
	game_text_set(&_text[13], "HAZEL AND THE LOST KITTENS", 2);
	game_text_set(&_text[14], "PRESS JUMP TO PLAY", 2);
	game_text_set(&_text[15], "LEFT AND RIGHT TO MOVE   JUMP   CAST", 1);
	game_text_set(&_text[16], "UP+CAST CASTS UP  DOWN+JUMP DROPS DOWN", 1);

	mm_game_tour_start();
	uint32_t stop = 0;
	const int32_t tx = (GAME_SCREEN_W - MM_ATTRACT_TITLE_LEN * 32) >> 1;
	for (uint32_t frame = 0; frame < MM_ATTRACT_TITLE_FRAMES; ++frame) {
		hstx_dvi_sprite_wait_for_frame();
		if (is_mm_input_play(game_input_pressed())) return true;
		// Drifts round the house, a few seconds at each place
		if (frame % 100 == 99) stop = (stop + 1) % MM_ATTRACT_TOUR_STOPS;
		const int32_t floor = _tour[stop][1] * MM_TILE;
		mm_camera_glide(_tour[stop][0], floor - MM_ATTRACT_TOUR_FLOOR + MM_VIEW_H / 2, MM_ATTRACT_TOUR_SPEED * 4);

		game_spr_begin();
		mm_game_tour_frame(frame);
		mm_gfx_rect(0, 0, GAME_SCREEN_W, 120, _shade);
		mm_gfx_rect(0, 400, GAME_SCREEN_W, 80, _shade);
		for (uint32_t i = 0; i < MM_ATTRACT_TITLE_LEN; ++i) {
			const int32_t bob = game_sin((frame << 2) + i * 20) * 5 / 127;
			game_spr_text(&_text[i], tx + i * 32, 24 + bob, game_pallet_rainbow(i + (frame >> 3)));
		}
		game_spr_text_centre(&_text[13], 76, game_pallet(GAME_WHITE));
		mm_gfx_draw(&mm_art_moon, mm_pal_moon, 8, 4, 2, 0);
		// Hazel four times her size
		const uint32_t cheer = (frame >> 5) & 1 ? MM_HAZEL_CHEER : MM_HAZEL_FRONT;
		mm_gfx_draw(&mm_art_hazel[cheer], mm_pal_hazel, GAME_SCREEN_W - 104, 270, 4, 0);
		if ((frame >> 5) & 1) game_spr_text_centre(&_text[14], 412, game_pallet(GAME_YELLOW));
		game_spr_text_centre(&_text[15], 444, game_pallet(GAME_CYAN));
		game_spr_text_centre(&_text[16], 458, game_pallet(GAME_CYAN));
		game_spr_end();
	}
	return false;
}

bool mm_attract_roll(void) {
	attract_init();
	game_text_set(&_text[0], "WHO LIVES IN THE MANOR", 2);
	game_text_set(&_text[1], "PRESS JUMP TO PLAY", 2);
	for (uint32_t i = 0; i < MM_ATTRACT_ROLL_COUNT; ++i) {
		game_text_set(&_roll_text[i][0], _roll[i].line1, 1);
		game_text_set(&_roll_text[i][1], _roll[i].line2, 1);
	}
	const int32_t ax = 90;
	for (uint32_t frame = 0; frame < MM_ATTRACT_ROLL_FRAMES; ++frame) {
		if (attract_frame()) return true;
		for (uint32_t i = 0; i < MM_ATTRACT_ROLL_COUNT; ++i) {
			if (frame == MM_ATTRACT_ROLL_START + i * MM_ATTRACT_ROLL_GAP) {
				const int32_t y = MM_ATTRACT_ROLL_Y + i * MM_ATTRACT_ROLL_H + 30;
				game_fx_implode(GAME_PX(ax), GAME_PX(y), GAME_COLOUR_RANDOM, 32, MM_ATTRACT_MATERIALISE);
			}
		}
		game_spr_begin();
		draw_stars(frame);
		game_spr_text_centre(&_text[0], 30, game_pallet_rainbow(frame >> 3));
		for (uint32_t i = 0; i < MM_ATTRACT_ROLL_COUNT; ++i) {
			if (frame < MM_ATTRACT_ROLL_START + i * MM_ATTRACT_ROLL_GAP + MM_ATTRACT_MATERIALISE) continue;
			const int32_t y = MM_ATTRACT_ROLL_Y + i * MM_ATTRACT_ROLL_H;
			draw_roll_picture(_roll[i].what, ax, y + 52, frame + i * 40);
			game_spr_text(&_roll_text[i][0], ax + 60, y + 24, game_pallet(i == MM_ROLL_KITTEN ? GAME_PINK : GAME_YELLOW));
			game_spr_text(&_roll_text[i][1], ax + 60, y + 38, game_pallet(GAME_WHITE));
		}
		if ((frame >> 5) & 1) game_spr_text_centre(&_text[1], 450, game_pallet(GAME_YELLOW));
		game_fx_draw_particles();
		game_spr_end();
	}
	return false;
}

static uint32_t rank_colour(const uint32_t rank) {
	if (rank == 0) return GAME_YELLOW;
	if (rank < 3) return GAME_WHITE;
	if (rank < 6) return GAME_PINK;
	return GAME_CYAN;
}

bool mm_attract_hiscores(int32_t highlight) {
	attract_init();
	char rows[GAME_HISCORE_COUNT][GAME_TEXT_MAX];
	game_text_set(&_text[0], "BRAVEST WITCHES", 3);
	for (uint32_t i = 0; i < GAME_HISCORE_COUNT; ++i) {
		const GameHiScore_t *e = game_hiscore_get(i);
		snprintf(rows[i], GAME_TEXT_MAX, "%-4s  %07lu  %s", game_hiscore_rank_name(i), (unsigned long)e->score, e->name);
	}
	game_text_set(&_text[11], "PRESS JUMP TO PLAY", 2);

	const uint32_t frames = highlight >= 0 ? MM_ATTRACT_HIGHLIGHT_FRAMES : MM_ATTRACT_HISCORE_FRAMES;
	for (uint32_t frame = 0; frame < frames; ++frame) {
		if (attract_frame()) return true;

		game_spr_begin();
		draw_stars(frame);
		mm_hud_draw(0, frame, false);
		game_spr_text_centre(&_text[0], 56, game_pallet_rainbow(frame >> 3));
		// Typed out a line at a time
		int32_t n = (int32_t)frame - 30;
		for (uint32_t i = 0; i < GAME_HISCORE_COUNT; ++i) {
			game_text_set_n(&_text[1 + i], rows[i], n < 0 ? 0 : n, 1);
			n -= strlen(rows[i]);
			const uint32_t c = (int32_t)i == highlight && ((frame >> 3) & 1) ? GAME_RED : rank_colour(i);
			game_spr_text_centre(&_text[1 + i], 104 + i * 22, game_pallet(c));
		}
		if (frame > 260 && ((frame >> 5) & 1)) game_spr_text_centre(&_text[11], 340, game_pallet(GAME_YELLOW));
		draw_parade(frame, 460);
		game_spr_end();
	}
	return false;
}

bool mm_attract_game_over(void) {
	const uint32_t score = mm_hud_score();
	const int32_t rank = game_hiscore_rank(score);
	if (rank < 0) return false;

	attract_init();
	char t[GAME_TEXT_MAX];
	game_text_set(&_text[0], "NEW HIGH SCORE", 3);
	snprintf(t, sizeof(t), "SCORE %07lu   %s PLACE", (unsigned long)score, game_hiscore_rank_name(rank));
	game_text_set(&_text[1], t, 1);
	game_text_set(&_text[2], "ENTER YOUR INITIALS", 2);
	game_text_set(&_text[6], "LEFT/RIGHT OR UP/DOWN TO CHOOSE", 1);
	game_text_set(&_text[7], "CAST TO ENTER  JUMP WHEN DONE", 1);

	GameInitials_t g;
	game_initials_reset(&g);
	for (uint32_t frame = 0; frame < MM_ATTRACT_ENTRY_FRAMES; ++frame) {
		hstx_dvi_sprite_wait_for_frame();

		const uint8_t pressed = game_input_pressed();
		const uint8_t input = game_input_get();
		const int32_t dir = (input & (MM_INP_BIT_L | MM_INP_BIT_D)) ? -1 : (input & (MM_INP_BIT_R | MM_INP_BIT_U)) ? 1 : 0;
		const bool stepped = (pressed & (MM_INP_BIT_L | MM_INP_BIT_R | MM_INP_BIT_U | MM_INP_BIT_D)) != 0;
		if (game_initials_update(&g, dir, stepped, pressed & MM_INP_BIT_F) || (pressed & MM_INP_BIT_J)) break;

		game_spr_begin();
		draw_stars(frame);
		game_spr_text_centre(&_text[0], 80, game_pallet_rainbow(frame >> 3));
		game_spr_text_centre(&_text[1], 130, game_pallet(GAME_WHITE));
		game_spr_text_centre(&_text[2], 170, game_pallet(GAME_GREEN));
		for (uint32_t j = 0; j < GAME_HISCORE_NAME_LEN; ++j) {
			const char s[2] = { game_initials_char(&g, j), 0 };
			const uint32_t c = j < g.pos ? GAME_WHITE : j == g.pos && ((frame >> 3) & 1) ? GAME_YELLOW : GAME_GREY;
			game_text_set(&_text[3 + j], s, 4);
			game_spr_text(&_text[3 + j], (GAME_SCREEN_W >> 1) - 64 + j * 48, 220, game_pallet(c));
		}
		game_spr_text_centre(&_text[6], 300, game_pallet(GAME_CYAN));
		game_spr_text_centre(&_text[7], 316, game_pallet(GAME_CYAN));
		draw_parade(frame, 450);
		game_spr_end();
	}

	char name[GAME_HISCORE_NAME_LEN + 1];
	game_initials_name(&g, name);

	return mm_attract_hiscores(game_hiscore_insert(score, name));
}
