#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// The screens shown between games. Each returns true if the player pressed
// fire or smart bomb to play.

bool def_attract_title(void);

// highlight is the rank to pick out, or -1 for none
bool def_attract_hiscores(int32_t highlight);

// Lets the player put their initials in the high score table, if they made it
bool def_attract_game_over(void);

#ifdef __cplusplus
}
#endif
