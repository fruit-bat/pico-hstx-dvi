// A USB keyboard on the Pico's own USB port, as a host, when built with
// GAME_USBKBD, using tinyusb and pico-hid-host. It is read the way the PS/2
// keyboard is, with HID key codes, so game_input treats the two the same.
//
// The port is then a host, so it can't also be the serial port: stdio goes to
// the UART on GPIOs 0 and 1 instead. The keyboard needs 5 V on the Pico's
// VBUS pin, as the USB port doesn't power it otherwise.
#pragma once

#include "tusb.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// A key going down or up, with the modifiers held after it. Held, a key
// repeats, as a PS/2 keyboard repeats it, coming as pressed again with repeat
// set.
typedef struct {
	uint8_t key;       // HID_KEY_...
	uint8_t modifiers; // KEYBOARD_MODIFIER_...
	bool pressed;
	bool repeat;
} UsbKbdEvent_t;

// Starts the USB host
void usbkbd_init(void);
// Runs the USB host, taking what the keyboard has sent
void usbkbd_tick(void);
// The next key event, returning false if there are none
bool usbkbd_event(UsbKbdEvent_t* e);
// Whether a key is held now
bool usbkbd_held(uint8_t key);
// The modifiers held now
uint8_t usbkbd_modifiers(void);
// The character a key makes with the modifiers and Caps Lock, on a UK
// keyboard, or 0 if none; Enter is '\r', Backspace '\b', Tab '\t', Escape 27
char usbkbd_ascii(uint8_t key, uint8_t modifiers);

#ifdef __cplusplus
}
#endif
