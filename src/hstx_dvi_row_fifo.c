#include "hardware/pio.h"
#include "hstx_dvi_row_fifo.h"
#include "hstx_dvi_fifo.pio.h"


static PIO _pio = pio0;
static uint _sm = 0;

hstx_dvi_row_t* HSTX_DVI_MEM_LOC(hstx_dvi_row_fifo_get)(uint32_t row_index) {
    return (hstx_dvi_row_t*)hstx_dvi_fifo_get(_pio, _sm);
}

void hstx_dvi_row_fifo_init(PIO pio, uint sm) {
    _pio = pio;
    _sm = sm;
    hstx_dvi_fifo_init(pio, sm);
}

void hstx_dvi_row_fifo_init1(PIO pio) {
    _pio = pio;
    _sm  = hstx_dvi_fifo_init1(pio);
}

void __not_in_flash_func(hstx_dvi_row_fifo_put_blocking)(hstx_dvi_row_t* row){
    hstx_dvi_fifo_put_blocking(_pio, _sm, (uint32_t)row);
}

hstx_dvi_pixel_row_fetcher hstx_dvi_row_fifo_get_row_fetcher() {
    return hstx_dvi_row_fifo_get;
}
