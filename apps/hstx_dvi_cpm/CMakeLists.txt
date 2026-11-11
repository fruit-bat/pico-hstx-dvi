
include(${PICO_FOLDER}/CPM-Emulator/cpm_emulator.cmake)

add_executable(hstx_dvi_cpm
  ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/cpm_settings.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/z80_disasm.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/cpm_aux.cpp
)

# The emulator reports errors with C++ exceptions, which can pass through the Z80 core
set_source_files_properties(
  ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/cpm_settings.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/cpm_aux.cpp
  ${PICO_FOLDER}/CPM-Emulator/external/Z80/sources/Z80.c
  PROPERTIES COMPILE_FLAGS "-fexceptions")

# The terminal's font: terminus_8x16 gives 80 x 30 with line drawing; 8x8
# gives 80 x 60, ASCII only. Any of the fonts in src/hstx_dvi_font.h will do,
# e.g. cmake -DCPM_VT_FONT=8x8 .. If the one chosen isn't built (see the top
# CMakeLists.txt), the 8x8 font is used.
set(CPM_VT_FONT "terminus_8x16" CACHE STRING "The CP/M terminal's font: 8x8, terminus_8x14, terminus_8x16, terminus_12x24, terminus_16x32 or vga_8x16")

# The Z80's speed to start with, in kHz: 0 is as fast as it goes, 4000 a Z80A
set(CPM_Z80_KHZ 0 CACHE STRING "The CP/M Z80's speed to start with, in kHz (0: as fast as it goes)")

# CP/M's reader and punch (RDR: and PUN:, e.g. PIP PUN:=FILE.TXT) on UART1, at
# 3.3V: TX on GPIO 8, RX on GPIO 9. OFF leaves them as before: the punch
# discards, the reader gives end of file.
option(CPM_AUX "CP/M's reader and punch on UART1" ON)
set(CPM_AUX_BAUD 115200 CACHE STRING "The baud rate of CP/M's reader and punch")
set(CPM_AUX_TX_PIN 8 CACHE STRING "UART1's TX GPIO for CP/M's punch (4, 8, 20 or 24)")
set(CPM_AUX_RX_PIN 9 CACHE STRING "UART1's RX GPIO for CP/M's reader (5, 9, 21 or 25)")

# The terminal CP/M programs are set up for, whose codes are turned into the
# screen's (ANSI) ones: mixed (Kaypro and CP/M-86 together), kaypro, vt52 or
# ansi (passed on as they are). The settings screen (T) changes it.
set(CPM_TERMINAL "mixed" CACHE STRING "The terminal CP/M programs are set up for: mixed, kaypro, vt52 or ansi")
string(TOUPPER "${CPM_TERMINAL}" _cpm_terminal)
if (NOT _cpm_terminal MATCHES "^(MIXED|KAYPRO|VT52|ANSI)$")
  message(FATAL_ERROR "CPM_TERMINAL ${CPM_TERMINAL}: mixed, kaypro, vt52 or ansi")
endif()

target_compile_definitions(hstx_dvi_cpm PRIVATE
  PICO_CORE1_STACK_SIZE=0x400
  VT_FONT="${CPM_VT_FONT}"
  MODE_BYTES_PER_PIXEL=1
  # The SD card on SPI0, as the Pico DVI Sock leaves GPIOs 2 to 5 free
  SDCARD_PIN_SPI0_SCK=2
  SDCARD_PIN_SPI0_MOSI=3
  SDCARD_PIN_SPI0_MISO=4
  SDCARD_PIN_SPI0_CS=5
  # The directory of the SD card holding CCP-DR.64K and the drives (else the top)
  CPM_DIR="cpm"
  # The Z80's speed to start with, in kHz (0: as fast as it goes). The
  # settings screen (Ctrl-\ or F12) changes it while running.
  CPM_Z80_KHZ=${CPM_Z80_KHZ}
  CPM_TERMINAL=CONSOLE_${_cpm_terminal}
  $<$<BOOL:${CPM_AUX}>:CPM_AUX=1>
  CPM_AUX_BAUD=${CPM_AUX_BAUD}
  CPM_AUX_TX_PIN=${CPM_AUX_TX_PIN}
  CPM_AUX_RX_PIN=${CPM_AUX_RX_PIN}
)

target_include_directories(hstx_dvi_cpm PRIVATE
  # host_console.h
  ${CMAKE_CURRENT_LIST_DIR}/src
  # host_fs.h: the drives on the SD card
  ${PICO_FOLDER}/CPM-Emulator/sources/host/fatfs
)

target_link_libraries(hstx_dvi_cpm
  pico_stdlib
  pico_multicore
  hardware_dma
  hardware_pio
  pico_sync
  pico_hstx_dvi
  pico_hstx_dvi_vt
  pico_vtlib
  hardware_uart
  cpm_emulator
  sdcard
  fatfs
)

# A USB keyboard, with GAME_USBKBD
if (GAME_USBKBD)
  target_link_libraries(hstx_dvi_cpm game_usbkbd)
endif()

# create map/bin/hex file etc.
pico_add_extra_outputs(hstx_dvi_cpm)

pico_enable_stdio_usb(hstx_dvi_cpm 1)
pico_enable_stdio_uart(hstx_dvi_cpm 0)
