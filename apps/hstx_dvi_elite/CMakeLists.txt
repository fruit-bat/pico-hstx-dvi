add_executable(hstx_dvi_elite
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
  ${CMAKE_CURRENT_LIST_DIR}/src/el_attract.c
  ${CMAKE_CURRENT_LIST_DIR}/src/el_chart.c
  ${CMAKE_CURRENT_LIST_DIR}/src/el_draw.c
  ${CMAKE_CURRENT_LIST_DIR}/src/el_game.c
  ${CMAKE_CURRENT_LIST_DIR}/src/el_hud.c
  ${CMAKE_CURRENT_LIST_DIR}/src/el_screen.c
  ${CMAKE_CURRENT_LIST_DIR}/src/el_space.c
  ${CMAKE_CURRENT_LIST_DIR}/src/galaxy.c
  ${CMAKE_CURRENT_LIST_DIR}/src/ship_model.c
)

target_compile_definitions(hstx_dvi_elite PRIVATE
  PICO_CORE1_STACK_SIZE=0x800
  MODE_BYTES_PER_PIXEL=1
  # The meter at the top of the screen, see README.md
  HSTX_DVI_SPRITE_TIMING=1
  # Buttons on GPIOs 4 to 11, clear of the display on 12 to 19
  EL_INP_GPIO_START=4
)

target_link_libraries(hstx_dvi_elite
  pico_stdlib
  pico_multicore
  hardware_dma
  hardware_pio
  pico_sync
  pico_hstx_dvi
  game_common
)

# create map/bin/hex file etc.
pico_add_extra_outputs(hstx_dvi_elite)

pico_enable_stdio_usb(hstx_dvi_elite 1)
pico_enable_stdio_uart(hstx_dvi_elite 0)
