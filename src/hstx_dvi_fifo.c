#include "hstx_dvi_fifo.h"
#include "hstx_dvi_fifo.pio.h"
#include "pico/stdlib.h"

static void fifo_passthrough_program_init(PIO pio, uint sm, uint offset) {
    pio_sm_config c = fifo_passthrough_program_get_default_config(offset);
    sm_config_set_in_shift(&c, true, false, 32);  // Auto-push 32-bit to RX
    sm_config_set_out_shift(&c, true, false, 32); // Auto-pull 32-bit from TX
    sm_config_set_clkdiv(&c, 1.0f);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

uint hstx_dvi_fifo_init(PIO pio, uint sm) {

    // Load the PIO program
    uint offset = pio_add_program(pio, &fifo_passthrough_program);
    fifo_passthrough_program_init(pio, sm, offset);

    sleep_ms(200); // Allow time for the PIO program to initialize

    return offset;
}

uint hstx_dvi_fifo_init1(PIO pio) {
    uint sm = pio_claim_unused_sm(pio, true);
    hstx_dvi_fifo_init(pio, sm);
    return sm;
}
