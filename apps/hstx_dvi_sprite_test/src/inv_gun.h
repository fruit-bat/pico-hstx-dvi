#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

SpriteId inv_gun_init(SpriteId start);

void inv_gun_update(uint32_t frame);

uint32_t inv_gun_get_lives(void);

#ifdef __cplusplus
}
#endif

