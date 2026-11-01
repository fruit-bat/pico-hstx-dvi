// Drives the Elite game on the PC, see common/host
//
// attract - nobody touches the buttons
// fly     - starts a game, launches, speeds up, rolls and climbs, looks at
//           the charts moving the cursor, then changes the style in the options
// dock    - starts a game, launches, then is put on a level run at the slot,
//           and docks
// crash   - as dock, but rolled so the slot is not level, and hits the station
// hyper   - starts a game, launches, picks Diso on the short range chart and
//           jumps there, then tries Lave's far neighbours, out of range
#include <stdio.h>
#include <string.h>
#include "host.h"

void game_debug_approach(float roll);

// The buttons, as in el_input.h
#define L 1
#define R 2
#define U 4
#define D 8
#define A 16
#define B 32
#define SELECT 64
#define START 128

uint32_t host_input(uint32_t f) {
	// Into a game, and launch
	if (f >= 30 && f < 33) return START;
	if (strcmp(host_mode, "attract") && f >= 60 && f < 63) return A;
	if (!strcmp(host_mode, "fly")) {
		// Speed up, then roll right and climb
		if (f >= 200 && f < 300) return B | U;
		if (f >= 340 && f < 400) return R;
		if (f >= 440 && f < 520) return D;
		// The charts, moving the cursor
		if (f >= 600 && f < 603) return SELECT;
		if (f >= 620 && f < 700) return R | D;
		if (f >= 760 && f < 763) return SELECT;
		if (f >= 780 && f < 860) return L;
		if (f >= 900 && f < 903) return SELECT;
		if (f >= 1000 && f < 1003) return SELECT;
		// The options: filled and dithered
		if (f >= 1100 && f < 1103) return START;
		if (f >= 1120 && f < 1123) return R;
		if (f >= 1130 && f < 1133) return R;
		if (f >= 1140 && f < 1143) return R;
		if (f >= 1150 && f < 1153) return D;
		if (f >= 1160 && f < 1163) return R;
		if (f >= 1200 && f < 1203) return START;
		// Dive
		if (f >= 1300 && f < 1360) return U;
	}
	if (!strcmp(host_mode, "hyper")) {
		// Diso is 9 steps left of Lave
		if (f >= 200 && f < 203) return SELECT;
		if (f >= 220 && f < 260) return L;
		if (f >= 300 && f < 303) return A;
		// Too far
		if (f >= 1300 && f < 1303) return SELECT;
		if (f >= 1320 && f < 1340) return R | B;
		if (f >= 1400 && f < 1403) return A;
	}
	return 0;
}

void host_frame(uint32_t frame, uint32_t sprites) {
	if (frame == 200 && !strcmp(host_mode, "dock")) game_debug_approach(0.0f);
	if (frame == 200 && !strcmp(host_mode, "crash")) game_debug_approach(0.9f);
	if (frame % 600 == 0) {
		printf("%6u sprites %3u\n", frame, sprites);
	}
}
