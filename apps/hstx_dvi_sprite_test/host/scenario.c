// Drives the invaders app on the PC, see common/host
//
// attract - nobody touches the buttons
// play    - starts a game and lets the demo player drive it
// idle    - starts a game and leaves the gun where it is
// settings - opens the settings on the title, plays a sound, turns the music
//            off and goes back
#include <stdio.h>
#include <string.h>
#include "host.h"

// From the game, to drive and report on it
uint8_t inv_demo_input(uint32_t frame);
uint32_t inv_score_get(void);
uint32_t inv_gun_get_lives(void);
uint32_t inv_invaders_alive(void);
uint32_t inv_level_get(void);

// Buttons as the game sees them: L R U D F S from bit 0
uint32_t host_input(uint32_t f) {
	if (!strcmp(host_mode, "play")) {
		// Start a game from the title, then let the demo player drive
		if (f > 100 && f < 104) return 16;
		if (f > 120) return inv_demo_input(f);
	}
	if (!strcmp(host_mode, "idle")) {
		if (f > 100 && f < 104) return 16;
	}
	if (!strcmp(host_mode, "settings")) {
		// One frame each: start, down to the sounds, right twice, fire, up to the music, right, start
		static const uint16_t at[][2] = {
			{ 100, 32 }, { 160, 8 }, { 200, 2 }, { 220, 2 }, { 240, 16 }, { 330, 4 }, { 360, 2 }, { 420, 32 },
		};
		for (uint32_t i = 0; i < sizeof(at) / sizeof(at[0]); ++i) if (f == at[i][0]) return at[i][1];
	}
	return 0;
}

void host_frame(uint32_t frame, uint32_t sprites) {
	if (frame % 600 == 0) {
		printf("%6u score %6u lives %u wave %2u invaders %2u sprites %3u\n",
			frame, inv_score_get(), inv_gun_get_lives(), inv_level_get(), inv_invaders_alive(), sprites);
	}
}
