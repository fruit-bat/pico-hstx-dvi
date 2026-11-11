add_library(pico_hstx_dvi INTERFACE)

target_sources(pico_hstx_dvi INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_core.c
  ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_row_fifo.c
  ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_fifo.c
  ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_row_buf.c
  ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_sprite.c
  ${CMAKE_CURRENT_LIST_DIR}/src/font_inv.c
)

add_library(pico_hstx_dvi_grid INTERFACE)

target_sources(pico_hstx_dvi_grid INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_grid.c
)

add_library(pico_hstx_dvi_vt INTERFACE)

target_sources(pico_hstx_dvi_vt INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_vt.c
  ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_font.c
  ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_fonts.c
)
target_link_libraries(pico_hstx_dvi_vt INTERFACE pico_vtlib)

# The virtual terminal's fonts: each larger one can be left out, trading
# quality for memory, e.g. cmake -DHSTX_DVI_FONT_TERMINUS_16X32=OFF ..
# The 8x8 font (ASCII only, 80x60 at 640x480) is always built. The others have
# line drawing, blocks and accented letters too. Each takes the flash shown,
# and the terminal takes RAM for a copy of the biggest built: all OFF saves
# about 35 KB of flash and 13 KB of RAM. See src/hstx_dvi_font.h and README.md.
option(HSTX_DVI_FONT_TERMINUS_8X14  "Build the Terminus 8x14 font, 80x34 (3.0 KB flash)" ON)
option(HSTX_DVI_FONT_TERMINUS_8X16  "Build the Terminus 8x16 font, 80x30 (3.5 KB flash)" ON)
option(HSTX_DVI_FONT_TERMINUS_12X24 "Build the Terminus 12x24 font, 53x20 (10.5 KB flash, 10.5 KB RAM if the biggest)" ON)
option(HSTX_DVI_FONT_TERMINUS_16X32 "Build the Terminus 16x32 font, 40x15 (13.9 KB flash, 13.9 KB RAM)" ON)
option(HSTX_DVI_FONT_VGA_8X16       "Build the IBM PC's VGA 8x16 font, 80x30 (3.5 KB flash)" ON)
target_compile_definitions(pico_hstx_dvi_vt INTERFACE
  HSTX_DVI_FONT_TERMINUS_8X14=$<BOOL:${HSTX_DVI_FONT_TERMINUS_8X14}>
  HSTX_DVI_FONT_TERMINUS_8X16=$<BOOL:${HSTX_DVI_FONT_TERMINUS_8X16}>
  HSTX_DVI_FONT_TERMINUS_12X24=$<BOOL:${HSTX_DVI_FONT_TERMINUS_12X24}>
  HSTX_DVI_FONT_TERMINUS_16X32=$<BOOL:${HSTX_DVI_FONT_TERMINUS_16X32}>
  HSTX_DVI_FONT_VGA_8X16=$<BOOL:${HSTX_DVI_FONT_VGA_8X16}>
)

add_library(pico_hstx_dvi_tmt INTERFACE)

target_sources(pico_hstx_dvi_tmt INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/src/libtmt/tmt.c
  ${CMAKE_CURRENT_LIST_DIR}/src/libtmt/tmt.h
)

pico_generate_pio_header(pico_hstx_dvi ${CMAKE_CURRENT_LIST_DIR}/src/hstx_dvi_fifo.pio)

target_include_directories(pico_hstx_dvi INTERFACE
    src
)

add_subdirectory(${PICO_FOLDER}/pico-vt "${CMAKE_CURRENT_BINARY_DIR}/pico-vt")
