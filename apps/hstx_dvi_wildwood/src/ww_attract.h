#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Each returns true if the player wants to play

// The title, with the folk of the wood and what they need
bool ww_attract_title(void);
// highlight is the rank to pick out, or -1
bool ww_attract_hiscores(int32_t highlight);
// Initials for a new high score, then the table
bool ww_attract_game_over(void);

#ifdef __cplusplus
}
#endif
