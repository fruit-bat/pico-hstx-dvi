add_executable(hstx_dvi_riftrunner
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rr_art.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rr_gfx.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rr_world.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rr_fx.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rr_hud.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rr_player.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rr_enemy.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rr_boss.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rr_demo.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rr_game.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rr_attract.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rr_sound.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rr_sound_data.c
)

target_compile_definitions(hstx_dvi_riftrunner PRIVATE
  PICO_CORE1_STACK_SIZE=0x400
  MODE_BYTES_PER_PIXEL=1
  # Buttons, if built with GAME_BUTTONS, on GPIOs 2 to 7, clear of the MIDI
  # synth on 8
  RR_INP_GPIO_START=2
  # Prints how far ahead of the display the renderer keeps, each second
  HSTX_DVI_SPRITE_TIMING=1
)

target_link_libraries(hstx_dvi_riftrunner
  pico_stdlib
  pico_multicore
  hardware_dma
  hardware_pio
  pico_sync
  pico_hstx_dvi
  game_common
)

# create map/bin/hex file etc.
pico_add_extra_outputs(hstx_dvi_riftrunner)

pico_enable_stdio_usb(hstx_dvi_riftrunner 1)
pico_enable_stdio_uart(hstx_dvi_riftrunner 0)
