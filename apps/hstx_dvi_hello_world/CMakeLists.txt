
add_executable(hstx_dvi_hello_world
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
)

target_compile_definitions(hstx_dvi_hello_world PRIVATE
  MODE_BYTES_PER_PIXEL=1
)

target_include_directories(hstx_dvi_hello_world PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/images
        )

target_link_libraries(hstx_dvi_hello_world
  pico_stdlib
  pico_multicore
  hardware_dma
  hardware_pio
  pico_sync
  pico_hstx_dvi
)

# create map/bin/hex file etc.
pico_add_extra_outputs(hstx_dvi_hello_world)
