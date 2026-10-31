#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// The screens shown between games. Each returns true if the player pressed
// fire or start to play.

bool inv_attract_title(void);

bool inv_attract_score_table(void);

// highlight is the rank to pick out, or -1 for none
bool inv_attract_hiscores(int32_t highlight);

// Lets the player put their initials in the high score table, if they made it
bool inv_attract_game_over(void);

#ifdef __cplusplus
}
#endif
