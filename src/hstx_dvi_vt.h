#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_core.h"
#include "vt/vt_types.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t hstx_dvi_vt_attr_t;
#define HSTX_DVI_GRID_ATTRS_NORMAL    (0x00)
#define HSTX_DVI_GRID_ATTRS_BOLD      (0x01)
#define HSTX_DVI_GRID_ATTRS_DIM       (0x02)
#define HSTX_DVI_GRID_ATTRS_UNDERLINE (0x04)
#define HSTX_DVI_GRID_ATTRS_BLINK     (0x08)
#define HSTX_DVI_GRID_ATTRS_REVERSE   (0x10)
#define HSTX_DVI_GRID_ATTRS_INVISIBLE (0x20)

void hstx_dvi_vt_init_all();
void hstx_dvi_vt_render_loop();

void hstx_dvi_vt_init();
void hstx_dvi_vt_clear();
void hstx_dvi_vt_render_frame(uint32_t frame_index);


#ifdef __cplusplus
} 
#endif
