// Copyright (c) 2024 fruit-bat.
#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void rr_game_play(void);
// Returns true if a player takes it over
bool rr_game_demo(void);

#ifdef __cplusplus
}
#endif
