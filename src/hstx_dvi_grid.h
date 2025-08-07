#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HSTX_DVI_GRID_ATTRS_BOLD      (0x01)
#define HSTX_DVI_GRID_ATTRS_DIM       (0x02)
#define HSTX_DVI_GRID_ATTRS_UNDERLINE (0x04)
#define HSTX_DVI_GRID_ATTRS_BLINK     (0x08)
#define HSTX_DVI_GRID_ATTRS_REVERSE   (0x10)
#define HSTX_DVI_GRID_ATTRS_INVISIBLE (0x20)

void hstx_dvi_grid_init();
void hstx_dvi_grid_clear();
void hstx_dvi_grid_render_frame(uint32_t frame_index);
void hstx_dvi_grid_write_str(
    const uint32_t y,
    const uint32_t x,
    const char *s,
    const uint8_t fgi,
    const uint8_t bgi,
    const uint8_t attr
);
void hstx_dvi_grid_write_ch(
    const uint32_t y,
    const uint32_t x,
    const char c,
    const uint8_t fgi,
    const uint8_t bgi,
    const uint8_t attr
);
void hstx_dvi_grid_set_pallet(
    const uint8_t index,
    hstx_dvi_pixel_t color
);

#ifdef __cplusplus
} 
#endif
