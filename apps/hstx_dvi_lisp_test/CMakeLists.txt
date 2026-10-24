
add_executable(hstx_dvi_lisp_test
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
  ${CMAKE_CURRENT_LIST_DIR}/src/lisp-repl.cpp)

# The interpreter reports errors with C++ exceptions, and reads the atom/string heap
# through casts of a char pointer, so it must not assume strict aliasing
set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/src/lisp-repl.cpp PROPERTIES COMPILE_FLAGS "-fexceptions -fno-strict-aliasing")

target_compile_definitions(hstx_dvi_lisp_test PRIVATE
  PICO_CORE1_STACK_SIZE=0x400
  MODE_BYTES_PER_PIXEL=1
  # The USB serial terminal does not echo what is typed
  LISP_ECHO=1
)

target_link_libraries(hstx_dvi_lisp_test
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
pico_add_extra_outputs(hstx_dvi_lisp_test)

pico_enable_stdio_usb(hstx_dvi_lisp_test 1)
pico_enable_stdio_uart(hstx_dvi_lisp_test 0)
