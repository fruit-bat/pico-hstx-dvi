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
