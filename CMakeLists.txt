add_library(pico_hstx_dvi INTERFACE)

target_sources(pico_hstx_dvi INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_core.c
  ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_row_fifo.c
  ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_row_buf.c
  ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_sprite.c
)

add_library(pico_hstx_dvi_grid INTERFACE)

target_sources(pico_hstx_dvi_grid INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_grid.c
)

add_library(pico_hstx_dvi_tmt INTERFACE)

target_sources(pico_hstx_dvi_tmt INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/src/libtmt/tmt.c
  ${CMAKE_CURRENT_LIST_DIR}/src/libtmt/tmt.h
)

pico_generate_pio_header(pico_hstx_dvi ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_row_fifo.pio)

target_include_directories(pico_hstx_dvi INTERFACE
    src
)

add_library(vtlib INTERFACE)

target_sources(vtlib INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/src/vt/vt_ansi_seq.c
  ${CMAKE_CURRENT_LIST_DIR}/src/vt/vt_ansi_seq.h
)
