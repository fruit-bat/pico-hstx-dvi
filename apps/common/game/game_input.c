#include "game_input.h"
#ifdef GAME_NESPAD
#include "nespad.h"
#endif

static uint32_t _first = 0;
static uint32_t _mask = 0;
static uint8_t _last_input = 0;

#ifdef GAME_NESPAD
static const uint16_t *_pad_map = NULL;

// The buttons held on either pad, one bit for each of the 16 a pad sends
static uint32_t __not_in_flash_func(pad_buttons)(void) {
	const uint32_t state = nespad_state();
	uint32_t buttons = 0;
	for (uint pad = 0; pad < 2; ++pad) {
		uint32_t b = 0;
		for (uint i = 0; i < 16; ++i) {
			b |= ((state >> (2 * i + pad)) & 1) << i;
		}
		// A SNES pad sends its last 4 bits as not pressed, but some NES pads
		// send everything after the first 8 as pressed
		if ((b & 0xf000) == 0xf000) b &= 0xff;
		buttons |= b;
	}
	return buttons;
}
#endif

void game_input_init(uint32_t first, uint32_t count, const uint16_t *pad_map) {
	_first = first;
	_mask = (1u << count) - 1;
	for(uint i = 0; i < count; ++i) {
		// Set the GPIO pin to input, with a pull up resistor
		const uint32_t gpio = first + i;
		gpio_init(gpio);
		gpio_set_dir(gpio, GPIO_IN);
		gpio_pull_up(gpio);
	}
#ifdef GAME_NESPAD
	_pad_map = pad_map;
	nespad_default_init();
#endif
}

uint8_t __not_in_flash_func(game_input_get)(void) {
	const uint32_t all_gpio = gpio_get_all();
	uint8_t input = (~all_gpio >> _first) & _mask;
#ifdef GAME_NESPAD
	const uint32_t buttons = pad_buttons();
	for (uint i = 0; (_mask >> i) != 0; ++i) {
		if (buttons & _pad_map[i]) input |= 1u << i;
	}
#endif
	return input;
}

uint8_t game_input_pressed(void) {
	const uint8_t input = game_input_get();
	const uint8_t pressed = input & ~_last_input;
	_last_input = input;
	return pressed;
}
