#include "def_attract.h"
#include "hstx_dvi_sprite.h"
#include "def_input.h"
#include "def_world.h"
#include "def_render.h"
#include "def_pallet.h"
#include "def_hud.h"
#include "def_fx.h"
#include "def_enemy.h"
#include "def_hiscore.h"
#include "def_util.h"
#include <stdio.h>
#include <string.h>

#define DEF_ATTRACT_TITLE "GUARDIAN"
#define DEF_ATTRACT_TITLE_LEN 8
#define DEF_ATTRACT_TITLE_FRAMES 660
#define DEF_ATTRACT_HISCORE_FRAMES 540
#define DEF_ATTRACT_HIGHLIGHT_FRAMES 840
// Give up waiting for initials after 30 seconds
#define DEF_ATTRACT_ENTRY_FRAMES (60 * 30)
// Frames a direction is held for before it starts to repeat, and then how often
#define DEF_ATTRACT_REPEAT_DELAY 20
#define DEF_ATTRACT_REPEAT_RATE 5
// Frames for each enemy to materialise in the roll call
#define DEF_ATTRACT_MATERIALISE 40
#define DEF_ATTRACT_ROLL_START 90
#define DEF_ATTRACT_ROLL_GAP 50

static const char _letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.-! ";
#define DEF_ATTRACT_LETTERS (sizeof(_letters) - 1)

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
static DefText_t _text[16];
static DefText_t _names[DEF_ATTRACT_ROLL_COUNT];
static DefText_t _scores[DEF_ATTRACT_ROLL_COUNT];

// Sets up a new screen with a still camera, so world and screen x are the same
static void attract_init(void) {
	def_world_set_camera(0);
	def_fx_init();
	// Forget about any buttons still held down from before
	def_input_pressed();
}

// Waits for the next frame. Returns true if the player wants to play.
static bool attract_frame(void) {
	hstx_dvi_sprite_wait_for_frame();
	def_fx_update();
	return is_def_input_play(def_input_pressed());
}

bool def_attract_title(void) {
	attract_init();
	char t[DEF_TEXT_MAX];

	// Each letter of the title separately, so they can be different colours
	for (uint32_t i = 0; i < DEF_ATTRACT_TITLE_LEN; ++i) {
		const char s[2] = { DEF_ATTRACT_TITLE[i], 0 };
		def_text_set(&_text[i], s, 4);
	}
	def_text_set(&_text[8], "DEFEND THE HUMANOIDS", 1);
	def_text_set(&_text[9], "PRESS FIRE TO START", 2);
	def_text_set(&_text[10], "RP2350 HSTX DVI SPRITES", 1);

	// Where the middle of each enemy in the roll call goes
	int32_t rx[DEF_ATTRACT_ROLL_COUNT];
	int32_t ry[DEF_ATTRACT_ROLL_COUNT];
	for (uint32_t i = 0; i < DEF_ATTRACT_ROLL_COUNT; ++i) {
		rx[i] = 120 + (i % 3) * 200;
		ry[i] = 200 + (i / 3) * 90;
		def_text_set(&_names[i], def_enemy_name(_roll[i]), 1);
		snprintf(t, sizeof(t), "%lu", (unsigned long)def_enemy_score(_roll[i]));
		def_text_set(&_scores[i], t, 1);
	}

	const int32_t tx = (DEF_SCREEN_W - DEF_ATTRACT_TITLE_LEN * 32) >> 1;
	for (uint32_t frame = 0; frame < DEF_ATTRACT_TITLE_FRAMES; ++frame) {
		if (attract_frame()) return true;

		// Each enemy is drawn in by particles before it appears
		for (uint32_t i = 0; i < DEF_ATTRACT_ROLL_COUNT; ++i) {
			if (frame == DEF_ATTRACT_ROLL_START + i * DEF_ATTRACT_ROLL_GAP) {
				def_fx_implode(DEF_PX(rx[i]), DEF_PX(ry[i] + (def_enemy_height(_roll[i]) >> 1)), def_enemy_colour(_roll[i]), 32, DEF_ATTRACT_MATERIALISE);
			}
		}

		def_spr_begin();
		def_hud_draw(0, false);
		for (uint32_t i = 0; i < DEF_ATTRACT_TITLE_LEN; ++i) {
			// A ripple of colour across the letters, which bob up and down
			const int32_t bob = def_sin((frame << 2) + i * 24) * 6 / 127;
			def_spr_text(&_text[i], tx + i * 32, 84 + bob, def_pallet_rainbow(i + (frame >> 3)));
		}
		def_spr_text_centre(&_text[8], 130, def_pallet(DEF_WHITE));
		for (uint32_t i = 0; i < DEF_ATTRACT_ROLL_COUNT; ++i) {
			if (frame < DEF_ATTRACT_ROLL_START + i * DEF_ATTRACT_ROLL_GAP + DEF_ATTRACT_MATERIALISE) continue;
			def_spr_text(&_names[i], rx[i] - (def_text_width(&_names[i]) >> 1), ry[i] + 22, def_pallet(DEF_WHITE));
			def_spr_text(&_scores[i], rx[i] - (def_text_width(&_scores[i]) >> 1), ry[i] + 36, def_pallet(DEF_YELLOW));
		}
		if ((frame >> 5) & 1) def_spr_text_centre(&_text[9], 410, def_pallet(DEF_YELLOW));
		def_spr_text_centre(&_text[10], DEF_SCREEN_H - 20, def_pallet(DEF_GREY));
		for (uint32_t i = 0; i < DEF_ATTRACT_ROLL_COUNT; ++i) {
			if (frame < DEF_ATTRACT_ROLL_START + i * DEF_ATTRACT_ROLL_GAP + DEF_ATTRACT_MATERIALISE) continue;
			def_enemy_draw_icon(_roll[i], rx[i] - (def_enemy_width(_roll[i]) >> 1), ry[i], frame);
		}
		def_fx_draw_particles();
		def_world_draw_stars(frame, frame);
		def_spr_end();
	}
	return false;
}

static uint32_t rank_colour(const uint32_t rank) {
	if (rank == 0) return DEF_YELLOW;
	if (rank < 3) return DEF_WHITE;
	if (rank < 6) return DEF_CYAN;
	return DEF_GREEN;
}

bool def_attract_hiscores(int32_t highlight) {
	attract_init();
	char rows[DEF_HISCORE_COUNT][DEF_TEXT_MAX];
	def_text_set(&_text[0], "HALL OF FAME", 3);
	for (uint32_t i = 0; i < DEF_HISCORE_COUNT; ++i) {
		const DefHiScore_t *e = def_hiscore_get(i);
		snprintf(rows[i], DEF_TEXT_MAX, "%-4s  %07lu  %s", def_hiscore_rank_name(i), (unsigned long)e->score, e->name);
	}
	def_text_set(&_text[11], "PRESS FIRE TO START", 2);

	const uint32_t frames = highlight >= 0 ? DEF_ATTRACT_HIGHLIGHT_FRAMES : DEF_ATTRACT_HISCORE_FRAMES;
	for (uint32_t frame = 0; frame < frames; ++frame) {
		if (attract_frame()) return true;

		def_spr_begin();
		def_hud_draw(0, false);
		def_spr_text_centre(&_text[0], 80, def_pallet_rainbow(frame >> 3));
		// Typed out a line at a time
		int32_t n = (int32_t)frame - 30;
		for (uint32_t i = 0; i < DEF_HISCORE_COUNT; ++i) {
			def_text_set_n(&_text[1 + i], rows[i], n < 0 ? 0 : n, 1);
			n -= strlen(rows[i]);
			const uint32_t c = (int32_t)i == highlight && ((frame >> 3) & 1) ? DEF_RED : rank_colour(i);
			def_spr_text_centre(&_text[1 + i], 140 + i * 24, def_pallet(c));
		}
		if (frame > 260 && ((frame >> 5) & 1)) def_spr_text_centre(&_text[11], 410, def_pallet(DEF_YELLOW));
		def_world_draw_stars(frame << 1, frame);
		def_spr_end();
	}
	return false;
}

bool def_attract_game_over(void) {
	const uint32_t score = def_hud_score();
	const int32_t rank = def_hiscore_rank(score);
	if (rank < 0) return false;

	attract_init();
	char t[DEF_TEXT_MAX];
	def_text_set(&_text[0], "NEW HIGH SCORE", 3);
	snprintf(t, sizeof(t), "SCORE %07lu   %s PLACE", (unsigned long)score, def_hiscore_rank_name(rank));
	def_text_set(&_text[1], t, 1);
	def_text_set(&_text[2], "ENTER YOUR INITIALS", 2);
	def_text_set(&_text[6], "LEFT/RIGHT OR UP/DOWN TO CHOOSE", 1);
	def_text_set(&_text[7], "FIRE TO ENTER  SMART BOMB WHEN DONE", 1);

	uint32_t letter[DEF_HISCORE_NAME_LEN] = {0};
	uint32_t pos = 0;
	uint32_t held = 0;
	for (uint32_t frame = 0; frame < DEF_ATTRACT_ENTRY_FRAMES && pos < DEF_HISCORE_NAME_LEN; ++frame) {
		hstx_dvi_sprite_wait_for_frame();

		const uint8_t pressed = def_input_pressed();
		const uint8_t input = def_input_get();
		const int32_t dir = (input & (DEF_INP_BIT_L | DEF_INP_BIT_D)) ? -1 : (input & (DEF_INP_BIT_R | DEF_INP_BIT_U)) ? 1 : 0;
		held = dir ? held + 1 : 0;
		const bool step = (pressed & (DEF_INP_BIT_L | DEF_INP_BIT_R | DEF_INP_BIT_U | DEF_INP_BIT_D)) ||
			(held > DEF_ATTRACT_REPEAT_DELAY && (held % DEF_ATTRACT_REPEAT_RATE) == 0);
		if (dir && step) {
			letter[pos] = (letter[pos] + DEF_ATTRACT_LETTERS + dir) % DEF_ATTRACT_LETTERS;
		}
		if (pressed & DEF_INP_BIT_F) {
			++pos;
		}
		else if (pressed & DEF_INP_BIT_B) {
			break;
		}

		def_spr_begin();
		def_hud_draw(0, false);
		def_spr_text_centre(&_text[0], 80, def_pallet_rainbow(frame >> 3));
		def_spr_text_centre(&_text[1], 130, def_pallet(DEF_WHITE));
		def_spr_text_centre(&_text[2], 170, def_pallet(DEF_GREEN));
		for (uint32_t j = 0; j < DEF_HISCORE_NAME_LEN; ++j) {
			const char s[2] = { j <= pos ? _letters[letter[j]] : '-', 0 };
			const uint32_t c = j < pos ? DEF_WHITE : j == pos && ((frame >> 3) & 1) ? DEF_YELLOW : DEF_GREY;
			def_text_set(&_text[3 + j], s, 4);
			def_spr_text(&_text[3 + j], (DEF_SCREEN_W >> 1) - 64 + j * 48, 220, def_pallet(c));
		}
		def_spr_text_centre(&_text[6], 300, def_pallet(DEF_CYAN));
		def_spr_text_centre(&_text[7], 316, def_pallet(DEF_CYAN));
		def_world_draw_stars(frame, frame);
		def_spr_end();
	}

	// Letters not got to are left blank
	char name[DEF_HISCORE_NAME_LEN + 1];
	for (uint32_t j = 0; j < DEF_HISCORE_NAME_LEN; ++j) {
		name[j] = j <= pos ? _letters[letter[j]] : ' ';
	}
	name[DEF_HISCORE_NAME_LEN] = 0;

	return def_attract_hiscores(def_hiscore_insert(score, name));
}
