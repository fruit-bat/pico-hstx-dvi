#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include "pico/stdlib.h"

#define MODE_H_SYNC_POLARITY 0
#define MODE_H_FRONT_PORCH   16
#define MODE_H_SYNC_WIDTH    96
#define MODE_H_BACK_PORCH    48
#define MODE_H_ACTIVE_PIXELS 640

#define MODE_V_SYNC_POLARITY 0
#define MODE_V_FRONT_PORCH   10
#define MODE_V_SYNC_WIDTH    2
#define MODE_V_BACK_PORCH    33
#define MODE_V_ACTIVE_LINES  480

#ifndef MODE_BYTES_PER_PIXEL
#define MODE_BYTES_PER_PIXEL 1
#endif
#define HSTX_DVI_BYTES_PER_ROW (MODE_BYTES_PER_PIXEL * MODE_H_ACTIVE_PIXELS)

typedef union {
    uint8_t b[HSTX_DVI_BYTES_PER_ROW];
    uint16_t s[(HSTX_DVI_BYTES_PER_ROW + 1) >> 1];
    uint32_t w[(HSTX_DVI_BYTES_PER_ROW + 3) >> 2];    
} hstx_dvi_row_t __attribute__((aligned(4)));;

#if MODE_BYTES_PER_PIXEL == 1
typedef uint8_t hstx_dvi_pixel_t; 
__force_inline void hstx_dvi_row_set_pixel(hstx_dvi_row_t* row, const uint32_t i, const uint32_t rgb332) {
    row->b[i] = rgb332;
}
__force_inline hstx_dvi_pixel_t hstx_dvi_row_pixel_rgb(const uint8_t r, const uint8_t g, const uint8_t b) {
    return (r & 0b11100000) | ((g >> 3) & 0b00011100) | ((b >> 6) & 0b00000011);
}
#elif MODE_BYTES_PER_PIXEL == 2
typedef uint16_t hstx_dvi_pixel_t; 
__force_inline void hstx_dvi_row_set_pixel(hstx_dvi_row_t* row, const uint32_t i, const uint32_t rgb565) {
    row->s[i] = rgb565;
}
__force_inline hstx_dvi_pixel_t hstx_dvi_row_pixel_rgb(const uint8_t r, const uint8_t g, const uint8_t b) {
    return ((uint32_t)r & 0xf8) << 8 | ((uint32_t)g & 0xfc) << 3 | ((uint32_t)b & 0xf8) >> 3;
}
#else
    #error "Unsupported MODE_BYTES_PER_PIXEL value"
#endif
typedef hstx_dvi_row_t* (*hstx_dvi_pixel_row_fetcher)(uint32_t row_index);

void hstx_dvi_init(hstx_dvi_pixel_row_fetcher row_fetcher, hstx_dvi_row_t* underflow_row);
void hstx_dvi_start();

#ifdef __cplusplus
} 
#endif

