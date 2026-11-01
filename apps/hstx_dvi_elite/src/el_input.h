// Copyright (c) 2024 fruit-bat.

// The buttons: a NES or SNES pad, or buttons on GPIOs.
#pragma once

#include "pico/stdlib.h"
#include "game_input.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EL_INP_LEFT   1
#define EL_INP_RIGHT  2
#define EL_INP_UP     4
#define EL_INP_DOWN   8
#define EL_INP_A      16 // Fire
#define EL_INP_B      32 // Held with up or down for speed
#define EL_INP_SELECT 64 // The next screen
#define EL_INP_START  128 // Options
#define EL_INP_L      256 // The screen before, from a SNES pad's shoulder
#define EL_INP_R      512 // The next screen
#define EL_INP_X      1024 // The docking computer, from a SNES pad's X
#define EL_INP_BOMB   2048 // The energy bomb, from a SNES pad's A, which is not fire

// The buttons on consecutive GPIOs from here. 12 to 19 are the display's.
// With an SD card on 2 to 5 there is only room for six, from 6 to 11, and
// select and start come from a pad.
#if defined(EL_SDCARD)
#ifndef EL_INP_GPIO_START
#define EL_INP_GPIO_START 6
#endif
#define EL_INP_GPIO_COUNT 6
#else
#ifndef EL_INP_GPIO_START
#define EL_INP_GPIO_START 4
#endif
#define EL_INP_GPIO_COUNT 8
#endif
// A pad presses all twelve, the last four only a SNES pad
#define EL_INP_PAD_COUNT 12

// The gamepad buttons for each input bit
#define EL_INP_PAD_MAP { \
	GAME_PAD_LEFT, GAME_PAD_RIGHT, GAME_PAD_UP, GAME_PAD_DOWN, \
	GAME_PAD_A & ~GAME_PAD_SNES_A, GAME_PAD_B & ~GAME_PAD_SNES_X, GAME_PAD_SELECT, GAME_PAD_START, \
	GAME_PAD_L, GAME_PAD_R, GAME_PAD_SNES_X, GAME_PAD_SNES_A }

#ifdef __cplusplus
}
#endif
