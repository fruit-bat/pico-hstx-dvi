#pragma once

#include "pico/stdlib.h"
#include "game_input.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INV_INP_BIT_L 1
#define INV_INP_BIT_R 2
#define INV_INP_BIT_U 4 // Up and down only for the settings
#define INV_INP_BIT_D 8
#define INV_INP_BIT_F 16
#define INV_INP_BIT_S 32 // The settings between games, done putting in initials

// Left, right, up, down, fire and start on consecutive GPIOs from here. Up
// lands on the MIDI synth's GPIO 8, so with MIDI built in it comes only from
// a pad.
#ifndef INV_INP_GPIO_START
#define INV_INP_GPIO_START 6
#endif
#define INV_INP_GPIO_COUNT 6

// The gamepad buttons for each input bit
#define INV_INP_PAD_MAP { GAME_PAD_LEFT, GAME_PAD_RIGHT, GAME_PAD_UP, GAME_PAD_DOWN, GAME_PAD_A | GAME_PAD_B, GAME_PAD_START }

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
    return (input & INV_INP_BIT_F) != 0;
}

#ifdef __cplusplus
}
#endif
