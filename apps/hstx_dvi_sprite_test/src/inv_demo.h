#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void inv_demo_init(void);

// Works out what buttons the demo player would press this frame
uint8_t inv_demo_input(uint32_t frame);

#ifdef __cplusplus
}
#endif
