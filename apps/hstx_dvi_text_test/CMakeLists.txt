
add_executable(hstx_dvi_text_test
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
)

target_compile_definitions(hstx_dvi_text_test PRIVATE
  MODE_BYTES_PER_PIXEL=2
)

target_link_libraries(hstx_dvi_text_test
  pico_stdlib
  pico_multicore
  hardware_dma
  hardware_pio
  pico_sync
  pico_hstx_dvi
)

# create map/bin/hex file etc.
pico_add_extra_outputs(hstx_dvi_text_test)

pico_enable_stdio_usb(hstx_dvi_text_test 1)
pico_enable_stdio_uart(hstx_dvi_text_test 0)
