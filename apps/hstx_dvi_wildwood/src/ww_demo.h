#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void ww_demo_reset(void);
// The buttons the demo player is pressing
uint8_t ww_demo_input(uint32_t frame);

#ifdef __cplusplus
}
#endif
