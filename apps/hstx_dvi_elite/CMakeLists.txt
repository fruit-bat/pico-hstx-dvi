add_executable(hstx_dvi_elite
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
  ${CMAKE_CURRENT_LIST_DIR}/src/el_attract.c
  ${CMAKE_CURRENT_LIST_DIR}/src/el_chart.c
  ${CMAKE_CURRENT_LIST_DIR}/src/el_draw.c
  ${CMAKE_CURRENT_LIST_DIR}/src/el_game.c
  ${CMAKE_CURRENT_LIST_DIR}/src/el_hud.c
  ${CMAKE_CURRENT_LIST_DIR}/src/el_options.c
  ${CMAKE_CURRENT_LIST_DIR}/src/el_save.c
  ${CMAKE_CURRENT_LIST_DIR}/src/el_screen.c
  ${CMAKE_CURRENT_LIST_DIR}/src/el_ships.c
  ${CMAKE_CURRENT_LIST_DIR}/src/el_space.c
  ${CMAKE_CURRENT_LIST_DIR}/src/el_trade.c
  ${CMAKE_CURRENT_LIST_DIR}/src/galaxy.c
  ${CMAKE_CURRENT_LIST_DIR}/src/ship_model.c
)

target_compile_definitions(hstx_dvi_elite PRIVATE
  PICO_CORE1_STACK_SIZE=0x800
  MODE_BYTES_PER_PIXEL=1
  # The meter at the top of the screen, see README.md
  HSTX_DVI_SPRITE_TIMING=1
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

# Saving commanders on an SD card on SPI0, GPIOs 2 to 5, as hstx_dvi_cpm, in
# the directory pico_dvi_elite. The buttons are then on GPIOs 6 to 11, with
# select and start from a pad. Without it the buttons are on GPIOs 4 to 11,
# clear of the display on 12 to 19, and nothing is saved.
option(ELITE_SDCARD "Save Elite commanders on an SD card on GPIOs 2 to 5" OFF)
if (ELITE_SDCARD)
  target_compile_definitions(hstx_dvi_elite PRIVATE
    EL_SDCARD=1
    SDCARD_PIN_SPI0_SCK=2
    SDCARD_PIN_SPI0_MOSI=3
    SDCARD_PIN_SPI0_MISO=4
    SDCARD_PIN_SPI0_CS=5
  )
  target_link_libraries(hstx_dvi_elite sdcard fatfs)
endif()

# create map/bin/hex file etc.
pico_add_extra_outputs(hstx_dvi_elite)

pico_enable_stdio_usb(hstx_dvi_elite 1)
pico_enable_stdio_uart(hstx_dvi_elite 0)
