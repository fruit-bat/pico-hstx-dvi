#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "hardware/pio.h"

#define HSTX_DVI_FIFO_SIZE 8

uint hstx_dvi_fifo_init(PIO pio, uint sm);
uint hstx_dvi_fifo_init1(PIO pio);

static inline void hstx_dvi_fifo_put_blocking(PIO pio, uint sm, uint32_t d) {
    pio_sm_put_blocking(pio, sm, d);
}

static inline uint32_t hstx_dvi_fifo_get(PIO pio, uint sm) {
    // Wait for data to be available in the RX FIFO
    if (pio_sm_is_rx_fifo_empty(pio, sm)) return 0;
    // Read data from the RX FIFO
    return pio->rxf[sm];
}

#ifdef __cplusplus
} 
#endif
