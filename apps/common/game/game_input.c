#include "game_input.h"
#ifdef GAME_NESPAD
#include "nespad.h"
#endif
// tinyusb's HID codes first, so the PS/2 keyboard's copies are left out
#ifdef GAME_USBKBD
#include "game_usbkbd.h"
#endif
#ifdef GAME_PS2KBD
#include "ps2kbd.h"
#endif

#ifdef GAME_BUTTONS
static uint32_t _first = 0;
static uint32_t _mask = 0;
#endif
static uint16_t _last_input = 0;
// The input bits the keyboard presses
static uint16_t _key_bits = 0;
// and those of keys that went down in the frame
static uint16_t _key_tapped = 0;

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
#ifdef GAME_USBKBD
	usbkbd_init();
#endif
#ifdef GAME_PS2KBD
	ps2kbd_init(PS2KBD_PIO, PS2KBD_DATA_GPIO);
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
	return input | _key_bits;
}

#ifdef GAME_KEYBOARD

#define CHARS 16

static const GameKey_t *_keys = NULL;
static uint32_t _key_count = 0;
// The keys that went down in the frame, a bit for each
static uint32_t _key_pressed[8];
static char _chars[CHARS];
static uint32_t _char_in = 0, _char_out = 0;

// A key that went down, or repeats, with the character it makes, returning
// the input bits it presses
static uint16_t take_key(uint8_t key, char c, bool repeat) {
	// Typed, which the keyboard repeats as a key is held
	const uint32_t next = (_char_in + 1) % CHARS;
	if (c && next != _char_out) {
		_chars[_char_in] = c;
		_char_in = next;
	}
	// but goes down only once
	if (repeat) return 0;
	_key_pressed[key / 32] |= 1u << (key % 32);
	uint16_t tapped = 0;
	for (uint32_t k = 0; k < _key_count; ++k) {
		if (_keys[k].key == key) tapped |= _keys[k].bits;
	}
	return tapped;
}

// Takes what the keyboards sent: the keys that went down, and what was typed,
// returning the input bits they press, so a key tapped within a frame counts
static uint16_t take_keys(void) {
	for (uint32_t i = 0; i < 8; ++i) _key_pressed[i] = 0;
	uint16_t tapped = 0;
#ifdef GAME_PS2KBD
	ps2kbd_tick();
	Ps2KbdEvent_t e;
	while (ps2kbd_event(&e)) {
		if (e.pressed) tapped |= take_key(e.key, ps2kbd_ascii(e.key, e.modifiers), e.repeat);
	}
#endif
#ifdef GAME_USBKBD
	usbkbd_tick();
	UsbKbdEvent_t u;
	while (usbkbd_event(&u)) {
		if (u.pressed) tapped |= take_key(u.key, usbkbd_ascii(u.key, u.modifiers), u.repeat);
	}
#endif
	_key_bits = 0;
	for (uint32_t k = 0; k < _key_count; ++k) {
		if (game_key_held(_keys[k].key)) _key_bits |= _keys[k].bits;
	}
	_key_tapped = tapped;
	return tapped;
}

void game_input_set_keys(const GameKey_t *keys, uint32_t count) {
	_keys = keys;
	_key_count = count;
}

bool game_keyboard(void) {
	return true;
}

bool game_key_held(uint8_t key) {
#ifdef GAME_PS2KBD
	if (ps2kbd_held(key)) return true;
#endif
#ifdef GAME_USBKBD
	if (usbkbd_held(key)) return true;
#endif
	return false;
}

bool game_key_pressed(uint8_t key) {
	return _key_pressed[key / 32] & (1u << (key % 32));
}

uint8_t game_key_modifiers(void) {
	uint8_t modifiers = 0;
#ifdef GAME_PS2KBD
	modifiers |= ps2kbd_modifiers();
#endif
#ifdef GAME_USBKBD
	modifiers |= usbkbd_modifiers();
#endif
	return modifiers;
}

char game_key_char(void) {
	if (_char_out == _char_in) return 0;
	const char c = _chars[_char_out];
	_char_out = (_char_out + 1) % CHARS;
	return c;
}

#else

static uint16_t take_keys(void) {
	return 0;
}

void game_input_set_keys(const GameKey_t *keys, uint32_t count) {
	(void)keys;
	(void)count;
}

bool game_keyboard(void) {
	return false;
}

bool game_key_held(uint8_t key) {
	(void)key;
	return false;
}

bool game_key_pressed(uint8_t key) {
	(void)key;
	return false;
}

uint8_t game_key_modifiers(void) {
	return 0;
}

char game_key_char(void) {
	return 0;
}

#endif

uint16_t game_input_keys(void) {
	return _key_bits | _key_tapped;
}

uint16_t game_input_pressed(void) {
	const uint16_t tapped = take_keys();
	const uint16_t input = game_input_get();
	const uint16_t pressed = (input & ~_last_input) | tapped;
	_last_input = input;
	return pressed;
}
