#pragma once

#include "pico/stdlib.h"

// Each game has a scenario.c that drives it, with these

// The buttons held down at a frame, with the first button in bit 0
uint32_t host_input(uint32_t frame);

// Called at the end of each frame, with the number of sprites enabled, for
// keeping track of how the game is going
void host_frame(uint32_t frame, uint32_t sprites);

// The scenario chosen on the command line
extern const char* host_mode;
