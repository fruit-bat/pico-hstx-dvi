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
// For trying a fight on the PC: puts a ship of the named type ahead of us, coming at us
void game_debug_pirate(const char* name);

#ifdef __cplusplus
}
#endif
