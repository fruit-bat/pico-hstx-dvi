// Copyright (c) 2024 fruit-bat.

// The start and end of each frame: the title at the top, the meter above it
// showing how well the renderer is keeping up, and waiting for the display.
#pragma once

#include "el_draw.h"

#ifdef __cplusplus
extern "C" {
#endif

void screen_init(void);
void screen_begin(void);
// Adds the title, and the meter with label for what is shown, then waits
// for the display to take the frame
void screen_end(const char* title, const char* label);

#ifdef __cplusplus
}
#endif
