// Copyright (c) 2024 fruit-bat.
#pragma once

#include "pico/stdlib.h"
#include "game_input.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RR_INP_BIT_L 1
#define RR_INP_BIT_R 2
#define RR_INP_BIT_U 4
#define RR_INP_BIT_D 8
#define RR_INP_BIT_F 16      // fire; held, charges the beam
#define RR_INP_BIT_B 32      // bomb
#define RR_INP_BIT_START 64  // settings between games, pause in a game: on a pad only
#define RR_INP_BIT_POWER 128 // takes the power up lit on the bar: on a pad only

// Left, right, up, down, fire and bomb on consecutive GPIOs from here
#ifndef RR_INP_GPIO_START
#define RR_INP_GPIO_START 2
#endif
#define RR_INP_GPIO_COUNT 6
// And start and power, from a pad
#define RR_INP_PAD_COUNT 8

// The gamepad buttons for each input bit: power on select or either shoulder
#define RR_INP_PAD_MAP { GAME_PAD_LEFT, GAME_PAD_RIGHT, GAME_PAD_UP, GAME_PAD_DOWN, GAME_PAD_A, GAME_PAD_B, \
	GAME_PAD_START, GAME_PAD_SELECT | GAME_PAD_L | GAME_PAD_R }

static inline bool is_rr_input_play(const uint16_t input) {
	return (input & (RR_INP_BIT_F | RR_INP_BIT_B)) != 0;
}

#ifdef __cplusplus
}
#endif
