#include "hardware/pio.h"
#include "hstx_dvi_row_fifo.h"
#include "hstx_dvi_row_fifo.pio.h"


static PIO _pio = pio0;
static uint _sm = 0;

hstx_dvi_row_t* HSTX_DVI_MEM_LOC(hstx_dvi_row_fifo_get)(uint32_t row_index) {
    // Wait for data to be available in the RX FIFO
    if (pio_sm_is_rx_fifo_empty(_pio, _sm)) return 0;

    // Read data from the RX FIFO
    return (hstx_dvi_row_t*)_pio->rxf[_sm];
}

void fifo_passthrough_program_init(PIO pio, uint sm, uint offset) {
    pio_sm_config c = fifo_passthrough_program_get_default_config(offset);
    sm_config_set_in_shift(&c, true, false, 32);  // Auto-push 32-bit to RX
    sm_config_set_out_shift(&c, true, false, 32); // Auto-pull 32-bit from TX
    sm_config_set_clkdiv(&c, 1.0f);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

hstx_dvi_pixel_row_fetcher hstx_dvi_row_fifo_init(PIO pio, uint sm, hstx_dvi_row_t* underflow_row) {
    _pio = pio;
    _sm = sm;

    // Load the PIO program
    uint offset = pio_add_program(pio, &fifo_passthrough_program);
    fifo_passthrough_program_init(pio, sm, offset);

    sleep_ms(200); // Allow time for the PIO program to initialize

    // for(int i = 0; i < 8; i++) {
    //     // Write the underflow row to the RX FIFO
    //     pio_sm_put_blocking(pio, sm, (uint32_t)underflow_row);
    // }

    return hstx_dvi_row_fifo_get;
}

hstx_dvi_pixel_row_fetcher hstx_dvi_row_fifo_init1(PIO pio, hstx_dvi_row_t* underflow_row) {
    uint sm = pio_claim_unused_sm(pio, true);
    return hstx_dvi_row_fifo_init(pio, sm, underflow_row);
}

void __not_in_flash_func(hstx_dvi_row_fifo_put_blocking)(hstx_dvi_row_t* row){
    pio_sm_put_blocking(_pio, _sm, (uint32_t)row);
}

hstx_dvi_pixel_row_fetcher hstx_dvi_row_fifo_get_row_fetcher() {
    return hstx_dvi_row_fifo_get;
}
