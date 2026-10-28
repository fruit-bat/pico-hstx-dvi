#pragma once

#include "hstx_dvi_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INV_PALLET_RAINBOW_COUNT 8

hstx_dvi_pixel_t* inv_pallet_green();
hstx_dvi_pixel_t* inv_pallet_red();
hstx_dvi_pixel_t* inv_pallet_blue();
hstx_dvi_pixel_t* inv_pallet_purple();
hstx_dvi_pixel_t* inv_pallet_white();
hstx_dvi_pixel_t* inv_pallet_yellow();
hstx_dvi_pixel_t* inv_pallet_grey();
hstx_dvi_pixel_t* inv_pallet_rainbow(uint32_t i);

#ifdef __cplusplus
}
#endif
