#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void ww_game_play(void);
// Returns true if a player takes over
bool ww_game_demo(void);

uint32_t ww_game_night(void);

#ifdef __cplusplus
}
#endif
