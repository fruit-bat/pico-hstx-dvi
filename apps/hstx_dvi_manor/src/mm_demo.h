#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void mm_demo_reset(void);
// The buttons the demo player is pressing
uint8_t mm_demo_input(uint32_t frame);
// How far along its route it is
uint32_t mm_demo_step(void);

#ifdef __cplusplus
}
#endif
