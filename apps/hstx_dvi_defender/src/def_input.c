#include "def_input.h"

// Left, right, up, down, fire and smart bomb on consecutive GPIOs from here
#ifndef DEF_INP_GPIO_START
#define DEF_INP_GPIO_START 6
#endif
#define DEF_INP_GPIO_COUNT 6

static uint8_t _last_input = 0;

void def_input_init(void) {
	for(uint i = 0; i < DEF_INP_GPIO_COUNT; ++i) {
		// Set the GPIO pin to input, with a pull up resistor
		const uint32_t gpio = DEF_INP_GPIO_START + i;
		gpio_init(gpio);
		gpio_set_dir(gpio, GPIO_IN);
		gpio_pull_up(gpio);
	}
}

uint8_t __not_in_flash_func(def_input_get)(void) {
	const uint32_t all_gpio = gpio_get_all();
	return (~all_gpio >> DEF_INP_GPIO_START) & ((1 << DEF_INP_GPIO_COUNT) - 1);
}

uint8_t def_input_pressed(void) {
	const uint8_t input = def_input_get();
	const uint8_t pressed = input & ~_last_input;
	_last_input = input;
	return pressed;
}
