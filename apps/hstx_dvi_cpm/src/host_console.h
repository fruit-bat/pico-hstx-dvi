// Copyright (c) 2026 fruit-bat

#pragma once

// Host console for the CP/M emulator: output to all the stdio drivers (the DVI
// screen and the USB serial port), keys from stdin (the USB serial port).

#include "pico/stdlib.h"

// A key read ahead by conctrl_status(), or -1
inline int conctrl_key = -1;

inline void conctrl_init() {}

// CP/M sends its own CR LF, so no translation
inline void conctrl_putc(const char c) {
    putchar_raw(c);
}

inline int conctrl_status() {
    if (conctrl_key < 0) conctrl_key = getchar_timeout_us(0);
    return conctrl_key >= 0 ? 0xFF : 0x00;
}

inline int conctrl_kbhit() {
    return conctrl_status();
}

inline int conctrl_getch_wait() {
    int c = conctrl_key;
    conctrl_key = -1;
    if (c < 0) c = getchar();
    return c == '\n' ? '\r' : c;
}

inline int conctrl_getch() {
    return conctrl_status() ? conctrl_getch_wait() : 0;
}
