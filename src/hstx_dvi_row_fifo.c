#include "hardware/pio.h"
#include "hstx_dvi_row_fifo.h"
#include "hstx_dvi_row_fifo.pio.h"


static PIO _pio = pio0;
static uint _sm = 0;
static hstx_dvi_row_t* _underflow_row;

hstx_dvi_row_t* __scratch_x("") hstx_dvi_row_fifo_get(uint32_t row_index) {
    // Wait for data to be available in the RX FIFO
    if (pio_sm_is_rx_fifo_empty(_pio, _sm)) return _underflow_row;
    // Read data from the RX FIFO
    uint32_t value = pio_sm_get_blocking(_pio, _sm);    

    // Cast the value back to hstx_dvi_row_t pointer
    return (hstx_dvi_row_t*)value;
}

void fifo_passthrough_program_init(PIO pio, uint sm, uint offset) {
    pio_sm_config c = fifo_passthrough_program_get_default_config(offset);
    sm_config_set_in_shift(&c, true, false, 32);  // Auto-push 32-bit to RX
    sm_config_set_out_shift(&c, true, false, 32); // Auto-pull 32-bit from TX
    sm_config_set_clkdiv(&c, 1.0f);
    pio_sm_init(pio, sm, offset, &c);

    pio_sm_set_enabled(pio, sm, true);
}

void hstx_dvi_row_fifo_init(PIO pio, uint sm, hstx_dvi_row_t* underflow_row) {
    _pio = pio;
    _sm = sm;
    _underflow_row = underflow_row;

    // Load the PIO program
    uint offset = pio_add_program(pio, &fifo_passthrough_program);
    fifo_passthrough_program_init(pio, sm, offset);
}

void hstx_dvi_row_fifo_init1(PIO pio, hstx_dvi_row_t* underflow_row) {
    uint sm = pio_claim_unused_sm(pio, true);
    hstx_dvi_row_fifo_init(pio, sm, underflow_row);
}

void __not_in_flash_func(hstx_dvi_row_fifo_put_blocking)(hstx_dvi_row_t* row){

    static bool initialized = false;

    if (!initialized && pio_sm_is_tx_fifo_full(_pio, _sm)) {
        hstx_dvi_start(hstx_dvi_row_fifo_get);
        initialized = true;
    }

    pio_sm_put_blocking(_pio, _sm, (uint32_t)row);
}
