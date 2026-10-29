#include "inv_attract.h"
#include "hstx_dvi_sprite.h"
#include "inv_input.h"
#include "inv_invaders.h"
#include "inv_title.h"
#include "inv_score.h"
#include "game_pallet.h"
#include "inv_base.h"
#include "game_hiscore.h"
#include "game_initials.h"
#include "game_render.h"
#include "inv_screen.h"
#include <stdio.h>

#define INV_ATTRACT_TITLE_FRAMES 600
#define INV_ATTRACT_TABLE_FRAMES 600
#define INV_ATTRACT_HISCORE_FRAMES 600
#define INV_ATTRACT_HIGHLIGHT_FRAMES 900
// Give up waiting for initials after 30 seconds
#define INV_ATTRACT_ENTRY_FRAMES (60 * 30)
#define INV_ATTRACT_PARADE_COUNT 8
#define INV_ATTRACT_PARADE_GAP 56
#define INV_ATTRACT_PARADE_Y 400

// The invaders shown off on the score table and in the parade, from the mystery ship down
static const uint32_t _types[4] = {
	INV_INVADER_TYPE_MOT, INV_ALIEN_COMMANDER, INV_ALIEN_CRAB, INV_ALIEN_JELLY
};

// Sets up a new screen with the score line and some text slots
static SpriteId attract_init(const uint32_t slots) {
	inv_screen_clear();
	SpriteId si = 0;
	si = inv_score_init(si, false);
	si = inv_screen_init(si, slots);
	// Forget about any buttons still held down from before
	game_input_pressed();
	return si;
}

// Returns true if the player wants to play
static bool attract_frame(void) {
	hstx_dvi_sprite_wait_for_frame();
	inv_score_update();
	return is_inv_input_play(game_input_pressed());
}

// Types out the text slots first to last, one after the other, a character
// every rate frames starting at frame start
static void type_out(const uint32_t first, const uint32_t last, const uint32_t frame, const uint32_t start, const uint32_t rate) {
	int32_t n = ((int32_t)frame - (int32_t)start) / (int32_t)rate;
	for (uint32_t slot = first; slot <= last; ++slot) {
		inv_screen_text_reveal(slot, n < 0 ? 0 : n);
		n -= inv_screen_text_len(slot);
	}
}

bool inv_attract_title(void) {
	SpriteId si = attract_init(3);
	si = inv_base_init(si);
	si = inv_title_init(si);

	char t[INV_SCREEN_TEXT_MAX];
	const GameHiScore_t *best = game_hiscore_get(0);
	snprintf(t, sizeof(t), "HI-SCORE  %06lu  %s", (unsigned long)best->score, best->name);
	inv_screen_text_centre(0, 230, t, game_pallet(GAME_WHITE), false);
	inv_screen_text_centre(1, 300, "PRESS FIRE TO PLAY", game_pallet(GAME_YELLOW), true);
	inv_screen_text_centre(2, MODE_V_ACTIVE_LINES - 24, "RP2350 HSTX DVI SPRITES", game_pallet(GAME_GREY), false);

	for (uint32_t frame = 0; frame < INV_ATTRACT_TITLE_FRAMES; ++frame) {
		if (attract_frame()) return true;
		inv_title_update(frame, INV_ATTRACT_TITLE_FRAMES);
		type_out(0, 0, frame, 90, 2);
		inv_screen_text_show(1, frame > 90 && ((frame >> 5) & 1));
	}
	return false;
}

bool inv_attract_score_table(void) {
	const SpriteId si = attract_init(9);

	char t[INV_SCREEN_TEXT_MAX];
	inv_screen_text_centre(0, 90, "*SCORE ADVANCE TABLE*", game_pallet(GAME_WHITE), true);
	// When each row starts to be typed out
	uint32_t starts[4];
	uint32_t typed = inv_screen_text_len(0);
	for (uint32_t i = 0; i < 4; ++i) {
		const uint32_t type = _types[i];
		if (type == INV_INVADER_TYPE_MOT) {
			snprintf(t, sizeof(t), "= ? MYSTERY");
		}
		else {
			snprintf(t, sizeof(t), "=%2lu %s", (unsigned long)inv_invaders_type_score(type), inv_invaders_type_name(type));
		}
		inv_screen_text(1 + i, 256, 150 + i * 40, t, game_pallet(GAME_WHITE), false);
		starts[i] = typed;
		typed += inv_screen_text_len(1 + i);
	}
	inv_screen_text_centre(5, 320, "COMMANDERS TAKE TWO HITS", game_pallet(GAME_PINK), false);
	inv_screen_text_centre(6, 340, "DOUBLE POINTS FOR DIVERS", game_pallet(GAME_PINK), false);
	snprintf(t, sizeof(t), "BONUS LIFE EVERY %u POINTS", INV_SCORE_BONUS_LIFE);
	inv_screen_text_centre(7, 370, t, game_pallet(GAME_GREEN), false);
	inv_screen_text_centre(8, 420, "PRESS FIRE TO PLAY", game_pallet(GAME_YELLOW), false);
	for (uint32_t i = 5; i <= 7; ++i) typed += inv_screen_text_len(i);

	const uint32_t start = 20;
	const uint32_t rate = 3;
	for (uint32_t frame = 0; frame < INV_ATTRACT_TABLE_FRAMES; ++frame) {
		if (attract_frame()) return true;
		type_out(0, 7, frame, start, rate);
		game_spr_begin_at(si);
		for (uint32_t i = 0; i < 4; ++i) {
			if (frame < start + starts[i] * rate) continue;
			// The ship is wider, so it sits further left
			const int32_t x = _types[i] == INV_INVADER_TYPE_MOT ? 212 : 220;
			inv_invaders_draw_icon(_types[i], x, 147 + i * 40 - 4, frame);
		}
		game_spr_end();
		inv_screen_text_show(8, frame > start + typed * rate && ((frame >> 5) & 1));
	}
	return false;
}

static hstx_dvi_pixel_t* rank_pallet(const uint32_t rank) {
	if (rank == 0) return game_pallet(GAME_YELLOW);
	if (rank < 3) return game_pallet(GAME_WHITE);
	if (rank < 6) return game_pallet(GAME_BLUE);
	return game_pallet(GAME_PURPLE);
}

bool inv_attract_hiscores(int32_t highlight) {
	const SpriteId si = attract_init(12);

	char t[INV_SCREEN_TEXT_MAX];
	inv_screen_text_centre(0, 48, "HIGH SCORES", game_pallet_rainbow(0), true);
	for (uint32_t i = 0; i < GAME_HISCORE_COUNT; ++i) {
		const GameHiScore_t *e = game_hiscore_get(i);
		snprintf(t, sizeof(t), "%-4s  %06lu  %s", game_hiscore_rank_name(i), (unsigned long)e->score, e->name);
		inv_screen_text_centre(1 + i, 100 + i * 26, t, rank_pallet(i), false);
	}
	inv_screen_text_centre(11, 448, "PRESS FIRE TO PLAY", game_pallet(GAME_YELLOW), false);

	const uint32_t frames = highlight >= 0 ? INV_ATTRACT_HIGHLIGHT_FRAMES : INV_ATTRACT_HISCORE_FRAMES;
	const uint32_t loop = MODE_H_ACTIVE_PIXELS + 32 + INV_ATTRACT_PARADE_COUNT * INV_ATTRACT_PARADE_GAP;
	for (uint32_t frame = 0; frame < frames; ++frame) {
		if (attract_frame()) return true;
		type_out(1, GAME_HISCORE_COUNT, frame, 30, 1);
		inv_screen_text_pallet(0, game_pallet_rainbow(frame >> 3));
		if (highlight >= 0) {
			inv_screen_text_pallet(1 + highlight, (frame >> 3) & 1 ? game_pallet(GAME_RED) : rank_pallet(highlight));
		}
		inv_screen_text_show(11, frame > 220 && ((frame >> 5) & 1));

		// A mystery ship leading a parade of invaders along the bottom
		game_spr_begin_at(si);
		for (uint32_t k = 0; k < INV_ATTRACT_PARADE_COUNT; ++k) {
			const uint32_t type = _types[k == 0 ? 0 : 1 + (k - 1) % INV_INVADER_TYPE_COUNT];
			const int32_t x = (int32_t)((frame + loop - k * INV_ATTRACT_PARADE_GAP) % loop) - 32;
			inv_invaders_draw_icon(type, x, INV_ATTRACT_PARADE_Y, frame + k * 8);
		}
		game_spr_end();
	}
	return false;
}

bool inv_attract_game_over(void) {
	const uint32_t score = inv_score_get();
	const int32_t rank = game_hiscore_rank(score);
	if (rank < 0) return false;

	attract_init(8);

	char t[INV_SCREEN_TEXT_MAX];
	inv_screen_text_centre(0, 70, "NEW HIGH SCORE", game_pallet_rainbow(0), true);
	snprintf(t, sizeof(t), "SCORE %06lu   %s PLACE", (unsigned long)score, game_hiscore_rank_name(rank));
	inv_screen_text_centre(1, 120, t, game_pallet(GAME_WHITE), false);
	inv_screen_text_centre(2, 170, "ENTER YOUR INITIALS", game_pallet(GAME_GREEN), false);
	inv_screen_text_centre(6, 290, "LEFT/RIGHT TO CHOOSE  FIRE TO ENTER", game_pallet(GAME_BLUE), false);
	inv_screen_text_centre(7, 310, "START WHEN DONE", game_pallet(GAME_GREY), false);

	GameInitials_t g;
	game_initials_reset(&g);
	for (uint32_t frame = 0; frame < INV_ATTRACT_ENTRY_FRAMES; ++frame) {
		hstx_dvi_sprite_wait_for_frame();
		inv_score_update();
		inv_screen_text_pallet(0, game_pallet_rainbow(frame >> 3));

		const uint8_t pressed = game_input_pressed();
		const uint8_t input = game_input_get();
		const int32_t dir = is_inv_input_left(input) ? -1 : is_inv_input_right(input) ? 1 : 0;
		const bool stepped = (pressed & (INV_INP_BIT_L | INV_INP_BIT_R)) != 0;
		if (game_initials_update(&g, dir, stepped, is_inv_input_fire(pressed)) || is_inv_input_start(pressed)) break;

		for (uint32_t j = 0; j < GAME_HISCORE_NAME_LEN; ++j) {
			const char s[2] = { game_initials_char(&g, j), 0 };
			hstx_dvi_pixel_t *p = j < g.pos ? game_pallet(GAME_WHITE) : j == g.pos && ((frame >> 3) & 1) ? game_pallet(GAME_YELLOW) : game_pallet(GAME_GREY);
			inv_screen_text(3 + j, (MODE_H_ACTIVE_PIXELS >> 1) - 48 + j * 32, 220, s, p, true);
		}
	}

	char name[GAME_HISCORE_NAME_LEN + 1];
	game_initials_name(&g, name);

	return inv_attract_hiscores(game_hiscore_insert(score, name));
}
