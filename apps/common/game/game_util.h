#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Positions that need to move by less than a pixel a frame are fixed point,
// with this many fraction bits
#define GAME_FP 8
#define GAME_PX(v) ((v) << GAME_FP)

void game_rand_seed(uint32_t seed);

uint32_t game_rand(void);

// Sine of a, where 256 is a full turn, scaled to -127..127
int32_t game_sin(uint32_t a);

__force_inline int32_t game_abs(const int32_t v) {
	return v < 0 ? -v : v;
}

__force_inline int32_t game_sign(const int32_t v) {
	return (v > 0) - (v < 0);
}

__force_inline int32_t game_clamp(const int32_t v, const int32_t lo, const int32_t hi) {
	return v < lo ? lo : v > hi ? hi : v;
}

#ifdef __cplusplus
}
#endif
