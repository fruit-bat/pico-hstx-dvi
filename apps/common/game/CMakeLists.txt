# Bits and pieces shared by the games. Built as part of each game, so they
# pick up its settings such as MODE_BYTES_PER_PIXEL.
add_library(game_common INTERFACE)

target_sources(game_common INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/game_util.c
  ${CMAKE_CURRENT_LIST_DIR}/game_input.c
  ${CMAKE_CURRENT_LIST_DIR}/game_pallet.c
  ${CMAKE_CURRENT_LIST_DIR}/game_render.c
  ${CMAKE_CURRENT_LIST_DIR}/game_art.c
  ${CMAKE_CURRENT_LIST_DIR}/game_fx.c
  ${CMAKE_CURRENT_LIST_DIR}/game_hiscore.c
  ${CMAKE_CURRENT_LIST_DIR}/game_initials.c
)

target_include_directories(game_common INTERFACE ${CMAKE_CURRENT_LIST_DIR})

if (GAME_NESPAD)
  target_compile_definitions(game_common INTERFACE
    GAME_NESPAD=1
    # The display uses pio0
    NESPAD_PIO=pio1
    # DATA for pads 1 and 2 on GPIOs 20 and 21, CLK and LATCH on 26 and 27,
    # clear of the HSTX on 12 to 19
    NESPAD_DATA_PIN_BASE=20
    NESPAD_CLK_PIN_BASE=26
  )
  target_link_libraries(game_common INTERFACE pico_nespad)
endif()
