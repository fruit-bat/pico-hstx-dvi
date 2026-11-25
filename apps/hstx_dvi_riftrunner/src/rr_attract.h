// Copyright (c) 2024 fruit-bat.

// The screens between games. Each returns true if a player wants to play.
#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

bool rr_attract_title(void);
// The enemies and what they are worth
bool rr_attract_roll(void);
// The giants that come from the second round
bool rr_attract_giants(void);
bool rr_attract_howto(void);
// highlight is the rank to flash, or -1
bool rr_attract_hiscores(int32_t highlight);
// Takes initials for a new high score, then shows the table
bool rr_attract_game_over(void);

#ifdef __cplusplus
}
#endif
