#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Each returns true if the player wants to play

// The title, over a tour of the house
bool mm_attract_title(void);
// Who lives in the house and what to do about them
bool mm_attract_roll(void);
// highlight is the rank to pick out, or -1
bool mm_attract_hiscores(int32_t highlight);
// Initials for a new high score, then the table
bool mm_attract_game_over(void);

#ifdef __cplusplus
}
#endif
