#pragma once

#include "pico/stdlib.h"
#include "game_input.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MM_INP_BIT_L 1
#define MM_INP_BIT_R 2
#define MM_INP_BIT_U 4
#define MM_INP_BIT_D 8
#define MM_INP_BIT_F 16 // Cast a spell
#define MM_INP_BIT_J 32 // Jump

// Left, right, up, down, cast and jump on consecutive GPIOs from here
#ifndef MM_INP_GPIO_START
#define MM_INP_GPIO_START 6
#endif
#define MM_INP_GPIO_COUNT 6

__force_inline bool is_mm_input_play(const uint8_t input) {
	return (input & (MM_INP_BIT_F | MM_INP_BIT_J)) != 0;
}

#ifdef __cplusplus
}
#endif
