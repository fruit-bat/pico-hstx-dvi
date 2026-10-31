#pragma once

#include "pico/stdlib.h"
#include "game_input.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WW_INP_BIT_L 1
#define WW_INP_BIT_R 2
#define WW_INP_BIT_U 4
#define WW_INP_BIT_D 8
#define WW_INP_BIT_F 16 // Cast a spell
#define WW_INP_BIT_B 32 // Tiptoe
#define WW_INP_DIRS (WW_INP_BIT_L | WW_INP_BIT_R | WW_INP_BIT_U | WW_INP_BIT_D)

// Left, right, up, down, cast and tiptoe on consecutive GPIOs from here
#ifndef WW_INP_GPIO_START
#define WW_INP_GPIO_START 6
#endif
#define WW_INP_GPIO_COUNT 6

// The gamepad buttons for each input bit
#define WW_INP_PAD_MAP { GAME_PAD_LEFT, GAME_PAD_RIGHT, GAME_PAD_UP, GAME_PAD_DOWN, GAME_PAD_A, GAME_PAD_B }

__force_inline bool is_ww_input_play(const uint8_t input) {
	return (input & (WW_INP_BIT_F | WW_INP_BIT_B)) != 0;
}

#ifdef __cplusplus
}
#endif
