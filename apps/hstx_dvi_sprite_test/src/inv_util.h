#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void inv_rand_seed(uint32_t seed);

uint32_t inv_rand(void);

// Sine of a, where 256 is a full turn, scaled to -127..127
int32_t inv_sin(uint32_t a);

#ifdef __cplusplus
}
#endif
