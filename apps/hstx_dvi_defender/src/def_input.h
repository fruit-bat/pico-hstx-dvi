#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEF_INP_BIT_L 1
#define DEF_INP_BIT_R 2
#define DEF_INP_BIT_U 4
#define DEF_INP_BIT_D 8
#define DEF_INP_BIT_F 16
#define DEF_INP_BIT_B 32 // Smart bomb

void def_input_init(void);

uint8_t def_input_get(void);

// Buttons that have gone down since the last call
uint8_t def_input_pressed(void);

__force_inline bool is_def_input_play(const uint8_t input) {
	return (input & (DEF_INP_BIT_F | DEF_INP_BIT_B)) != 0;
}

#ifdef __cplusplus
}
#endif
