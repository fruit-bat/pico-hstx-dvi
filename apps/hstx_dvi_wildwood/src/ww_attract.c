#include "ww_attract.h"
#include "hstx_dvi_sprite.h"
#include "ww_input.h"
#include "ww_witch.h"
#include "ww_animal.h"
#include "ww_items.h"
#include "ww_hud.h"
#include "ww_art.h"
#include "game_render.h"
#include "game_pallet.h"
#include "game_fx.h"
#include "game_hiscore.h"
#include "game_initials.h"
#include "game_util.h"
#include <stdio.h>
#include <string.h>

#define WW_ATTRACT_TITLE "WILDWOOD"
#define WW_ATTRACT_TITLE_LEN 8
#define WW_ATTRACT_TITLE_FRAMES 900
#define WW_ATTRACT_HISCORE_FRAMES 600
#define WW_ATTRACT_HIGHLIGHT_FRAMES 900
// Give up waiting for initials after 30 seconds
#define WW_ATTRACT_ENTRY_FRAMES (60 * 30)
#define WW_ATTRACT_MATERIALISE 40
#define WW_ATTRACT_ROLL_START 60
#define WW_ATTRACT_ROLL_GAP 60
#define WW_ATTRACT_ROLL_Y 128
#define WW_ATTRACT_ROLL_H 52
#define WW_ATTRACT_STARS 48

typedef struct {
	uint8_t type;
	uint8_t mood;
	const char* line1;
	const char* line2;
} WwRoll_t;

static const WwRoll_t _roll[] = {
	{ WW_HEDGEHOG, WW_MOOD_HURT, "HURT", "CAST A SPELL TO HEAL" },
	{ WW_FOX, WW_MOOD_GRUMPY, "GRUMPY", "CAST SPELLS TO CALM, OR KEEP AWAY" },
	{ WW_BEAR, WW_MOOD_ASLEEP, "ASLEEP", "HOLD TIPTOE TO CREEP PAST" },
	{ WW_OWLET, WW_MOOD_LOST, "LOST", "LEAD IT HOME TO ITS MUMMY" },
	{ WW_WOLF, WW_MOOD_GRUMPY, "THE OLD WOLF", "ONLY THE WHOLE MOON CHARM CALMS HIM" },
};
#define WW_ATTRACT_ROLL_COUNT (sizeof(_roll) / sizeof(_roll[0]))

static const char* const _effect_names[WW_EFFECTS] = { "HEARTS", "SPEEDY", "HUSH", "MUDDLE" };
static const uint8_t _effect_text[WW_EFFECTS] = { GAME_PINK, GAME_YELLOW, GAME_CYAN, GAME_PURPLE };

// Text has to outlive the screen that shows it, as the renderer keeps
// drawing the last frame until the next screen has been set up
static GameText_t _text[16];
static GameText_t _roll_text[WW_ATTRACT_ROLL_COUNT][2];
static GameText_t _effect_texts[WW_EFFECTS];

typedef struct {
	uint16_t x;
	uint16_t y;
	uint8_t c;
} WwStar_t;

static WwStar_t _stars[WW_ATTRACT_STARS];
static bool _stars_made = false;
static Tile8x8p2_t _tile_star = {{ 0b10000000 }};

static void attract_init(void) {
	game_fx_set_view(NULL);
	game_fx_init();
	ww_art_set_clip(0);
	if (!_stars_made) {
		for (uint32_t i = 0; i < WW_ATTRACT_STARS; ++i) {
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
	return is_ww_input_play(game_input_pressed());
}

static void draw_stars(uint32_t frame) {
	for (uint32_t i = 0; i < WW_ATTRACT_STARS; ++i) {
		const WwStar_t* s = &_stars[i];
		const uint32_t c = ((s->c + (frame >> 3)) & 15) == 0 ? GAME_WHITE : (s->c & 1) ? GAME_GREY : GAME_NAVY;
		game_spr_add(s->x, s->y, 8, 1, &_tile_star, game_pallet(c), sprite_renderer_sprite_8x8_p1);
	}
}

// The witch walking along with her friends behind her
static void draw_parade(uint32_t frame, int32_t y) {
	static const uint8_t friends[] = { WW_OWLET, WW_BUNNY, WW_HEDGEHOG, WW_FOX, WW_DEER, WW_WOLF };
	const int32_t span = GAME_SCREEN_W + 400;
	const int32_t x = (int32_t)((frame * 3 / 2) % span) - 40;
	ww_witch_draw_at(x, y - WW_WITCH_H - ((frame >> 3) & 1), WW_VIEW_RIGHT, 1 + ((frame >> 4) & 1));
	int32_t fx = x - 8;
	for (uint32_t i = 0; i < sizeof(friends); ++i) {
		const uint32_t t = friends[i];
		fx -= ww_animal_width(t) + 12;
		ww_animal_draw_at(t, WW_MOOD_HAPPY, fx, y - ww_animal_height(t), (frame >> 3) + i, false);
	}
}

bool ww_attract_title(void) {
	attract_init();
	for (uint32_t i = 0; i < WW_ATTRACT_TITLE_LEN; ++i) {
		const char s[2] = { WW_ATTRACT_TITLE[i], 0 };
		game_text_set(&_text[i], s, 4);
	}
	game_text_set(&_text[8], "HAZEL THE WITCH AND THE MOON CHARM", 1);
	game_text_set(&_text[9], "PRESS FIRE TO PLAY", 2);
	game_text_set(&_text[10], "MAGIC FLOWERS", 1);
	for (uint32_t i = 0; i < WW_ATTRACT_ROLL_COUNT; ++i) {
		game_text_set(&_roll_text[i][0], _roll[i].line1, 1);
		game_text_set(&_roll_text[i][1], _roll[i].line2, 1);
	}
	for (uint32_t i = 0; i < WW_EFFECTS; ++i) game_text_set(&_effect_texts[i], _effect_names[i], 1);

	const int32_t tx = (GAME_SCREEN_W - WW_ATTRACT_TITLE_LEN * 32) >> 1;
	const int32_t ax = 120;
	for (uint32_t frame = 0; frame < WW_ATTRACT_TITLE_FRAMES; ++frame) {
		if (attract_frame()) return true;

		for (uint32_t i = 0; i < WW_ATTRACT_ROLL_COUNT; ++i) {
			if (frame == WW_ATTRACT_ROLL_START + i * WW_ATTRACT_ROLL_GAP) {
				const int32_t y = WW_ATTRACT_ROLL_Y + i * WW_ATTRACT_ROLL_H + 20;
				game_fx_implode(GAME_PX(ax), GAME_PX(y), GAME_COLOUR_RANDOM, 32, WW_ATTRACT_MATERIALISE);
			}
		}

		game_spr_begin();
		for (uint32_t i = 0; i < WW_ATTRACT_TITLE_LEN; ++i) {
			const int32_t bob = game_sin((frame << 2) + i * 24) * 5 / 127;
			game_spr_text(&_text[i], tx + i * 32, 30 + bob, game_pallet_rainbow(i + (frame >> 3)));
		}
		game_spr_text_centre(&_text[8], 78, game_pallet(GAME_WHITE));
		// The pieces of the charm light up in turn
		ww_hud_draw_charm(tx - 56, 28, (1u << ((frame >> 5) % (WW_PIECES + 1))) - 1);
		ww_witch_draw_at(tx + WW_ATTRACT_TITLE_LEN * 32 + 20, 22, WW_VIEW_FRONT, (frame >> 5) & 1 ? 0 : 0);

		for (uint32_t i = 0; i < WW_ATTRACT_ROLL_COUNT; ++i) {
			if (frame < WW_ATTRACT_ROLL_START + i * WW_ATTRACT_ROLL_GAP + WW_ATTRACT_MATERIALISE) continue;
			const WwRoll_t* r = &_roll[i];
			const int32_t y = WW_ATTRACT_ROLL_Y + i * WW_ATTRACT_ROLL_H;
			const uint32_t h = ww_animal_height(r->type);
			const bool moving = r->mood != WW_MOOD_ASLEEP && r->mood != WW_MOOD_HURT;
			ww_animal_draw_at(r->type, r->mood, ax - (ww_animal_width(r->type) >> 1), y + 36 - h, moving ? (frame >> 3) : 0, false);
			ww_animal_draw_mood(r->mood, ax, y + 36 - h, frame + i * 16);
			game_spr_text(&_roll_text[i][0], ax + 50, y + 12, game_pallet(i == 4 ? GAME_BLUE : GAME_YELLOW));
			game_spr_text(&_roll_text[i][1], ax + 50, y + 24, game_pallet(GAME_WHITE));
		}

		if (frame >= WW_ATTRACT_ROLL_START + WW_ATTRACT_ROLL_COUNT * WW_ATTRACT_ROLL_GAP) {
			game_spr_text_centre(&_text[10], 392, game_pallet(GAME_GREEN));
			for (uint32_t i = 0; i < WW_EFFECTS; ++i) {
				const int32_t x = 92 + i * 128;
				ww_items_draw_flower(i, x, 406, frame);
				game_spr_text(&_effect_texts[i], x + 20, 412, game_pallet(_effect_text[i]));
			}
		}
		if ((frame >> 5) & 1) game_spr_text_centre(&_text[9], 444, game_pallet(GAME_YELLOW));
		game_fx_draw_particles();
		draw_stars(frame);
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

bool ww_attract_hiscores(int32_t highlight) {
	attract_init();
	char rows[GAME_HISCORE_COUNT][GAME_TEXT_MAX];
	game_text_set(&_text[0], "SWEETEST DREAMERS", 3);
	for (uint32_t i = 0; i < GAME_HISCORE_COUNT; ++i) {
		const GameHiScore_t *e = game_hiscore_get(i);
		snprintf(rows[i], GAME_TEXT_MAX, "%-4s  %07lu  %s", game_hiscore_rank_name(i), (unsigned long)e->score, e->name);
	}
	game_text_set(&_text[11], "PRESS FIRE TO PLAY", 2);

	const uint32_t frames = highlight >= 0 ? WW_ATTRACT_HIGHLIGHT_FRAMES : WW_ATTRACT_HISCORE_FRAMES;
	for (uint32_t frame = 0; frame < frames; ++frame) {
		if (attract_frame()) return true;

		game_spr_begin();
		ww_hud_draw(0, frame, false);
		game_spr_text_centre(&_text[0], 60, game_pallet_rainbow(frame >> 3));
		// Typed out a line at a time
		int32_t n = (int32_t)frame - 30;
		for (uint32_t i = 0; i < GAME_HISCORE_COUNT; ++i) {
			game_text_set_n(&_text[1 + i], rows[i], n < 0 ? 0 : n, 1);
			n -= strlen(rows[i]);
			const uint32_t c = (int32_t)i == highlight && ((frame >> 3) & 1) ? GAME_RED : rank_colour(i);
			game_spr_text_centre(&_text[1 + i], 116 + i * 24, game_pallet(c));
		}
		if (frame > 260 && ((frame >> 5) & 1)) game_spr_text_centre(&_text[11], 370, game_pallet(GAME_YELLOW));
		draw_parade(frame, 456);
		draw_stars(frame);
		game_spr_end();
	}
	return false;
}

bool ww_attract_game_over(void) {
	const uint32_t score = ww_hud_score();
	const int32_t rank = game_hiscore_rank(score);
	if (rank < 0) return false;

	attract_init();
	char t[GAME_TEXT_MAX];
	game_text_set(&_text[0], "NEW HIGH SCORE", 3);
	snprintf(t, sizeof(t), "SCORE %07lu   %s PLACE", (unsigned long)score, game_hiscore_rank_name(rank));
	game_text_set(&_text[1], t, 1);
	game_text_set(&_text[2], "ENTER YOUR INITIALS", 2);
	game_text_set(&_text[6], "LEFT/RIGHT OR UP/DOWN TO CHOOSE", 1);
	game_text_set(&_text[7], "CAST TO ENTER  TIPTOE WHEN DONE", 1);

	GameInitials_t g;
	game_initials_reset(&g);
	for (uint32_t frame = 0; frame < WW_ATTRACT_ENTRY_FRAMES; ++frame) {
		hstx_dvi_sprite_wait_for_frame();

		const uint8_t pressed = game_input_pressed();
		const uint8_t input = game_input_get();
		const int32_t dir = (input & (WW_INP_BIT_L | WW_INP_BIT_D)) ? -1 : (input & (WW_INP_BIT_R | WW_INP_BIT_U)) ? 1 : 0;
		const bool stepped = (pressed & WW_INP_DIRS) != 0;
		if (game_initials_update(&g, dir, stepped, pressed & WW_INP_BIT_F) || (pressed & WW_INP_BIT_B)) break;

		game_spr_begin();
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
		draw_parade(frame, 440);
		draw_stars(frame);
		game_spr_end();
	}

	char name[GAME_HISCORE_NAME_LEN + 1];
	game_initials_name(&g, name);

	return ww_attract_hiscores(game_hiscore_insert(score, name));
}
