
add_executable(hstx_dvi_lisp_test
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
  ${CMAKE_CURRENT_LIST_DIR}/src/lisp-repl.cpp)

set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/src/lisp.c PROPERTIES COMPILE_FLAGS -Wno-strict-aliasing)
set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/src/lisp-repl.cpp PROPERTIES COMPILE_FLAGS -Wno-strict-aliasing)
set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/src/lisp-repl.cpp PROPERTIES COMPILE_FLAGS -fexceptions)

target_compile_definitions(hstx_dvi_lisp_test PRIVATE
  PICO_CORE1_STACK_SIZE=0x400
  PICO_CORE0_STACK_SIZE=0x10000
  MODE_BYTES_PER_PIXEL=1
)

target_link_libraries(hstx_dvi_lisp_test
  pico_stdlib
  pico_multicore
  hardware_dma
  hardware_pio
  pico_sync
  pico_hstx_dvi
  pico_hstx_dvi_grid
)

# create map/bin/hex file etc.
pico_add_extra_outputs(hstx_dvi_lisp_test)

pico_enable_stdio_usb(hstx_dvi_lisp_test 1)
pico_enable_stdio_uart(hstx_dvi_lisp_test 0)
