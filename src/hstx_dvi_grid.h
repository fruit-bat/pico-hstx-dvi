#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_core.h"

#ifdef __cplusplus
extern "C" {
#endif

void hstx_dvi_grid_init();
void hstx_dvi_grid_clear();
void hstx_dvi_grid_render_frame();
void hstx_dvi_grid_write_str(
    const uint32_t y,
    const uint32_t x,
    const char *s,
    const uint8_t fgi,
    const uint8_t bgi
);
void hstx_dvi_grid_write_ch(
    const uint32_t y,
    const uint32_t x,
    const char c,
    const uint8_t fgi,
    const uint8_t bgi
);
void hstx_dvi_grid_set_pallet(
    const uint8_t index,
    hstx_dvi_pixel_t color
);

#ifdef __cplusplus
} 
#endif
