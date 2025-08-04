add_library(pico_hstx_dvi INTERFACE)

target_sources(pico_hstx_dvi INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_core.c
  ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_row_fifo.c
  ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_row_buf.c
)

pico_generate_pio_header(pico_hstx_dvi ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_row_fifo.pio)

target_include_directories(pico_hstx_dvi INTERFACE
    src
)
