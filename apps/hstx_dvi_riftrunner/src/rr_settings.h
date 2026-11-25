// Copyright (c) 2024 fruit-bat.

// The settings: the shared ones (music on or off, the sound test) and Rift
// Runner's own: music while flying and which tune, the round a game starts
// at, and whether the demo is heard. Opened with start between
// games, or in a game, where it pauses it.
#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// draw_back adds what goes behind, or NULL
void rr_settings_run(void (*draw_back)(uint32_t frame));

// The music for flying through the land and for the warship, as chosen, or
// none if music while flying is off. While flying, so a change is heard at once.
void rr_music_flying(void);
void rr_music_warship(void);
// Not flying any more, between games or after the warship
void rr_music_landed(void);

// The round a game starts at, from 0, for trying the later ones
uint32_t rr_settings_start_round(void);
// The demo game played without music or sounds
bool rr_settings_demo_silent(void);

#ifdef __cplusplus
}
#endif
