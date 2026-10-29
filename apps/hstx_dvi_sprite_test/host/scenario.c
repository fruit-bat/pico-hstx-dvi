// Drives the invaders app on the PC, see common/host
//
// attract - nobody touches the buttons
// play    - starts a game and lets the demo player drive it
// idle    - starts a game and leaves the gun where it is
#include <stdio.h>
#include <string.h>
#include "host.h"

// From the game, to drive and report on it
uint8_t inv_demo_input(uint32_t frame);
uint32_t inv_score_get(void);
uint32_t inv_gun_get_lives(void);
uint32_t inv_invaders_alive(void);
uint32_t inv_level_get(void);

// Buttons as the game sees them: L R F S from bit 0
uint32_t host_input(uint32_t f) {
	if (!strcmp(host_mode, "play")) {
		// Start a game from the title, then let the demo player drive
		if (f > 100 && f < 104) return 4;
		if (f > 120) return inv_demo_input(f);
	}
	if (!strcmp(host_mode, "idle")) {
		if (f > 100 && f < 104) return 4;
	}
	return 0;
}

void host_frame(uint32_t frame, uint32_t sprites) {
	if (frame % 600 == 0) {
		printf("%6u score %6u lives %u wave %2u invaders %2u sprites %3u\n",
			frame, inv_score_get(), inv_gun_get_lives(), inv_level_get(), inv_invaders_alive(), sprites);
	}
}
