// Drives Harmless on the PC, see common/host
//
// attract - nobody touches the buttons
// options - shows the options on the title and dithers, then starts a game
//           and waits; built with EL_HOST_SAVE, the commanders go back to
//           the title after 30 seconds
// shipyard - starts a game, puts every ship on sale in the options, buys an
//           Adder at the shipyard, looks at an Anaconda, then the status
// fly     - starts a game, launches, speeds up, rolls and climbs, looks at
//           the charts moving the cursor, then changes the style and the
//           dashboard in the options, and fits everything
// dock    - starts a game, launches, then is put on a level run at the slot,
//           and docks
// autodock - starts a game, launches, then is put behind the station facing
//           away, rolled, with a docking computer, turned on in the options, and
//           it flies us round and in
// pod     - starts a game, launches, climbs away, then has an escape pod
//           fitted and food in the hold, and runs out of energy, so leaves
//           in the pod and docks, without the food or the pod
// bomb    - starts a game, launches, climbs away, then has an energy bomb
//           fitted and three pirates put round us, presses its button once,
//           which only arms it, then three times quickly, which sets it off
// galactic - starts a game, launches, climbs away, has a galactic hyperdrive
//           fitted, and jumps from the galactic chart into galaxy 2
// witch   - starts a game, launches, climbs away, then a jump to the nearest
//           system goes wrong, into witchspace, among Thargoids, firing at
//           them, then looks at the short range and galactic charts
// dodo    - starts a game, launches, then is moved to a system with a
//           dodecahedron station, behind it, and the docking computer flies in
// crash   - as dock, but rolled so the slot is not level, and hits the station
// hyper   - starts a game, launches, climbs away, picks Diso on the short
//           range chart, then holds on into the overdrive and the hyperdrive,
//           and jumps there
// fight   - starts a game, launches, then a Sidewinder is put ahead, which
//           we shoot down, and then a Python, at which we fire a missile
// target  - starts a game, launches, then has a targeting computer fitted and
//           a Sidewinder put ahead, a little off the middle, which the sights
//           follow and we shoot down, with everything else fitted, then
//           looks at the inventory
// station - starts a game, launches, is put still in front of the slot and
//           shoots the station, which turns on us, launching Vipers and
//           firing its guns
// story   - starts a game, launches with Pip aboard, and the lost kittens
//           start: a basket ahead, scooped, and its page read
// race    - starts a game, launches, and the great race is offered, as if
//           docked; down picks the second choice, and fire takes it
// quill   - starts a game, docked, with the Thargoid going home and nobody
//           aboard, and launches: Quill knocks before the tunnel, and is
//           welcomed aboard, then the Thargoid follows us out
// clear   - built with EL_HOST_SAVE, after save: on the commanders, goes to
//           slot 2's CLEAR, asks, says NO, asks again, and says YES
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
void game_debug_quill(void);
void game_debug_pirate(const char* name);
void game_debug_chaser(const char* name);
void game_debug_scoop(void);
void game_debug_face_sun(void);
void game_debug_behind(void);
void game_debug_pod(void);
void game_debug_bomb(void);
void game_debug_witch(void);
void game_debug_dodo(void);
void game_debug_galactic(void);
void game_debug_target(void);
void game_debug_station(void);
void game_debug_story(unsigned i, _Bool docked);
void game_debug_press(uint16_t buttons);

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
	if (!strcmp(host_mode, "quill")) {
		// Into a game, then launch, and take Quill aboard
		if (f >= 30 && f < 33) return A;
		if (f == 60) game_debug_quill();
		if (f >= 90 && f < 93) return A;
		if (f >= 150 && f < 153) return A;
		return 0;
	}
	// Into a game, and launch
	if (f >= 30 && f < 33) return A;
	if (!strcmp(host_mode, "autodock") || !strcmp(host_mode, "dodo")) {
		// The options, down to docking, as the PC has no X
		if (f >= 220 && f < 223) return START;
		if (f >= 226 && f < 256 && (f - 226) % 6 < 3) return D;
		if (f >= 258 && f < 261) return A;
	}
	if (!strcmp(host_mode, "pod") && f >= 100 && f < 175) return D;
	if (!strcmp(host_mode, "bomb") && f >= 100 && f < 175) return D;
	if (!strcmp(host_mode, "galactic")) {
		// Climb away and speed up, as in hyper, then the galactic chart
		if (f >= 160 && f < 235) return D;
		if (f >= 240 && f < 310) return B | U;
		if (f >= 320 && f < 344 && (f - 320) % 6 < 3) return SELECT;
		if (f >= 700 && f < 703) return B | A;
		// Arrived, the galactic chart again
		if (f >= 1500 && f < 1524 && (f - 1500) % 6 < 3) return SELECT;
	}
	if (!strcmp(host_mode, "witch")) {
		if (f >= 100 && f < 175) return D;
		if (f >= 400 && f < 800) return A;
		// Round to the short range chart, then the galactic one
		if (f >= 900 && f < 930 && (f - 900) % 10 < 3) return SELECT;
		if (f >= 1000 && f < 1003) return SELECT;
	}
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
		// Round the screens to the status
		if (f >= 130 && f < 210 && (f - 130) % 10 < 3) return SELECT;
		if (f >= 215 && f < 218) return A;
		// The dashboard filled, saved in the settings
		if (f >= 230 && f < 233) return START;
		if (f >= 240 && f < 243) return D;
		if (f >= 250 && f < 253) return D;
		if (f >= 260 && f < 263) return R;
		if (f >= 270 && f < 273) return START;
		return 0;
	}
	if (!strcmp(host_mode, "clear")) {
		if (f >= 50 && f < 53) return D;
		if (f >= 60 && f < 63) return R;
		if (f >= 70 && f < 73) return A;
		if (f >= 80 && f < 83) return A;
		if (f >= 90 && f < 93) return A;
		if (f >= 100 && f < 103) return R;
		if (f >= 110 && f < 113) return A;
		return 0;
	}
	if (!strcmp(host_mode, "shipyard")) {
		// Every ship for sale, from the options, then the shipyard: buy an
		// Adder, look at the Anaconda, and round to the status
		if (f >= 60 && f < 63) return START;
		if (f >= 66 && f < 108 && (f - 66) % 6 < 3) return D;
		if (f >= 110 && f < 113) return A;
		if (f >= 120 && f < 123) return START;
		if (f >= 140 && f < 170 && (f - 140) % 10 < 3) return SELECT;
		if (f >= 180 && f < 183) return D;
		if (f >= 240 && f < 243) return A;
		if (f >= 250 && f < 253) return A;
		if (f >= 280 && f < 334 && (f - 280) % 6 < 3) return D;
		if (f >= 420 && f < 480 && (f - 420) % 10 < 3) return SELECT;
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
		// Past the passenger cabin
		if (f >= 485 && f < 488) return D;
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
		// Round the inventory and the log to the charts, moving the cursor
		if (f >= 600 && f < 618 && (f - 600) % 6 < 3) return SELECT;
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
		// Everything fitted
		if (f >= 1184 && f < 1187) return D;
		if (f >= 1188 && f < 1191) return D;
		if (f >= 1192 && f < 1195) return D;
		if (f >= 1196 && f < 1199) return D;
		if (f >= 1200 && f < 1203) return A;
		if (f >= 1208 && f < 1211) return START;
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
	if (!strcmp(host_mode, "race")) {
		if (f >= 250 && f < 253) return D;
		if (f >= 270 && f < 273) return A;
	}
	if (!strcmp(host_mode, "story")) {
		// Read the page, then go on, and round to the ship's log
		if (f >= 700 && f < 703) return A;
		if (f >= 760 && f < 780 && (f - 760) % 10 < 3) return SELECT;
	}
	if (!strcmp(host_mode, "station")) {
		if (f >= 210 && f < 225) return A;
	}
	if (!strcmp(host_mode, "target")) {
		// Slow down, then shoot
		if (f >= 160 && f < 180) return B | D;
		if (f >= 202 && f < 260) return A;
		if (f >= 300 && f < 303) return SELECT;
	}
	if (!strcmp(host_mode, "views")) {
		// Climb a quarter turn, so the station is not behind, slow down, look
		// back and shoot, then left and right, and round to the front
		if (f >= 100 && f < 175) return D;
		if (f >= 180 && f < 200) return B | D;
		if (f >= 260 && f < 460) return A;
		if (f >= 500 && f < 530) return R;
		if (f >= 580 && f < 610) return B | U;
	}
	if (!strcmp(host_mode, "overdrive")) {
		// Climb away from the planet, then full speed and on
		if (f >= 100 && f < 175) return D;
		if (f >= 190 && f < 1600) return B | U;
	}
	if (!strcmp(host_mode, "scoop")) {
		// Look at the inventory, then round the charts back to the front
		if (f >= 450 && f < 510 && (f < 452 || f >= 500) && !(f & 1)) return SELECT;
		// Dive into the sun, below us
		if (f >= 600 && f < 700) return U;
		if (f >= 700 && f < 900) return B | U;
	}
	if (!strcmp(host_mode, "hyper")) {
		// Climb a quarter turn, away from both the planet and the station,
		// and speed up, then pick Diso, 9 steps left of Lave, and back to
		// the front; then on into the overdrive and the hyperdrive
		if (f >= 160 && f < 235) return D;
		if (f >= 240 && f < 310) return B | U;
		// Round the inventory and the log to the short range chart
		if (f >= 320 && f < 338 && (f - 320) % 6 < 3) return SELECT;
		if (f >= 360 && f < 383) return L;
		// and on round the galactic chart and the data to the front
		if (f >= 400 && f < 418 && (f - 400) % 6 < 3) return SELECT;
		if (f >= 440 && f < 800) return B | U;
	}
	return 0;
}

void host_frame(uint32_t frame, uint32_t sprites) {
	if (frame == 200 && !strcmp(host_mode, "dock")) game_debug_approach(0.0f);
	if (frame == 200 && !strcmp(host_mode, "autodock")) game_debug_behind();
	if (frame == 200 && !strcmp(host_mode, "dodo")) game_debug_dodo();
	if (frame == 200 && !strcmp(host_mode, "galactic")) game_debug_galactic();
	if (!strcmp(host_mode, "bomb")) {
		// The bomb's button is a SNES pad's A, which the PC does not have
		if (frame == 500) game_debug_bomb();
		if (frame == 505 || frame == 540 || frame == 546 || frame == 552) game_debug_press(2048);
	}
	if (frame == 300 && !strcmp(host_mode, "witch")) game_debug_witch();
	if (frame == 600 && !strcmp(host_mode, "pod")) game_debug_pod();
	if (frame == 200 && !strcmp(host_mode, "crash")) game_debug_approach(0.9f);
	if (frame == 200 && !strcmp(host_mode, "fight")) game_debug_pirate("SIDEWINDER");
	if (frame == 480 && !strcmp(host_mode, "fight")) game_debug_pirate("PYTHON");
	if (frame == 200 && !strcmp(host_mode, "target")) game_debug_target();
	if (frame == 200 && !strcmp(host_mode, "station")) game_debug_station();
	if (frame == 200 && !strcmp(host_mode, "story")) game_debug_story(0, false);
	if (frame == 200 && !strcmp(host_mode, "race")) game_debug_story(2, true);
	if (frame == 120 && !strcmp(host_mode, "scoop")) game_debug_scoop();
	if (frame == 180 && !strcmp(host_mode, "overdrive")) game_debug_face_sun();
	if (frame == 240 && !strcmp(host_mode, "views")) game_debug_chaser("SIDEWINDER");
	if (!strcmp(host_mode, "views")) {
		// The shoulders, which the PC does not have: round to the rear,
		// then left, right, and back to the front
		if (frame == 245 || frame == 249 || frame == 480 || frame == 560 || frame == 564) game_debug_press(512);
		if (frame == 660) game_debug_press(256);
	}
	if (frame % 600 == 0) {
		printf("%6u sprites %3u\n", frame, sprites);
	}
}
