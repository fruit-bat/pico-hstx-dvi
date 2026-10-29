#include "game_input.h"

static uint32_t _first = 0;
static uint32_t _mask = 0;
static uint8_t _last_input = 0;

void game_input_init(uint32_t first, uint32_t count) {
	_first = first;
	_mask = (1u << count) - 1;
	for(uint i = 0; i < count; ++i) {
		// Set the GPIO pin to input, with a pull up resistor
		const uint32_t gpio = first + i;
		gpio_init(gpio);
		gpio_set_dir(gpio, GPIO_IN);
		gpio_pull_up(gpio);
	}
}

uint8_t __not_in_flash_func(game_input_get)(void) {
	const uint32_t all_gpio = gpio_get_all();
	return (~all_gpio >> _first) & _mask;
}

uint8_t game_input_pressed(void) {
	const uint8_t input = game_input_get();
	const uint8_t pressed = input & ~_last_input;
	_last_input = input;
	return pressed;
}
