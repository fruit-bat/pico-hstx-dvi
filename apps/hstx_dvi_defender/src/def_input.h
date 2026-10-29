#pragma once

#include "pico/stdlib.h"
#include "game_input.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEF_INP_BIT_L 1
#define DEF_INP_BIT_R 2
#define DEF_INP_BIT_U 4
#define DEF_INP_BIT_D 8
#define DEF_INP_BIT_F 16
#define DEF_INP_BIT_B 32 // Smart bomb

// Left, right, up, down, fire and smart bomb on consecutive GPIOs from here
#ifndef DEF_INP_GPIO_START
#define DEF_INP_GPIO_START 6
#endif
#define DEF_INP_GPIO_COUNT 6

__force_inline bool is_def_input_play(const uint8_t input) {
	return (input & (DEF_INP_BIT_F | DEF_INP_BIT_B)) != 0;
}

#ifdef __cplusplus
}
#endif
