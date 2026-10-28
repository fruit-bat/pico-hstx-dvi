#include "inv_attract.h"
#include "hstx_dvi_sprite.h"
#include "inv_input.h"
#include "inv_invaders.h"
#include "inv_score.h"
#include "inv_pallet.h"
#include "inv_base.h"
#include "inv_hiscore.h"
#include "inv_screen.h"
#include <stdio.h>

#define INV_ATTRACT_TITLE_FRAMES 600
#define INV_ATTRACT_TABLE_FRAMES 600
#define INV_ATTRACT_HISCORE_FRAMES 600
#define INV_ATTRACT_HIGHLIGHT_FRAMES 900
// Give up waiting for initials after 30 seconds
#define INV_ATTRACT_ENTRY_FRAMES (60 * 30)
// Frames a direction is held for before it starts to repeat, and then how often
#define INV_ATTRACT_REPEAT_DELAY 20
#define INV_ATTRACT_REPEAT_RATE 5
#define INV_ATTRACT_PARADE_COUNT 8
#define INV_ATTRACT_PARADE_GAP 40

static const char _letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.-! ";
#define INV_ATTRACT_LETTERS (sizeof(_letters) - 1)

// Sets up a new screen with the score line and some text slots
static SpriteId attract_init(const uint32_t slots) {
	inv_screen_clear();
	SpriteId si = 0;
	si = inv_score_init(si, false);
	si = inv_screen_init(si, slots);
	// Forget about any buttons still held down from before
	inv_input_pressed();
	return si;
}

// Returns true if the player wants to play
static bool attract_frame(void) {
	hstx_dvi_sprite_wait_for_frame();
	inv_score_update();
	return is_inv_input_play(inv_input_pressed());
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
	si = inv_invaders_init_title(si);

	char t[INV_SCREEN_TEXT_MAX];
	const InvHiScore_t *best = inv_hiscore_get(0);
	snprintf(t, sizeof(t), "HI-SCORE  %06lu  %s", (unsigned long)best->score, best->name);
	inv_screen_text_centre(0, 230, t, inv_pallet_white(), false);
	inv_screen_text_centre(1, 300, "PRESS FIRE TO PLAY", inv_pallet_yellow(), true);
	inv_screen_text_centre(2, MODE_V_ACTIVE_LINES - 24, "RP2350 HSTX DVI SPRITES", inv_pallet_grey(), false);

	for (uint32_t frame = 0; frame < INV_ATTRACT_TITLE_FRAMES; ++frame) {
		if (attract_frame()) return true;
		inv_invaders_update_title(frame, INV_ATTRACT_TITLE_FRAMES);
		type_out(0, 0, frame, 90, 2);
		inv_screen_text_show(1, frame > 90 && ((frame >> 5) & 1));
	}
	return false;
}

bool inv_attract_score_table(void) {
	static const uint32_t types[4] = {
		INV_INVADER_TYPE_MOT, 0, 1, 2
	};
	const SpriteId si = attract_init(7);

	char t[INV_SCREEN_TEXT_MAX];
	inv_screen_text_centre(0, 90, "*SCORE ADVANCE TABLE*", inv_pallet_white(), true);
	// When each row starts to be typed out
	uint32_t starts[4];
	uint32_t typed = inv_screen_text_len(0);
	for (uint32_t i = 0; i < 4; ++i) {
		const int32_t y = 160 + i * 40;
		const uint32_t type = types[i];
		if (type == INV_INVADER_TYPE_MOT) {
			snprintf(t, sizeof(t), "= ? MYSTERY");
		}
		else {
			snprintf(t, sizeof(t), "=%2lu POINTS", (unsigned long)inv_invaders_type_score(type));
		}
		inv_screen_text(1 + i, 264, y, t, inv_pallet_white(), false);
		starts[i] = typed;
		typed += inv_screen_text_len(1 + i);
		init_sprite(
			si + i,
			232,
			y,
			16,
			8,
			0,
			(void*)inv_invaders_tile(type, 0),
			inv_invaders_pallet(type),
			sprite_renderer_sprite_16x8_p1);
	}
	snprintf(t, sizeof(t), "BONUS LIFE EVERY %u POINTS", INV_SCORE_BONUS_LIFE);
	inv_screen_text_centre(5, 340, t, inv_pallet_green(), false);
	inv_screen_text_centre(6, 420, "PRESS FIRE TO PLAY", inv_pallet_yellow(), false);

	const uint32_t start = 20;
	const uint32_t rate = 3;
	for (uint32_t frame = 0; frame < INV_ATTRACT_TABLE_FRAMES; ++frame) {
		if (attract_frame()) return true;
		type_out(0, 5, frame, start, rate);
		for (uint32_t i = 0; i < 4; ++i) {
			Sprite *sprite = hstx_dvi_sprite_get(si + i);
			sprite->d1 = (void*)inv_invaders_tile(types[i], frame >> 5);
			if (frame >= start + starts[i] * rate) sprite->f |= SF_ENABLE;
		}
		inv_screen_text_show(6, frame > start + typed * rate && ((frame >> 5) & 1));
	}
	return false;
}

static hstx_dvi_pixel_t* rank_pallet(const uint32_t rank) {
	if (rank == 0) return inv_pallet_yellow();
	if (rank < 3) return inv_pallet_white();
	if (rank < 6) return inv_pallet_blue();
	return inv_pallet_purple();
}

bool inv_attract_hiscores(int32_t highlight) {
	const SpriteId si = attract_init(12);

	char t[INV_SCREEN_TEXT_MAX];
	inv_screen_text_centre(0, 48, "HIGH SCORES", inv_pallet_rainbow(0), true);
	for (uint32_t i = 0; i < INV_HISCORE_COUNT; ++i) {
		const InvHiScore_t *e = inv_hiscore_get(i);
		snprintf(t, sizeof(t), "%-4s  %06lu  %s", inv_hiscore_rank_name(i), (unsigned long)e->score, e->name);
		inv_screen_text_centre(1 + i, 100 + i * 26, t, rank_pallet(i), false);
	}
	inv_screen_text_centre(11, 448, "PRESS FIRE TO PLAY", inv_pallet_yellow(), false);

	// A mystery ship leading a parade of invaders along the bottom
	for (uint32_t k = 0; k < INV_ATTRACT_PARADE_COUNT; ++k) {
		const uint32_t type = k == 0 ? INV_INVADER_TYPE_MOT : (k - 1) % INV_INVADER_TYPE_COUNT;
		init_sprite(
			si + k,
			-16,
			400,
			16,
			8,
			SF_ENABLE,
			(void*)inv_invaders_tile(type, 0),
			inv_invaders_pallet(type),
			sprite_renderer_sprite_16x8_p1);
	}

	const uint32_t frames = highlight >= 0 ? INV_ATTRACT_HIGHLIGHT_FRAMES : INV_ATTRACT_HISCORE_FRAMES;
	const uint32_t loop = MODE_H_ACTIVE_PIXELS + 32 + INV_ATTRACT_PARADE_COUNT * INV_ATTRACT_PARADE_GAP;
	for (uint32_t frame = 0; frame < frames; ++frame) {
		if (attract_frame()) return true;
		type_out(1, INV_HISCORE_COUNT, frame, 30, 1);
		inv_screen_text_pallet(0, inv_pallet_rainbow(frame >> 3));
		if (highlight >= 0) {
			inv_screen_text_pallet(1 + highlight, (frame >> 3) & 1 ? inv_pallet_red() : rank_pallet(highlight));
		}
		inv_screen_text_show(11, frame > 220 && ((frame >> 5) & 1));

		for (uint32_t k = 0; k < INV_ATTRACT_PARADE_COUNT; ++k) {
			const uint32_t type = k == 0 ? INV_INVADER_TYPE_MOT : (k - 1) % INV_INVADER_TYPE_COUNT;
			Sprite *sprite = hstx_dvi_sprite_get(si + k);
			sprite->x = (int32_t)((frame + loop - k * INV_ATTRACT_PARADE_GAP) % loop) - 16;
			sprite->d1 = (void*)inv_invaders_tile(type, sprite->x >> 3);
		}
	}
	return false;
}

bool inv_attract_game_over(void) {
	const uint32_t score = inv_score_get();
	const int32_t rank = inv_hiscore_rank(score);
	if (rank < 0) return false;

	attract_init(8);

	char t[INV_SCREEN_TEXT_MAX];
	inv_screen_text_centre(0, 70, "NEW HIGH SCORE", inv_pallet_rainbow(0), true);
	snprintf(t, sizeof(t), "SCORE %06lu   %s PLACE", (unsigned long)score, inv_hiscore_rank_name(rank));
	inv_screen_text_centre(1, 120, t, inv_pallet_white(), false);
	inv_screen_text_centre(2, 170, "ENTER YOUR INITIALS", inv_pallet_green(), false);
	inv_screen_text_centre(6, 290, "LEFT/RIGHT TO CHOOSE  FIRE TO ENTER", inv_pallet_blue(), false);
	inv_screen_text_centre(7, 310, "START WHEN DONE", inv_pallet_grey(), false);

	uint32_t letter[INV_HISCORE_NAME_LEN] = {0};
	uint32_t pos = 0;
	uint32_t held = 0;
	for (uint32_t frame = 0; frame < INV_ATTRACT_ENTRY_FRAMES && pos < INV_HISCORE_NAME_LEN; ++frame) {
		hstx_dvi_sprite_wait_for_frame();
		inv_score_update();
		inv_screen_text_pallet(0, inv_pallet_rainbow(frame >> 3));

		const uint8_t pressed = inv_input_pressed();
		const uint8_t input = get_inv_input();
		const int32_t dir = is_inv_input_left(input) ? -1 : is_inv_input_right(input) ? 1 : 0;
		held = dir ? held + 1 : 0;
		const bool step = (pressed & (INV_INP_BIT_L | INV_INP_BIT_R)) ||
			(held > INV_ATTRACT_REPEAT_DELAY && (held % INV_ATTRACT_REPEAT_RATE) == 0);
		if (dir && step) {
			letter[pos] = (letter[pos] + INV_ATTRACT_LETTERS + dir) % INV_ATTRACT_LETTERS;
		}
		if (is_inv_input_fire(pressed)) {
			++pos;
		}
		else if (is_inv_input_start(pressed)) {
			break;
		}

		for (uint32_t j = 0; j < INV_HISCORE_NAME_LEN; ++j) {
			const char s[2] = { j <= pos ? _letters[letter[j]] : '-', 0 };
			hstx_dvi_pixel_t *p = j < pos ? inv_pallet_white() : j == pos && ((frame >> 3) & 1) ? inv_pallet_yellow() : inv_pallet_grey();
			inv_screen_text(3 + j, (MODE_H_ACTIVE_PIXELS >> 1) - 48 + j * 32, 220, s, p, true);
		}
	}

	// Letters not got to are left blank
	char name[INV_HISCORE_NAME_LEN + 1];
	for (uint32_t j = 0; j < INV_HISCORE_NAME_LEN; ++j) {
		name[j] = j <= pos ? _letters[letter[j]] : ' ';
	}
	name[INV_HISCORE_NAME_LEN] = 0;

	return inv_attract_hiscores(inv_hiscore_insert(score, name));
}
