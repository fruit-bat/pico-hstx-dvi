#include "def_attract.h"
#include "hstx_dvi_sprite.h"
#include "def_input.h"
#include "def_world.h"
#include "game_render.h"
#include "game_pallet.h"
#include "def_hud.h"
#include "game_fx.h"
#include "def_enemy.h"
#include "game_hiscore.h"
#include "game_initials.h"
#include "game_util.h"
#include <stdio.h>
#include <string.h>

#define DEF_ATTRACT_TITLE "GUARDIAN"
#define DEF_ATTRACT_TITLE_LEN 8
#define DEF_ATTRACT_TITLE_FRAMES 660
#define DEF_ATTRACT_HISCORE_FRAMES 540
#define DEF_ATTRACT_HIGHLIGHT_FRAMES 840
// Give up waiting for initials after 30 seconds
#define DEF_ATTRACT_ENTRY_FRAMES (60 * 30)
// Frames for each enemy to materialise in the roll call
#define DEF_ATTRACT_MATERIALISE 40
#define DEF_ATTRACT_ROLL_START 90
#define DEF_ATTRACT_ROLL_GAP 50

static const uint32_t _roll[] = {
	DEF_ENEMY_LANDER,
	DEF_ENEMY_MUTANT,
	DEF_ENEMY_BAITER,
	DEF_ENEMY_BOMBER,
	DEF_ENEMY_POD,
	DEF_ENEMY_SWARMER
};
#define DEF_ATTRACT_ROLL_COUNT (sizeof(_roll) / sizeof(_roll[0]))

// Text has to outlive the screen that shows it, as the renderer keeps
// drawing the last frame until the next screen has been set up
static GameText_t _text[16];
static GameText_t _names[DEF_ATTRACT_ROLL_COUNT];
static GameText_t _scores[DEF_ATTRACT_ROLL_COUNT];

// Sets up a new screen with a still camera, so world and screen x are the same
static void attract_init(void) {
	def_world_set_camera(0);
	game_fx_init();
	// Forget about any buttons still held down from before
	game_input_pressed();
}

// Waits for the next frame. Returns true if the player wants to play.
static bool attract_frame(void) {
	hstx_dvi_sprite_wait_for_frame();
	game_fx_update();
	return is_def_input_play(game_input_pressed());
}

bool def_attract_title(void) {
	attract_init();
	char t[GAME_TEXT_MAX];

	// Each letter of the title separately, so they can be different colours
	for (uint32_t i = 0; i < DEF_ATTRACT_TITLE_LEN; ++i) {
		const char s[2] = { DEF_ATTRACT_TITLE[i], 0 };
		game_text_set(&_text[i], s, 4);
	}
	game_text_set(&_text[8], "DEFEND THE HUMANOIDS", 1);
	game_text_set(&_text[9], "PRESS FIRE TO START", 2);
	game_text_set(&_text[10], "RP2350 HSTX DVI SPRITES", 1);

	// Where the middle of each enemy in the roll call goes
	int32_t rx[DEF_ATTRACT_ROLL_COUNT];
	int32_t ry[DEF_ATTRACT_ROLL_COUNT];
	for (uint32_t i = 0; i < DEF_ATTRACT_ROLL_COUNT; ++i) {
		rx[i] = 120 + (i % 3) * 200;
		ry[i] = 200 + (i / 3) * 90;
		game_text_set(&_names[i], def_enemy_name(_roll[i]), 1);
		snprintf(t, sizeof(t), "%lu", (unsigned long)def_enemy_score(_roll[i]));
		game_text_set(&_scores[i], t, 1);
	}

	const int32_t tx = (GAME_SCREEN_W - DEF_ATTRACT_TITLE_LEN * 32) >> 1;
	for (uint32_t frame = 0; frame < DEF_ATTRACT_TITLE_FRAMES; ++frame) {
		if (attract_frame()) return true;

		// Each enemy is drawn in by particles before it appears
		for (uint32_t i = 0; i < DEF_ATTRACT_ROLL_COUNT; ++i) {
			if (frame == DEF_ATTRACT_ROLL_START + i * DEF_ATTRACT_ROLL_GAP) {
				game_fx_implode(DEF_PX(rx[i]), DEF_PX(ry[i] + (def_enemy_height(_roll[i]) >> 1)), def_enemy_colour(_roll[i]), 32, DEF_ATTRACT_MATERIALISE);
			}
		}

		game_spr_begin();
		def_hud_draw(0, false);
		for (uint32_t i = 0; i < DEF_ATTRACT_TITLE_LEN; ++i) {
			// A ripple of colour across the letters, which bob up and down
			const int32_t bob = game_sin((frame << 2) + i * 24) * 6 / 127;
			game_spr_text(&_text[i], tx + i * 32, 84 + bob, game_pallet_rainbow(i + (frame >> 3)));
		}
		game_spr_text_centre(&_text[8], 130, game_pallet(GAME_WHITE));
		for (uint32_t i = 0; i < DEF_ATTRACT_ROLL_COUNT; ++i) {
			if (frame < DEF_ATTRACT_ROLL_START + i * DEF_ATTRACT_ROLL_GAP + DEF_ATTRACT_MATERIALISE) continue;
			game_spr_text(&_names[i], rx[i] - (game_text_width(&_names[i]) >> 1), ry[i] + 22, game_pallet(GAME_WHITE));
			game_spr_text(&_scores[i], rx[i] - (game_text_width(&_scores[i]) >> 1), ry[i] + 36, game_pallet(GAME_YELLOW));
		}
		if ((frame >> 5) & 1) game_spr_text_centre(&_text[9], 410, game_pallet(GAME_YELLOW));
		game_spr_text_centre(&_text[10], GAME_SCREEN_H - 20, game_pallet(GAME_GREY));
		for (uint32_t i = 0; i < DEF_ATTRACT_ROLL_COUNT; ++i) {
			if (frame < DEF_ATTRACT_ROLL_START + i * DEF_ATTRACT_ROLL_GAP + DEF_ATTRACT_MATERIALISE) continue;
			def_enemy_draw_icon(_roll[i], rx[i] - (def_enemy_width(_roll[i]) >> 1), ry[i], frame);
		}
		game_fx_draw_particles();
		def_world_draw_stars(frame, frame);
		game_spr_end();
	}
	return false;
}

static uint32_t rank_colour(const uint32_t rank) {
	if (rank == 0) return GAME_YELLOW;
	if (rank < 3) return GAME_WHITE;
	if (rank < 6) return GAME_CYAN;
	return GAME_GREEN;
}

bool def_attract_hiscores(int32_t highlight) {
	attract_init();
	char rows[GAME_HISCORE_COUNT][GAME_TEXT_MAX];
	game_text_set(&_text[0], "HALL OF FAME", 3);
	for (uint32_t i = 0; i < GAME_HISCORE_COUNT; ++i) {
		const GameHiScore_t *e = game_hiscore_get(i);
		snprintf(rows[i], GAME_TEXT_MAX, "%-4s  %07lu  %s", game_hiscore_rank_name(i), (unsigned long)e->score, e->name);
	}
	game_text_set(&_text[11], "PRESS FIRE TO START", 2);

	const uint32_t frames = highlight >= 0 ? DEF_ATTRACT_HIGHLIGHT_FRAMES : DEF_ATTRACT_HISCORE_FRAMES;
	for (uint32_t frame = 0; frame < frames; ++frame) {
		if (attract_frame()) return true;

		game_spr_begin();
		def_hud_draw(0, false);
		game_spr_text_centre(&_text[0], 80, game_pallet_rainbow(frame >> 3));
		// Typed out a line at a time
		int32_t n = (int32_t)frame - 30;
		for (uint32_t i = 0; i < GAME_HISCORE_COUNT; ++i) {
			game_text_set_n(&_text[1 + i], rows[i], n < 0 ? 0 : n, 1);
			n -= strlen(rows[i]);
			const uint32_t c = (int32_t)i == highlight && ((frame >> 3) & 1) ? GAME_RED : rank_colour(i);
			game_spr_text_centre(&_text[1 + i], 140 + i * 24, game_pallet(c));
		}
		if (frame > 260 && ((frame >> 5) & 1)) game_spr_text_centre(&_text[11], 410, game_pallet(GAME_YELLOW));
		def_world_draw_stars(frame << 1, frame);
		game_spr_end();
	}
	return false;
}

bool def_attract_game_over(void) {
	const uint32_t score = def_hud_score();
	const int32_t rank = game_hiscore_rank(score);
	if (rank < 0) return false;

	attract_init();
	char t[GAME_TEXT_MAX];
	game_text_set(&_text[0], "NEW HIGH SCORE", 3);
	snprintf(t, sizeof(t), "SCORE %07lu   %s PLACE", (unsigned long)score, game_hiscore_rank_name(rank));
	game_text_set(&_text[1], t, 1);
	game_text_set(&_text[2], "ENTER YOUR INITIALS", 2);
	game_text_set(&_text[6], "LEFT/RIGHT OR UP/DOWN TO CHOOSE", 1);
	game_text_set(&_text[7], "FIRE TO ENTER  SMART BOMB WHEN DONE", 1);

	GameInitials_t g;
	game_initials_reset(&g);
	for (uint32_t frame = 0; frame < DEF_ATTRACT_ENTRY_FRAMES; ++frame) {
		hstx_dvi_sprite_wait_for_frame();

		const uint8_t pressed = game_input_pressed();
		const uint8_t input = game_input_get();
		const int32_t dir = (input & (DEF_INP_BIT_L | DEF_INP_BIT_D)) ? -1 : (input & (DEF_INP_BIT_R | DEF_INP_BIT_U)) ? 1 : 0;
		const bool stepped = (pressed & (DEF_INP_BIT_L | DEF_INP_BIT_R | DEF_INP_BIT_U | DEF_INP_BIT_D)) != 0;
		if (game_initials_update(&g, dir, stepped, pressed & DEF_INP_BIT_F) || (pressed & DEF_INP_BIT_B)) break;

		game_spr_begin();
		def_hud_draw(0, false);
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
		def_world_draw_stars(frame, frame);
		game_spr_end();
	}

	char name[GAME_HISCORE_NAME_LEN + 1];
	game_initials_name(&g, name);

	return def_attract_hiscores(game_hiscore_insert(score, name));
}
