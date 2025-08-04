#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "hstx_dvi_core.h"

void hstx_dvi_row_buf_init();
hstx_dvi_row_t* hstx_dvi_row_buf_get();

#ifdef __cplusplus
} 
#endif

