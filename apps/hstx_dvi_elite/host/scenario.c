// Drives the Elite game on the PC, see common/host
//
// attract - nobody touches the buttons
// options - shows the options on the title and dithers, then starts a game
//           and waits; built with EL_HOST_SAVE, the commanders go back to
//           the title after 30 seconds
// fly     - starts a game, launches, speeds up, rolls and climbs, looks at
//           the charts moving the cursor, then changes the style and the
//           dashboard in the options
// dock    - starts a game, launches, then is put on a level run at the slot,
//           and docks
// crash   - as dock, but rolled so the slot is not level, and hits the station
// hyper   - starts a game, launches, climbs away, tries to jump while still
//           too close to the station, then picks Diso on the short range
//           chart and jumps there once clear, then tries Lave's far
//           neighbours, out of range
// fight   - starts a game, launches, then a Sidewinder is put ahead, which
//           we shoot down, and then a Python, at which we fire a missile
// trade   - starts a game, and docked, buys food, holding to buy more, and
//           computers, then a missile, and sells some food, then tries for
//           a rear pulse laser
// views   - starts a game, launches and climbs, then a Sidewinder is put
//           behind and a rear laser fitted, and we look back and shoot at it,
//           then look left and right
// scoop   - starts a game, launches, then has fuel scoops fitted and an
//           empty tank, and is put beside the sun, which fills it, with a
//           canister ahead, a little above the nose, that it scoops up, looks
//           at the inventory, then dives into the sun
// overdrive - starts a game, launches, climbs away, is turned to face the
//           sun, and holds B and up, into the overdrive once clear of the
//           station, until mass locked by the sun
// save    - built with EL_HOST_SAVE, looks at the options on the commanders,
//           picks commander slot 2, names it NAMESON,
//           holding up to go on changing the letter, buys food and launches,
//           which saves, then fills the dashboard in the options, saving
//           the settings; run again, the slot holds it, and the dashboard
//           is filled
#include <stdio.h>
#include <string.h>
#include "host.h"

void game_debug_approach(float roll);
void game_debug_pirate(const char* name);
void game_debug_chaser(const char* name);
void game_debug_scoop(void);
void game_debug_face_sun(void);

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
	if (!strcmp(host_mode, "attract")) return 0;
	if (!strcmp(host_mode, "options")) {
		// The options on the title: dithered, then into a game, and wait
		if (f >= 30 && f < 33) return START;
		if (f >= 40 && f < 43) return D;
		if (f >= 50 && f < 53) return R;
		if (f >= 70 && f < 73) return START;
		if (f >= 100 && f < 103) return A;
		return 0;
	}
	// Into a game, and launch
	if (f >= 30 && f < 33) return A;
	if (!strcmp(host_mode, "save")) {
		// The options and out again, then slot 2, named NAMESON, then food in
		// the market, and launch from the status
		if (f >= 40 && f < 43) return START;
		if (f >= 46 && f < 49) return START;
		if (f >= 50 && f < 53) return D;
		if (f >= 60 && f < 63) return A;
		if (f >= 66 && f < 94) return U;
		if (f >= 96 && f < 99) return A;
		if (f >= 104 && f < 107) return SELECT;
		if (f >= 112 && f < 115) return R;
		if (f >= 120 && f < 123) return R;
		if (f >= 130 && f < 133) return SELECT;
		if (f >= 133 && f < 136) return 0;
		if (f >= 140 && f < 143) return SELECT;
		if (f >= 143 && f < 146) return 0;
		if (f >= 150 && f < 153) return SELECT;
		if (f >= 153 && f < 156) return 0;
		if (f >= 160 && f < 163) return SELECT;
		if (f >= 163 && f < 166) return 0;
		if (f >= 170 && f < 173) return SELECT;
		if (f >= 200 && f < 203) return A;
		// The dashboard filled, saved in the settings
		if (f >= 230 && f < 233) return START;
		if (f >= 240 && f < 243) return D;
		if (f >= 250 && f < 253) return D;
		if (f >= 260 && f < 263) return R;
		if (f >= 270 && f < 273) return START;
		return 0;
	}
	if (!strcmp(host_mode, "trade")) {
		// The market: food, held to buy more, then computers
		if (f >= 60 && f < 63) return SELECT;
		if (f >= 80 && f < 83) return R;
		if (f >= 100 && f < 140) return R;
		if (f >= 160 && f < 163) return D;
		if (f >= 170 && f < 173) return D;
		if (f >= 180 && f < 183) return D;
		if (f >= 190 && f < 193) return D;
		if (f >= 200 && f < 203) return D;
		if (f >= 210 && f < 213) return D;
		if (f >= 220 && f < 223) return D;
		if (f >= 240 && f < 243) return R;
		// Sell some food
		if (f >= 260 && f < 263) return U;
		if (f >= 263 && f < 266) return 0;
		if (f >= 270 && f < 273) return U;
		if (f >= 280 && f < 283) return U;
		if (f >= 290 && f < 293) return U;
		if (f >= 300 && f < 303) return U;
		if (f >= 310 && f < 313) return U;
		if (f >= 320 && f < 323) return U;
		if (f >= 340 && f < 343) return L;
		// The equipment: a missile
		if (f >= 400 && f < 403) return SELECT;
		if (f >= 420 && f < 423) return D;
		if (f >= 440 && f < 443) return A;
		// A pulse laser for the rear, which costs too much
		if (f >= 460 && f < 463) return D;
		if (f >= 470 && f < 473) return D;
		if (f >= 480 && f < 483) return D;
		if (f >= 490 && f < 493) return R;
		if (f >= 500 && f < 503) return A;
		return 0;
	}
	if (f >= 60 && f < 63) return A;
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
		// The options: filled and dithered, with the dashboard filled
		if (f >= 1100 && f < 1103) return START;
		if (f >= 1120 && f < 1123) return R;
		if (f >= 1130 && f < 1133) return R;
		if (f >= 1140 && f < 1143) return R;
		if (f >= 1150 && f < 1153) return D;
		if (f >= 1160 && f < 1163) return R;
		if (f >= 1170 && f < 1173) return D;
		if (f >= 1180 && f < 1183) return R;
		if (f >= 1200 && f < 1203) return START;
		// Dive
		if (f >= 1300 && f < 1360) return U;
	}
	if (!strcmp(host_mode, "fight")) {
		// Slow down, then shoot
		if (f >= 160 && f < 180) return B | D;
		if (f >= 205 && f < 400) return A;
		if (f >= 500 && f < 510) return B;
		if (f >= 510 && f < 513) return B | A;
	}
	if (!strcmp(host_mode, "views")) {
		// Climb a quarter turn, so the station is not behind, slow down, look
		// back and shoot, then left and right, and round to the front
		if (f >= 100 && f < 175) return D;
		if (f >= 180 && f < 200) return B | D;
		if (f >= 245 && f < 248) return SELECT;
		if (f >= 260 && f < 460) return A;
		if (f >= 480 && f < 483) return SELECT;
		if (f >= 500 && f < 530) return R;
		if (f >= 560 && f < 563) return SELECT;
		if (f >= 580 && f < 610) return B | U;
		if (f >= 660 && f < 663) return SELECT;
	}
	if (!strcmp(host_mode, "overdrive")) {
		// Climb away from the planet, then full speed and on
		if (f >= 100 && f < 175) return D;
		if (f >= 190 && f < 1600) return B | U;
	}
	if (!strcmp(host_mode, "scoop")) {
		// Look at the inventory, then round the charts back to the front
		if (f >= 450 && f < 508 && (f < 458 || f >= 500) && !(f & 1)) return SELECT;
		// Dive into the sun, below us
		if (f >= 600 && f < 700) return U;
		if (f >= 700 && f < 900) return B | U;
	}
	if (!strcmp(host_mode, "hyper")) {
		// Climb a quarter turn, away from both the planet and the station,
		// and speed up, trying to jump too soon on the way, then go to Diso,
		// 9 steps left of Lave, once clear
		if (f >= 160 && f < 235) return D;
		if (f >= 240 && f < 310) return B | U;
		if (f >= 320 && f < 323) return SELECT;
		if (f >= 330 && f < 370) return L;
		if (f >= 380 && f < 383) return A;
		if (f >= 700 && f < 703) return A;
		// Then too far
		if (f >= 1600 && f < 1603) return SELECT;
		if (f >= 1620 && f < 1640) return R | B;
		if (f >= 1700 && f < 1703) return A;
	}
	return 0;
}

void host_frame(uint32_t frame, uint32_t sprites) {
	if (frame == 200 && !strcmp(host_mode, "dock")) game_debug_approach(0.0f);
	if (frame == 200 && !strcmp(host_mode, "crash")) game_debug_approach(0.9f);
	if (frame == 200 && !strcmp(host_mode, "fight")) game_debug_pirate("SIDEWINDER");
	if (frame == 480 && !strcmp(host_mode, "fight")) game_debug_pirate("PYTHON");
	if (frame == 120 && !strcmp(host_mode, "scoop")) game_debug_scoop();
	if (frame == 180 && !strcmp(host_mode, "overdrive")) game_debug_face_sun();
	if (frame == 240 && !strcmp(host_mode, "views")) game_debug_chaser("SIDEWINDER");
	if (frame % 600 == 0) {
		printf("%6u sprites %3u\n", frame, sprites);
	}
}
