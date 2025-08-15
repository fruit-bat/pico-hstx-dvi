#include "inv_input.h"

#define INV_INP_GPIO_START 2
#define INV_INP_GPIO_COUNT 4

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
