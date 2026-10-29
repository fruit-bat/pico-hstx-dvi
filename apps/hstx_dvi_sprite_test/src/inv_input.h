#pragma once

#include "pico/stdlib.h"
#include "game_input.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INV_INP_BIT_L 1
#define INV_INP_BIT_R 2
#define INV_INP_BIT_F 4
#define INV_INP_BIT_S 8

// Left, right, fire and start on consecutive GPIOs from here
#ifndef INV_INP_GPIO_START
#define INV_INP_GPIO_START 6
#endif
#define INV_INP_GPIO_COUNT 4

bool __force_inline is_inv_input_left(uint8_t input) {
    return (input & INV_INP_BIT_L) != 0;
}
bool __force_inline is_inv_input_right(uint8_t input) {
    return (input & INV_INP_BIT_R) != 0;
}
bool __force_inline is_inv_input_fire(uint8_t input) {
    return (input & INV_INP_BIT_F) != 0;
}
bool __force_inline is_inv_input_start(uint8_t input) {
    return (input & INV_INP_BIT_S) != 0;
}
bool __force_inline is_inv_input_play(uint8_t input) {
    return (input & (INV_INP_BIT_F | INV_INP_BIT_S)) != 0;
}

#ifdef __cplusplus
}
#endif
