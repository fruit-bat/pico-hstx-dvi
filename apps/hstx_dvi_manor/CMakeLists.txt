add_executable(hstx_dvi_manor
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mm_gfx.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mm_art.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mm_tiles.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mm_map.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mm_camera.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mm_body.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mm_thing.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mm_witch.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mm_hud.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mm_demo.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mm_game.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mm_hiscore.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mm_attract.c
)

target_compile_definitions(hstx_dvi_manor PRIVATE
  PICO_CORE1_STACK_SIZE=0x400
  MODE_BYTES_PER_PIXEL=1
  # Buttons on GPIOs 6 to 11, leaving 2 to 5 for the SD card as in hstx_dvi_cpm
  MM_INP_GPIO_START=6
)

target_link_libraries(hstx_dvi_manor
  pico_stdlib
  pico_multicore
  hardware_dma
  hardware_pio
  pico_sync
  pico_hstx_dvi
  game_common
)

# create map/bin/hex file etc.
pico_add_extra_outputs(hstx_dvi_manor)

pico_enable_stdio_usb(hstx_dvi_manor 1)
pico_enable_stdio_uart(hstx_dvi_manor 0)
