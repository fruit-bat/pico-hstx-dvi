
add_executable(hstx_dvi_defender
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
  ${CMAKE_CURRENT_LIST_DIR}/src/def_input.c
  ${CMAKE_CURRENT_LIST_DIR}/src/def_util.c
  ${CMAKE_CURRENT_LIST_DIR}/src/def_pallet.c
  ${CMAKE_CURRENT_LIST_DIR}/src/def_render.c
  ${CMAKE_CURRENT_LIST_DIR}/src/def_world.c
  ${CMAKE_CURRENT_LIST_DIR}/src/def_scanner.c
  ${CMAKE_CURRENT_LIST_DIR}/src/def_hud.c
  ${CMAKE_CURRENT_LIST_DIR}/src/def_fx.c
  ${CMAKE_CURRENT_LIST_DIR}/src/def_player.c
  ${CMAKE_CURRENT_LIST_DIR}/src/def_enemy.c
  ${CMAKE_CURRENT_LIST_DIR}/src/def_human.c
  ${CMAKE_CURRENT_LIST_DIR}/src/def_demo.c
  ${CMAKE_CURRENT_LIST_DIR}/src/def_game.c
  ${CMAKE_CURRENT_LIST_DIR}/src/def_hiscore.c
  ${CMAKE_CURRENT_LIST_DIR}/src/def_attract.c
)

target_compile_definitions(hstx_dvi_defender PRIVATE
  PICO_CORE1_STACK_SIZE=0x400
  MODE_BYTES_PER_PIXEL=1
  # Buttons on GPIOs 6 to 11, leaving 2 to 5 for the SD card as in hstx_dvi_cpm
  DEF_INP_GPIO_START=6
)

target_link_libraries(hstx_dvi_defender
  pico_stdlib
  pico_multicore
  hardware_dma
  hardware_pio
  pico_sync
  pico_hstx_dvi
)

# create map/bin/hex file etc.
pico_add_extra_outputs(hstx_dvi_defender)

pico_enable_stdio_usb(hstx_dvi_defender 1)
pico_enable_stdio_uart(hstx_dvi_defender 0)
