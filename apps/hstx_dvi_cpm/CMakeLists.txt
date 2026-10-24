
include(${PICO_FOLDER}/CPM-Emulator/cpm_emulator.cmake)

add_executable(hstx_dvi_cpm
  ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
)

# The emulator reports errors with C++ exceptions, which can pass through the Z80 core
set_source_files_properties(
  ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
  ${PICO_FOLDER}/CPM-Emulator/external/Z80/sources/Z80.c
  PROPERTIES COMPILE_FLAGS "-fexceptions")

target_compile_definitions(hstx_dvi_cpm PRIVATE
  PICO_CORE1_STACK_SIZE=0x400
  MODE_BYTES_PER_PIXEL=1
  # The SD card on SPI0, as the Pico DVI Sock leaves GPIOs 2 to 5 free
  SDCARD_PIN_SPI0_SCK=2
  SDCARD_PIN_SPI0_MOSI=3
  SDCARD_PIN_SPI0_MISO=4
  SDCARD_PIN_SPI0_CS=5
  # The directory of the SD card holding CCP-DR.64K and the drives (else the top)
  CPM_DIR="cpm"
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
  cpm_emulator
  sdcard
  fatfs
)

# create map/bin/hex file etc.
pico_add_extra_outputs(hstx_dvi_cpm)

pico_enable_stdio_usb(hstx_dvi_cpm 1)
pico_enable_stdio_uart(hstx_dvi_cpm 0)
