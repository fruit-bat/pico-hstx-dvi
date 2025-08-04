add_library(pico_hstx_dvi INTERFACE)

target_sources(pico_hstx_dvi INTERFACE
//  ${CMAKE_CURRENT_LIST_DIR}/src/nespad.c
)

target_include_directories(pico_hstx_dvi INTERFACE
    src
)
