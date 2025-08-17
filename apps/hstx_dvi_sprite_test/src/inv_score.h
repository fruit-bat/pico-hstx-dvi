#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

SpriteId inv_score_init(SpriteId start);
void inv_score_update(void);
void inv_score_clear(void);
void inv_score_add(uint32_t score);

#ifdef __cplusplus
}
#endif

