// Drives the wildwood app on the PC, see common/host
//
// attract - nobody touches the buttons
// play    - starts a game and lets the demo player drive it
// idle    - starts a game and stands still in the first room
// charm   - as play, but with the whole moon charm from the start, for the end of the night
#include <stdio.h>
#include <string.h>
#include "host.h"

// From the game, to drive and report on it
uint8_t ww_demo_input(uint32_t frame);
uint32_t ww_hud_score(void);
uint32_t ww_hud_lives(void);
uint32_t ww_hud_pieces(void);
uint32_t ww_witch_hearts(void);
uint32_t ww_world_room(void);
uint32_t ww_world_visited_count(void);
uint32_t ww_animal_helped(void);
uint32_t ww_game_night(void);
void ww_hud_add_piece(uint32_t i);

// Buttons as the game sees them: L R U D F B from bit 0
uint32_t host_input(uint32_t f) {
	if (!strcmp(host_mode, "play") || !strcmp(host_mode, "charm")) {
		// Start a game from the title, then let the demo player drive
		if (f > 100 && f < 104) return 16;
		if (f > 120) return ww_demo_input(f);
	}
	if (!strcmp(host_mode, "idle")) {
		if (f > 100 && f < 104) return 16;
	}
	return 0;
}

void host_frame(uint32_t frame, uint32_t sprites) {
	if (!strcmp(host_mode, "charm") && frame == 200) {
		for (uint32_t i = 0; i < 4; ++i) ww_hud_add_piece(i);
	}
	if (frame % 600 == 0) {
		printf("%6u score %6u lives %u hearts %u night %u pieces %x room %2u visited %2u helped %2u sprites %3u\n",
			frame, ww_hud_score(), ww_hud_lives(), ww_witch_hearts(), ww_game_night(), ww_hud_pieces(),
			ww_world_room(), ww_world_visited_count(), ww_animal_helped(), sprites);
	}
}
