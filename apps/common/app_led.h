#pragma once

#include "pico/stdlib.h"

// Turn on the board's LED, if it has one on a GPIO.
// The pin comes from the board header (e.g. 25 on a Pico 2, 7 on a Feather RP2350).
// Boards without a GPIO LED (e.g. Pico 2 W, where it hangs off the wireless chip) are left alone.
static inline void app_led_on(void) {
#ifdef PICO_DEFAULT_LED_PIN
#ifndef PICO_DEFAULT_LED_PIN_INVERTED
#define PICO_DEFAULT_LED_PIN_INVERTED 0
#endif
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, !PICO_DEFAULT_LED_PIN_INVERTED);
#endif
}
