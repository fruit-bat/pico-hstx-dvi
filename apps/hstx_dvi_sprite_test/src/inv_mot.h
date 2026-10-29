#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

SpriteId inv_mot_init(SpriteId start);

void inv_mot_update(uint32_t frame);

// For showing the mystery ship off outside of the game, on sprites handed out each frame
void inv_mot_draw_icon(int32_t x, int32_t y, uint32_t frame);

#ifdef __cplusplus
}
#endif

