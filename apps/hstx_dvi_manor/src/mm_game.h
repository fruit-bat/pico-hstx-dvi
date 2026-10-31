#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void mm_game_play(void);
// Returns true if a player takes over
bool mm_game_demo(void);

uint32_t mm_game_night(void);

// For the title to show the house off: sets it up with nobody in it but
// the spooks, then moves them along and draws them
void mm_game_tour_start(void);
void mm_game_tour_frame(uint32_t frame);

#ifdef __cplusplus
}
#endif
