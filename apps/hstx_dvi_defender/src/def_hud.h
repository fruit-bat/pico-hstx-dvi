#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

// An extra ship and smart bomb each time the score passes a multiple of this
#define DEF_HUD_BONUS 10000

void def_hud_reset(void);

void def_hud_add_score(uint32_t points);
uint32_t def_hud_score(void);
void def_hud_set_score(uint32_t score);

// Ships in reserve
uint32_t def_hud_lives(void);
// Returns false if there were none left
bool def_hud_take_life(void);

uint32_t def_hud_smart_bombs(void);
bool def_hud_use_smart_bomb(void);

// The ships and smart bombs are only shown when in_game is set
void def_hud_draw(uint32_t wave, bool in_game);

#ifdef __cplusplus
}
#endif
