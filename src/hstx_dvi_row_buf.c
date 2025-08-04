#include "hstx_dvi_core.h"
#include "hstx_dvi_row_fifo.h"

static hstx_dvi_row_t row[HSTX_DVI_ROW_FIFO_SIZE];
static uint32_t row_index = 0;

void hstx_dvi_row_buf_init() {
    // Initialize the row buffer with zeros
    for (uint32_t i = 0; i < HSTX_DVI_ROW_FIFO_SIZE; ++i) {
        for (uint32_t j = 0; j < HSTX_DVI_BYTES_PER_ROW; ++j) {
            row[i].b[j] = 0;
        }
    }
}

hstx_dvi_row_t* __not_in_flash_func(hstx_dvi_row_buf_get)() {
    // Return the next row in the buffer, wrapping around if necessary
    hstx_dvi_row_t* current_row = &row[row_index];
    row_index = (row_index + 1) % HSTX_DVI_ROW_FIFO_SIZE;
    return current_row;
}

