// Drives the defender app on the PC, see common/host
//
// attract - nobody touches the buttons
// play    - starts a game and lets the demo player drive it
// idle    - starts a game and sits at the top of the screen
// planet  - as play, but all the humanoids are killed at frame 900
// manual  - starts a game and flies about a bit
#include <stdio.h>
#include <string.h>
#include "host.h"

// From the game, to drive and report on it
uint8_t def_demo_input(uint32_t frame);
uint32_t def_hud_score(void);
uint32_t def_hud_lives(void);
uint32_t def_hud_smart_bombs(void);
uint32_t def_enemy_count_hostile(void);
uint32_t def_human_alive(void);
bool def_world_planet_alive(void);
void def_human_kill(int32_t i);
int32_t def_player_y(void);

// Times the ship has changed between going up and down from one frame to the next
static uint32_t _reversals = 0;
static int32_t _last_y = 0;
static int32_t _last_dy = 0;

// Buttons as the game sees them: L R U D F B from bit 0
uint32_t host_input(uint32_t f) {
	if (!strcmp(host_mode, "play") || !strcmp(host_mode, "planet")) {
		// Start a game from the title, then let the demo player drive
		if (f > 100 && f < 104) return 16;
		if (f > 120) return def_demo_input(f);
	}
	if (!strcmp(host_mode, "idle")) {
		if (f > 100 && f < 104) return 16;
		// Sit at the top out of the way
		if (f > 120) return 4;
	}
	if (!strcmp(host_mode, "manual")) {
		if (f > 100 && f < 104) return 16;
		// Fly right firing, then turn round
		if (f > 300 && f < 500) return 2 | ((f & 8) ? 16 : 0);
		if (f >= 500 && f < 540) return 1;
		if (f == 600) return 32;
	}
	return 0;
}

void host_frame(uint32_t frame, uint32_t sprites) {
	if (!strcmp(host_mode, "planet") && frame == 900) {
		for (int i = 0; i < 10; ++i) def_human_kill(i);
	}
	const int32_t y = def_player_y();
	const int32_t dy = y - _last_y;
	if (dy && _last_dy && (dy > 0) != (_last_dy > 0)) ++_reversals;
	if (dy) _last_dy = dy;
	_last_y = y;
	if (frame % 600 == 0) {
		printf("%6u score %6u lives %u smart %u hostile %2u humans %2u planet %d sprites %3u reversals %u\n",
			frame, def_hud_score(), def_hud_lives(), def_hud_smart_bombs(), def_enemy_count_hostile(),
			def_human_alive(), def_world_planet_alive(), sprites, _reversals);
		_reversals = 0;
	}
}
