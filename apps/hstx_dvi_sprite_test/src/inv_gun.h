#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

SpriteId inv_gun_init(SpriteId start);

void inv_gun_update(void);

#ifdef __cplusplus
} 
#endif

