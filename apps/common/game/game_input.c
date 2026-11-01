#include "game_input.h"
#ifdef GAME_NESPAD
#include "nespad.h"
#endif

#ifdef GAME_BUTTONS
static uint32_t _first = 0;
static uint32_t _mask = 0;
#endif
static uint16_t _last_input = 0;

#ifdef GAME_NESPAD
static const uint16_t *_pad_map = NULL;
// The bits a pad can press, which can be more than there are GPIOs
static uint32_t _pad_mask = 0;

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
	game_input_init_pad(first, count, pad_map, count);
}

void game_input_init_pad(uint32_t first, uint32_t count, const uint16_t *pad_map, uint32_t pad_count) {
#ifdef GAME_BUTTONS
	_first = first;
	_mask = (1u << count) - 1;
	for(uint i = 0; i < count; ++i) {
		// Set the GPIO pin to input, with a pull up resistor
		const uint32_t gpio = first + i;
		gpio_init(gpio);
		gpio_set_dir(gpio, GPIO_IN);
		gpio_pull_up(gpio);
	}
#else
	(void)first;
	(void)count;
#endif
#ifdef GAME_NESPAD
	_pad_mask = (1u << pad_count) - 1;
	_pad_map = pad_map;
	nespad_default_init();
#else
	(void)pad_count;
	(void)pad_map;
#endif
}

uint16_t __not_in_flash_func(game_input_get)(void) {
	uint16_t input = 0;
#ifdef GAME_BUTTONS
	input = (~gpio_get_all() >> _first) & _mask;
#endif
#ifdef GAME_NESPAD
	const uint32_t buttons = pad_buttons();
	for (uint i = 0; (_pad_mask >> i) != 0; ++i) {
		if (buttons & _pad_map[i]) input |= 1u << i;
	}
#endif
	return input;
}

uint16_t game_input_pressed(void) {
	const uint16_t input = game_input_get();
	const uint16_t pressed = input & ~_last_input;
	_last_input = input;
	return pressed;
}
