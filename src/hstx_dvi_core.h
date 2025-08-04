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

uintptr_t get_pixel_row(uint row_index);

#ifdef __cplusplus
} 
#endif
