#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

SpriteId inv_bullets_init(SpriteId start);

void inv_bullets_update(void);

void inv_bullets_fire(SpriteId gunSpriteId);

#ifdef __cplusplus
} 
#endif

