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
  ${CMAKE_CURRENT_LIST_DIR}/game_seq.c
  ${CMAKE_CURRENT_LIST_DIR}/game_sound.c
  ${CMAKE_CURRENT_LIST_DIR}/game_synth.c
  ${CMAKE_CURRENT_LIST_DIR}/game_synth_uart.c
  ${CMAKE_CURRENT_LIST_DIR}/game_synth_file.c
)

target_include_directories(game_common INTERFACE ${CMAKE_CURRENT_LIST_DIR})

if (GAME_BUTTONS)
  target_compile_definitions(game_common INTERFACE GAME_BUTTONS=1)
endif()

if (GAME_MIDI)
  target_compile_definitions(game_common INTERFACE
    GAME_MIDI=1
    GAME_MIDI_UART=uart1
    GAME_MIDI_TX_GPIO=8
  )
  target_link_libraries(game_common INTERFACE hardware_uart hardware_irq)
endif()

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

if (GAME_PS2KBD)
  target_compile_definitions(game_common INTERFACE
    GAME_PS2KBD=1
    # Beside the pads
    PS2KBD_PIO=pio1
    # DATA on GPIO 0 and CLOCK on 1, free in every game
    PS2KBD_DATA_GPIO=0
  )
  if (GAME_PS2KBD_DEBUG)
    target_compile_definitions(game_common INTERFACE PS2KBD_DEBUG=1)
  endif()
  target_link_libraries(game_common INTERFACE pico_ps2_keyboard)
endif()

# The USB keyboard on its own, for apps that aren't games, such as hstx_dvi_cpm
if (GAME_USBKBD)
  add_library(game_usbkbd INTERFACE)
  target_sources(game_usbkbd INTERFACE ${CMAKE_CURRENT_LIST_DIR}/game_usbkbd.c)
  target_include_directories(game_usbkbd INTERFACE ${CMAKE_CURRENT_LIST_DIR})
  target_compile_definitions(game_usbkbd INTERFACE GAME_USBKBD=1)
  target_link_libraries(game_usbkbd INTERFACE pico_hid_host tinyusb_host tinyusb_board)
  target_link_libraries(game_common INTERFACE game_usbkbd)
endif()

# Either keyboard
if (GAME_PS2KBD OR GAME_USBKBD)
  target_compile_definitions(game_common INTERFACE GAME_KEYBOARD=1)
endif()
