#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Plays a game through to game over
void def_game_play(void);

// Lets the demo player have a go. Returns true if a player pressed fire or
// smart bomb to take over.
bool def_game_demo(void);

#ifdef __cplusplus
}
#endif
