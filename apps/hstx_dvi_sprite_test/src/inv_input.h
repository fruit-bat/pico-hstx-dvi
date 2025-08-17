#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INV_INP_BIT_L 1
#define INV_INP_BIT_R 2
#define INV_INP_BIT_F 4
#define INV_INP_BIT_S 8

void init_inv_input();
uint8_t get_inv_input();

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

#ifdef __cplusplus
}
#endif

