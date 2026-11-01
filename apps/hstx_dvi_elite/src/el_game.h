// Copyright (c) 2024 fruit-bat.

// A game: flying about a system, from launching off its space station.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Returns when the game is over
void game_play(void);

// For trying docking on the PC: puts us on a run at the station's slot,
// rolled from level by roll
void game_debug_approach(float roll);

#ifdef __cplusplus
}
#endif
