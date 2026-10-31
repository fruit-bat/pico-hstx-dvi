
add_executable(hstx_dvi_android_test
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
  ${CMAKE_CURRENT_LIST_DIR}/src/android_art.c
)

target_compile_definitions(hstx_dvi_android_test PRIVATE
  PICO_CORE1_STACK_SIZE=0x400
  MODE_BYTES_PER_PIXEL=1
)

target_link_libraries(hstx_dvi_android_test
  pico_stdlib
  pico_multicore
  hardware_dma
  hardware_pio
  pico_sync
  pico_hstx_dvi
  game_common
)

# create map/bin/hex file etc.
pico_add_extra_outputs(hstx_dvi_android_test)

pico_enable_stdio_usb(hstx_dvi_android_test 1)
pico_enable_stdio_uart(hstx_dvi_android_test 0)
