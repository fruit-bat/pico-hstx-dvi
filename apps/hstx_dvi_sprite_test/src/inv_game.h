#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Plays a game through to game over
void inv_game_play(void);

// Lets the demo player have a go. Returns true if a player pressed fire or
// start to take over.
bool inv_game_demo(void);

#ifdef __cplusplus
}
#endif
