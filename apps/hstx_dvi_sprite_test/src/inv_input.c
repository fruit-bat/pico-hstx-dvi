#include "inv_input.h"

// Left, right, fire and start on consecutive GPIOs from here
#ifndef INV_INP_GPIO_START
#define INV_INP_GPIO_START 6
#endif
#define INV_INP_GPIO_COUNT 4

static uint8_t _last_input = 0;

void init_inv_input() {
    for(uint i = 0; i < INV_INP_GPIO_COUNT; ++i) {
        // Set the GPIO pin to input, with a pull up resistor
        const uint32_t gpio = INV_INP_GPIO_START + i;
        gpio_init(gpio);
        gpio_set_dir(gpio, GPIO_IN);
        gpio_pull_up(gpio);
    }
}

uint8_t __not_in_flash_func(get_inv_input)() {
    uint32_t all_gpio = gpio_get_all();
    return (~all_gpio >> INV_INP_GPIO_START) & ((1 << INV_INP_GPIO_COUNT) - 1);
}

// Buttons that have gone down since the last call
uint8_t inv_input_pressed() {
    const uint8_t input = get_inv_input();
    const uint8_t pressed = input & ~_last_input;
    _last_input = input;
    return pressed;
}
