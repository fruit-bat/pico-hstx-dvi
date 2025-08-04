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

void hstx_dvi_start(void);

typedef union {
	uint8_t b[MODE_H_ACTIVE_PIXELS];
    uint16_t s[(MODE_H_ACTIVE_PIXELS + 1) >> 1];
    uint32_t w[(MODE_H_ACTIVE_PIXELS + 3) >> 2];    
} hstx_dvi_row __attribute__((aligned(4)));;

hstx_dvi_row* hstx_dvi_get_pixel_row(uint row_index);

#ifdef __cplusplus
} 
#endif

