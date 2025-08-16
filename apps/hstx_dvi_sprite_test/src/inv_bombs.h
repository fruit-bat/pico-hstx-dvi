#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

SpriteId inv_bombs_init(SpriteId start);

void inv_bombs_update(void);

void inv_bombs_fire(SpriteId invSpriteId);

#ifdef __cplusplus
} 
#endif

