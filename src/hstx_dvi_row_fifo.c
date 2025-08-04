#include "hardware/pio.h"
#include "hstx_dvi_row_fifo.h"
#include "hstx_dvi_row_fifo.pio.h"


static PIO _pio = pio0;
static uint _sm = 0;

// Call this in your main program
void fifo_passthrough_program_init(PIO pio, uint sm, uint offset) {
    pio_sm_config c = fifo_passthrough_program_get_default_config(offset);
    sm_config_set_in_shift(&c, true, true, 32);  // Auto-push 32-bit to RX
    sm_config_set_out_shift(&c, true, true, 32); // Auto-pull 32-bit from TX

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

void hstx_dvi_row_fifo_init(PIO pio, uint sm) {
    _pio = pio;
    _sm = sm;

    // Load the PIO program
    uint offset = pio_add_program(pio, &fifo_passthrough_program);
    fifo_passthrough_program_init(pio, sm, offset);
}
