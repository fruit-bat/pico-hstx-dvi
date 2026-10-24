cmake_minimum_required(VERSION 3.12)

# HSTX is only on the RP2350, so build for a Pico 2 unless told otherwise,
# e.g. cmake -DPICO_BOARD=pico2_w ..
if (NOT DEFINED PICO_BOARD AND NOT DEFINED ENV{PICO_BOARD})
    set(PICO_BOARD pico2)
endif()

# Pull in PICO SDK (must be before project)
include(pico_sdk_import.cmake)

# We also need PICO EXTRAS
include(pico_extras_import.cmake)

project(pico_hstx_dvi C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if (PICO_SDK_VERSION_STRING VERSION_LESS "2.0.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.0.0 (or later) required. Current version is ${PICO_SDK_VERSION_STRING}")
endif()

pico_sdk_init()

if (PICO_PLATFORM STREQUAL "rp2040")
    message(FATAL_ERROR "HSTX needs an RP2350 board, but PICO_BOARD '${PICO_BOARD}' is an RP2040")
endif()

add_compile_options(-Wall)

# Helpers shared by the apps
include_directories(${CMAKE_CURRENT_LIST_DIR}/common)

get_filename_component(PICO_FOLDER ${CMAKE_CURRENT_LIST_DIR}/../.. ABSOLUTE)

add_subdirectory(${PICO_FOLDER}/pico-hstx-dvi "${CMAKE_CURRENT_BINARY_DIR}/pico-hstx-dvi")

# SD card & FAT file system
include(${PICO_FOLDER}/pimoroni-pico/drivers/fatfs/fatfs.cmake)
include(${PICO_FOLDER}/pimoroni-pico/drivers/sdcard/sdcard.cmake)

add_subdirectory(hstx_dvi_hello_world  "${CMAKE_CURRENT_BINARY_DIR}/hstx_dvi_hello_world")
add_subdirectory(hstx_dvi_row_fifo_test  "${CMAKE_CURRENT_BINARY_DIR}/hstx_dvi_row_fifo_test")
add_subdirectory(hstx_dvi_text_test  "${CMAKE_CURRENT_BINARY_DIR}/hstx_dvi_text_test")
add_subdirectory(hstx_dvi_tmt_test  "${CMAKE_CURRENT_BINARY_DIR}/hstx_dvi_tmt_test")
add_subdirectory(hstx_dvi_vt_test  "${CMAKE_CURRENT_BINARY_DIR}/hstx_dvi_vt_test")
add_subdirectory(hstx_dvi_sprite_test  "${CMAKE_CURRENT_BINARY_DIR}/hstx_dvi_sprite_test")
add_subdirectory(hstx_dvi_lisp_test  "${CMAKE_CURRENT_BINARY_DIR}/hstx_dvi_lisp_test")
add_subdirectory(hstx_dvi_cpm  "${CMAKE_CURRENT_BINARY_DIR}/hstx_dvi_cpm")
