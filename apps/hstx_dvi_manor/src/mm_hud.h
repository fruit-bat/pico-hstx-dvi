#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MM_HUD_START_LIVES 3
#define MM_HUD_MAX_LIVES 6
#define MM_HUD_BONUS 10000

void mm_hud_init(void);
void mm_hud_reset(void);

void mm_hud_add_score(uint32_t points);
// Scores and shows the points at a place in the house, in pixels
void mm_hud_score_at(int32_t x, int32_t y, uint32_t points);
uint32_t mm_hud_score(void);
void mm_hud_set_score(uint32_t score);
uint32_t mm_hud_lives(void);
// Returns false if there were none left to lose
bool mm_hud_take_life(void);

void mm_hud_draw(uint32_t night, uint32_t frame, bool in_game);

// Something said at a place in the house, in pixels, which floats up for a while
void mm_hud_say(int32_t x, int32_t y, const char* s, uint32_t colour);
void mm_hud_say_clear(void);
void mm_hud_say_update(void);
void mm_hud_say_draw(void);

#ifdef __cplusplus
}
#endif
