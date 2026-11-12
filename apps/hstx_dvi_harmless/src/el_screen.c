// Copyright (c) 2024 fruit-bat.

// The meter is shown when built with HSTX_DVI_SPRITE_TIMING, and printed
// over USB serial each second. See the wireframe test for what it shows.

#include "el_screen.h"
#include "hstx_dvi_row_fifo.h"
#include "el_save.h"
#include "el_sound.h"
#include "game_pallet.h"
#include <stdio.h>

// The system clock runs at the HSTX clock, 10 times the pixel clock
#define LINE_CYCLES ((MODE_H_FRONT_PORCH + MODE_H_SYNC_WIDTH + MODE_H_BACK_PORCH + MODE_H_ACTIVE_PIXELS) * 10)
#define FRAME_LINES (MODE_V_FRONT_PORCH + MODE_V_SYNC_WIDTH + MODE_V_BACK_PORCH + MODE_V_ACTIVE_LINES)

#define TEXTS 60

static GameText_t _title[2];
static GameText_t _texts[2][TEXTS];
static uint32_t _text_count = 0;
static GameText_t _status[2];
static uint32_t _frame = 0;

static uint32_t percent(uint32_t cycles, uint32_t lines) {
	return (cycles * 100u + lines * LINE_CYCLES / 2) / (lines * LINE_CYCLES);
}

// Cycles as rows of the display, to one place, such as "5.2" or "-0.3"
static void rows_text(char* s, size_t n, int32_t cycles) {
	const int32_t tenths = cycles * 10 / LINE_CYCLES;
	const int32_t a = tenths < 0 ? -tenths : tenths;
	snprintf(s, n, "%s%d.%d", tenths < 0 ? "-" : "", (int)(a / 10), (int)(a % 10));
}

void screen_init(void) {
	for (uint32_t i = 0; i < 2; ++i) {
		game_text_set_smooth(&_title[i], "");
		game_text_set(&_status[i], "", 1);
	}
}

void screen_begin(void) {
	draw_frame_begin();
	_text_count = 0;
}

static GameText_t* next_text(const char* s, bool big) {
	if (_text_count >= TEXTS) return NULL;
	GameText_t* t = &_texts[draw_set()][_text_count++];
	if (big) game_text_set_smooth(t, s);
	else game_text_set(t, s, 1);
	draw_count_sprite();
	return t;
}

int32_t screen_text(int32_t x, int32_t y, const char* s, uint32_t colour, bool big) {
	GameText_t* t = next_text(s, big);
	if (!t) return 0;
	game_spr_text(t, x, y, game_pallet(colour));
	return game_text_width(t);
}

int32_t screen_text_centre(int32_t y, const char* s, uint32_t colour, bool big) {
	GameText_t* t = next_text(s, big);
	if (!t) return 0;
	game_spr_text_centre(t, y, game_pallet(colour));
	return game_text_width(t);
}

int32_t screen_text_centre_on(int32_t x, int32_t y, const char* s, uint32_t colour, bool big) {
	GameText_t* t = next_text(s, big);
	if (!t) return 0;
	const int32_t w = (int32_t)game_text_width(t);
	int32_t left = x - w / 2;
	if (left > GAME_SCREEN_W - w) left = GAME_SCREEN_W - w;
	if (left < 0) left = 0;
	game_spr_text(t, left, y, game_pallet(colour));
	return w;
}

void screen_select(int32_t x, int32_t y, int32_t w) {
	if (!object_begin()) return;
	const float x0 = (float)x, x1 = (float)(x + w), y0 = (float)y;
	const SpriteColour_t arrow = sprite_colour(hstx_dvi_pixel_rgb(0, 219, 255));
	const Vec2_t left[3] = { { x0 - 20.0f, y0 - 1.0f }, { x0 - 11.0f, y0 + 4.0f }, { x0 - 20.0f, y0 + 9.0f } };
	const Vec2_t right[3] = { { x1 + 20.0f, y0 - 1.0f }, { x1 + 20.0f, y0 + 9.0f }, { x1 + 11.0f, y0 + 4.0f } };
	add_fill(left, 3, arrow);
	add_fill(right, 3, arrow);
	add_rect(x0 - 6.0f, y0 - 3.0f, x1 + 6.0f, y0 + 11.0f, hstx_dvi_pixel_rgb(0, 36, 146));
	object_end();
}

void screen_end(const char* title, const char* label) {
	const uint32_t set = draw_set();
	game_text_set_smooth(&_title[set], title);
	game_spr_text_centre(&_title[set], 16, game_pallet(GAME_YELLOW));
	draw_count_sprite();

#if defined(HSTX_DVI_SPRITE_TIMING)
	// How hard the renderer worked on the last frame it drew
	const HstxDviSpriteTiming_t timing = hstx_dvi_sprite_timing;
	if (timing.frame) {
		char ahead[16];
		rows_text(ahead, sizeof(ahead), timing.ahead_min);
		char s[GAME_TEXT_MAX + 1];
		snprintf(s, sizeof(s), "%s AHEAD %s AVG %u%%%s", label, ahead, (unsigned)percent(timing.frame, FRAME_LINES),
			save_card() ? " SD" : "");
		game_text_set(&_status[set], s, 1);
		game_spr_text_centre(&_status[set], 4, game_pallet(GAME_CYAN));
		draw_count_sprite();
		if (_frame % 60 == 0) {
			const DrawStats_t st = draw_stats();
			printf("%s parts %u sprites %u hidden %u calls %u ahead %s line %u%% load %u%% avg %u%%\n", label,
				(unsigned)st.parts, (unsigned)st.sprites, (unsigned)st.hidden, (unsigned)timing.calls_max, ahead,
				(unsigned)percent(timing.line_max, 1),
				(unsigned)percent(timing.window_max, HSTX_DVI_ROW_FIFO_SIZE),
				(unsigned)percent(timing.frame, FRAME_LINES));
		}
	}
#else
	(void)label;
#endif
	++_frame;
	sound_frame();
	draw_frame_end();
	hstx_dvi_sprite_wait_for_frame();
}
