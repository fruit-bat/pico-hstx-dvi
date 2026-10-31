// Drives the manor app on the PC, see common/host
//
// attract - nobody touches the buttons
// play    - starts a game and lets the demo player drive it
// idle    - starts a game and stands still
// tour    - starts a game and moves Hazel round the house, for looking at it
// script  - starts a game with Hazel on tile MM_START ("x,y") and presses the
//           buttons in MM_SCRIPT, such as "R30 RJ12 .20 F1", each letters
//           L R U D F J (or . for none) held for a number of frames.
//           MM_GIVE can have "float" and "key4" in it. MM_TRACE=1 prints
//           where Hazel is every frame.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "host.h"

// From the game, to drive and report on it
uint8_t mm_demo_input(uint32_t frame);
uint32_t mm_hud_score(void);
uint32_t mm_hud_lives(void);
uint32_t mm_witch_hearts(void);
int32_t mm_witch_x(void);
int32_t mm_witch_y(void);
void mm_witch_reset(int32_t cx, int32_t cy);
void mm_camera_reset(int32_t x, int32_t y);
uint32_t mm_thing_kittens_found(void);
uint32_t mm_game_night(void);
uint32_t mm_demo_step(void);
void mm_witch_learn_float(void);
void mm_witch_add_key(uint32_t group);
bool mm_witch_grounded(void);

#define SCRIPT_START 130
#define SCRIPT_STEPS 256

static uint8_t _script_buttons[SCRIPT_STEPS];
static uint32_t _script_frames[SCRIPT_STEPS];
static uint32_t _script_steps = 0;

static void script_parse(void) {
	const char* s = getenv("MM_SCRIPT");
	if (!s) return;
	while (*s && _script_steps < SCRIPT_STEPS) {
		while (*s == ' ') ++s;
		uint8_t b = 0;
		for (; *s && strchr("LRUDFJ.", *s); ++s) {
			const char* k = strchr("LRUDFJ", *s);
			if (k) b |= 1 << (k - "LRUDFJ");
		}
		const uint32_t n = strtoul(s, (char**)&s, 10);
		_script_buttons[_script_steps] = b;
		_script_frames[_script_steps] = n ? n : 1;
		++_script_steps;
	}
}

static uint32_t script_input(uint32_t f) {
	uint32_t t = f - SCRIPT_START;
	for (uint32_t i = 0; i < _script_steps; ++i) {
		if (t < _script_frames[i]) return _script_buttons[i];
		t -= _script_frames[i];
	}
	return 0;
}

// Tiles to stand Hazel on in the tour
static const int _tour[][2] = {
	{ 4, 34 }, { 20, 34 }, { 38, 34 }, { 50, 34 }, { 62, 34 }, { 80, 34 },
	{ 30, 43 }, { 60, 43 }, { 80, 43 },
	{ 25, 23 }, { 42, 23 }, { 56, 23 }, { 67, 23 }, { 82, 23 },
	{ 80, 12 }, { 62, 12 }, { 40, 12 }, { 24, 12 },
};
#define TOUR_STOPS (sizeof(_tour) / sizeof(_tour[0]))
#define TOUR_GAP 60

static uint32_t _input = 0;

static uint32_t input(uint32_t f) {
	if (f > 100 && f < 104 && strcmp(host_mode, "attract")) return 32;
	if (!strcmp(host_mode, "play") && f > 120) return mm_demo_input(f);
	if (!strcmp(host_mode, "script") && f >= SCRIPT_START) return script_input(f);
	return 0;
}

// Buttons as the game sees them: L R U D F J from bit 0
uint32_t host_input(uint32_t f) {
	_input = input(f);
	return _input;
}

void host_frame(uint32_t frame, uint32_t sprites) {
	if (!strcmp(host_mode, "script")) {
		if (frame == SCRIPT_START - 1) {
			script_parse();
			const char* start = getenv("MM_START");
			int x = 4, y = 34;
			if (start) sscanf(start, "%d,%d", &x, &y);
			mm_witch_reset(x, y);
			mm_camera_reset(mm_witch_x(), mm_witch_y());
			const char* give = getenv("MM_GIVE");
			if (give && strstr(give, "float")) mm_witch_learn_float();
			for (const char* k = give; k && (k = strstr(k, "key")); k += 3) mm_witch_add_key(atoi(k + 3));
		}
	}
	if (getenv("MM_TRACE") && frame >= SCRIPT_START) {
		{
			printf("%5u in %02x hearts %u score %6u at %4d,%4d tile %2d,%2d %s\n", frame, _input, mm_witch_hearts(), mm_hud_score(), mm_witch_x(), mm_witch_y(),
				mm_witch_x() / 32, mm_witch_y() / 32 - 1, mm_witch_grounded() ? "ground" : "");
		}
	}
	if (!strcmp(host_mode, "tour") && frame >= 200 && (frame - 200) % TOUR_GAP == 0) {
		const uint32_t i = (frame - 200) / TOUR_GAP;
		if (i < TOUR_STOPS) {
			mm_witch_reset(_tour[i][0], _tour[i][1]);
			mm_camera_reset(mm_witch_x(), mm_witch_y());
		}
	}
	static uint32_t step = 0;
	if (!strcmp(host_mode, "play") && mm_demo_step() != step) {
		step = mm_demo_step();
		printf("%6u step %2u at tile %2d,%2d hearts %u kittens %u\n", frame, step, mm_witch_x() / 32, mm_witch_y() / 32 - 1,
			mm_witch_hearts(), mm_thing_kittens_found());
	}
	if (frame % 600 == 0) {
		printf("%6u score %6u lives %u hearts %u night %u kittens %u at %4d,%4d sprites %3u\n",
			frame, mm_hud_score(), mm_hud_lives(), mm_witch_hearts(), mm_game_night(),
			mm_thing_kittens_found(), mm_witch_x(), mm_witch_y(), sprites);
	}
}
