add_executable(hstx_dvi_wireframe_test
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
  ${CMAKE_CURRENT_LIST_DIR}/src/ship_model.c
  ${CMAKE_CURRENT_LIST_DIR}/src/galaxy.c
)

target_compile_definitions(hstx_dvi_wireframe_test PRIVATE
  PICO_CORE1_STACK_SIZE=0x800
  MODE_BYTES_PER_PIXEL=1
  HSTX_DVI_SPRITE_TIMING=1
  # Times the parts of the renderer's work at the start, see README.md
  # WIREFRAME_BENCH=1
)

target_link_libraries(hstx_dvi_wireframe_test
  pico_stdlib
  pico_multicore
  hardware_dma
  hardware_pio
  pico_sync
  pico_hstx_dvi
  game_common
)

# create map/bin/hex file etc.
pico_add_extra_outputs(hstx_dvi_wireframe_test)

pico_enable_stdio_usb(hstx_dvi_wireframe_test 1)
pico_enable_stdio_uart(hstx_dvi_wireframe_test 0)
