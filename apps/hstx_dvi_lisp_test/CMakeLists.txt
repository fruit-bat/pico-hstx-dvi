
add_executable(hstx_dvi_lisp_test
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
  ${CMAKE_CURRENT_LIST_DIR}/src/lisp-repl.cpp)

# The interpreter reports errors with C++ exceptions, and reads the atom/string heap
# through casts of a char pointer, so it must not assume strict aliasing
set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/src/lisp-repl.cpp PROPERTIES COMPILE_FLAGS "-fexceptions -fno-strict-aliasing")

# The terminal's font: terminus_8x16 gives 80 x 30 with line drawing; 8x8
# gives 80 x 60, ASCII only. Any of the fonts in src/hstx_dvi_font.h will do,
# e.g. cmake -DLISP_VT_FONT=8x8 .. If the one chosen isn't built (see the top
# CMakeLists.txt), the 8x8 font is used.
set(LISP_VT_FONT "terminus_8x16" CACHE STRING "The Lisp terminal's font: 8x8, terminus_8x14, terminus_8x16, terminus_12x24, terminus_16x32 or vga_8x16")

target_compile_definitions(hstx_dvi_lisp_test PRIVATE
  PICO_CORE1_STACK_SIZE=0x400
  VT_FONT="${LISP_VT_FONT}"
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
