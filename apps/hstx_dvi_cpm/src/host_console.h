// Copyright (c) 2026 fruit-bat

#pragma once

// Host console for the CP/M emulator: output to all the stdio drivers (the DVI
// screen and the serial port), keys from stdin (the serial port) and, built
// with GAME_USBKBD, a USB keyboard.

#include "pico/stdlib.h"
#ifdef GAME_USBKBD
#include "game_usbkbd.h"
#endif

// The key that opens the settings screen: Ctrl-\ (F12 on a USB keyboard
// too). CP/M programs don't see it.
#define CONCTRL_SETTINGS_KEY 0x1c

// Opens the settings screen (cpm_settings.h), if set
inline void (*conctrl_settings)() = nullptr;

// A key read ahead by conctrl_status(), or -1
inline int conctrl_key = -1;

// Whether CP/M is waiting for a key, for the settings screen to say so
inline bool conctrl_waiting = false;

inline void conctrl_init() {}

// CP/M sends its own CR LF, so no translation
inline void conctrl_putc(const char c) {
    putchar_raw(c);
}

#ifdef GAME_USBKBD
// The character a key makes for CP/M: Ctrl with a letter its control code,
// the arrows WordStar's ^E ^X ^S ^D, Page Up and Down its ^R ^C, Delete DEL,
// F12 the settings key; or -1 if none
inline int conctrl_usb_char(const UsbKbdEvent_t& e) {
    const bool ctrl = e.modifiers & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL);
    switch (e.key) {
        case HID_KEY_ARROW_UP: return 'E' & 0x1f;
        case HID_KEY_ARROW_DOWN: return 'X' & 0x1f;
        case HID_KEY_ARROW_LEFT: return 'S' & 0x1f;
        case HID_KEY_ARROW_RIGHT: return 'D' & 0x1f;
        case HID_KEY_PAGE_UP: return 'R' & 0x1f;
        case HID_KEY_PAGE_DOWN: return 'C' & 0x1f;
        case HID_KEY_DELETE: return 0x7f;
        case HID_KEY_F12: return CONCTRL_SETTINGS_KEY;
        default: break;
    }
    const char c = usbkbd_ascii(e.key, e.modifiers);
    if (c == 0) return -1;
    if (ctrl && c >= '@' && c <= 0x7e) return c & 0x1f;
    return c;
}

// The next key typed on the USB keyboard, or -1
inline int conctrl_usb_key() {
    usbkbd_tick();
    UsbKbdEvent_t e;
    while (usbkbd_event(&e)) {
        if (!e.pressed) continue;
        const int c = conctrl_usb_char(e);
        if (c >= 0) return c;
    }
    return -1;
}
#endif

// The next key from the keyboard or the serial port, or -1
inline int conctrl_poll_raw() {
#ifdef GAME_USBKBD
    const int k = conctrl_usb_key();
    if (k >= 0) return k;
#endif
    const int c = getchar_timeout_us(0);
    return c == PICO_ERROR_TIMEOUT ? -1 : c;
}

// The next key for CP/M, or -1. The settings key opens the settings screen.
inline int conctrl_poll() {
    const int c = conctrl_poll_raw();
    if (c == CONCTRL_SETTINGS_KEY && conctrl_settings) {
        conctrl_settings();
        return -1;
    }
    return c;
}

inline int conctrl_status() {
    if (conctrl_key < 0) conctrl_key = conctrl_poll();
    return conctrl_key >= 0 ? 0xFF : 0x00;
}

inline int conctrl_kbhit() {
    return conctrl_status();
}

inline int conctrl_getch_wait() {
    int c = conctrl_key;
    conctrl_key = -1;
    conctrl_waiting = true;
    while (c < 0) c = conctrl_poll();
    conctrl_waiting = false;
    return c == '\n' ? '\r' : c;
}

inline int conctrl_getch() {
    return conctrl_status() ? conctrl_getch_wait() : 0;
}
