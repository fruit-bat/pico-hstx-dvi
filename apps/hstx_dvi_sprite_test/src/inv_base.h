#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

SpriteId inv_base_init(SpriteId start);

void inv_base_update(void);

void inv_base_hit(SpriteId gunSpriteId);

#ifdef __cplusplus
} 
#endif

