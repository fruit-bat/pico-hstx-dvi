#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

// A bonus life is awarded each time the score passes a multiple of this
#define INV_SCORE_BONUS_LIFE 5000

// The score line across the top of the screen. The wave and lives are only
// shown when in_game is set. The score itself is kept, see inv_score_reset.
SpriteId inv_score_init(SpriteId start, bool in_game);
void inv_score_update(void);
void inv_score_add(uint32_t score);
void inv_score_reset(void);
uint32_t inv_score_get(void);
void inv_score_set(uint32_t score);

#ifdef __cplusplus
}
#endif
