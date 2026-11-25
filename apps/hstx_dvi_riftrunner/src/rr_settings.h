// Copyright (c) 2024 fruit-bat.

// The settings: the shared ones (music on or off, the sound test) and Rift
// Runner's own, music while flying and which tune. Opened with start between
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

#ifdef __cplusplus
}
#endif
