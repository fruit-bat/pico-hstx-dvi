#include "hstx_dvi_core.h"
#include "hstx_dvi_row_fifo.h"

// TODO Why do I need to add 8 to this!
#define ROW_BUF_SIZE (HSTX_DVI_ROW_FIFO_SIZE + 8)
static hstx_dvi_row_t row[ROW_BUF_SIZE];
static uint32_t row_index = 0;

void hstx_dvi_row_buf_init() {
    // Initialize the row buffer with zeros
    for (uint32_t i = 0; i < ROW_BUF_SIZE; ++i) {
        for (uint32_t j = 0; j < HSTX_DVI_BYTES_PER_ROW; ++j) {
            row[i].b[j] = 0;
        }
    }
}

hstx_dvi_row_t* __not_in_flash_func(hstx_dvi_row_buf_get)() {
    // Return the next row in the buffer, wrapping around if necessary
    hstx_dvi_row_t* current_row = &row[row_index];
    ++row_index;
    if (row_index >= ROW_BUF_SIZE) row_index=0;
    return current_row;
}

