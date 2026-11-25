// Copyright (c) 2024 fruit-bat.

// The score, lives and fuel along the top, and the power bar and beam gauge
// along the bottom
#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// An extra life at the first, then every so many after
#define RR_HUD_FIRST_BONUS 20000
#define RR_HUD_BONUS 50000

void rr_hud_reset(void);
void rr_hud_add_score(uint32_t points);
uint32_t rr_hud_score(void);
void rr_hud_set_score(uint32_t score);
uint32_t rr_hud_lives(void);
// Times round the sections, from 0, for the section shown
void rr_hud_set_loop(uint32_t loop);
// Takes one, returning false if there were none left
bool rr_hud_take_life(void);

// in_game shows the lives, fuel, section and power bar. Its text goes over
// what was drawn before it, so add it after the play area.
void rr_hud_draw(uint32_t frame, bool in_game);

#ifdef __cplusplus
}
#endif
