
add_executable(hstx_dvi_vt_test
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
)

# The font, and how many times as wide and tall it is drawn (1 or 2), e.g.
# cmake -DVT_FONT=terminus_8x16 .. for 80 x 30. See hstx_dvi_vt.h for them all.
set(VT_FONT "8x8" CACHE STRING "The virtual terminal's font: 8x8, terminus_8x14, terminus_8x16, terminus_12x24, terminus_16x32 or vga_8x16")
set(VT_SCALE_X 1 CACHE STRING "The virtual terminal's font drawn this many times as wide, 1 or 2")
set(VT_SCALE_Y 1 CACHE STRING "The virtual terminal's font drawn this many times as tall, 1 or 2")

target_compile_definitions(hstx_dvi_vt_test PRIVATE
  PICO_CORE1_STACK_SIZE=0x400
  MODE_BYTES_PER_PIXEL=1
  VT_FONT="${VT_FONT}"
  VT_SCALE_X=${VT_SCALE_X}
  VT_SCALE_Y=${VT_SCALE_Y}
)

target_link_libraries(hstx_dvi_vt_test
  pico_stdlib
  pico_multicore
  hardware_dma
  hardware_pio
  pico_sync
  pico_hstx_dvi
  pico_hstx_dvi_vt
  pico_vtlib
)

# create map/bin/hex file etc.
pico_add_extra_outputs(hstx_dvi_vt_test)

pico_enable_stdio_usb(hstx_dvi_vt_test 1)
pico_enable_stdio_uart(hstx_dvi_vt_test 0)
