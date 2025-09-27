#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_core.h"
#include "vt_emu.h"
#ifdef __cplusplus
extern "C" {
#endif

void hstx_dvi_vt_init_all();
void hstx_dvi_vt_render_loop();

void hstx_dvi_vt_init();
void hstx_dvi_vt_render_frame(uint32_t frame_index);
vt_emu_t* hstx_dvi_vt_emu_get();


#ifdef __cplusplus
} 
#endif
