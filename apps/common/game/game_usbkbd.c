// Only built with GAME_USBKBD, though every file here is built for the PC
#ifdef GAME_USBKBD

#include "game_usbkbd.h"
#include "hid_app.h"
#include "pico/time.h"

#define EVENTS 16
// The keyboard's lock lights, as it takes them in its output report
#define LED_NUM    0x01
#define LED_CAPS   0x02
#define LED_SCROLL 0x04
// How long a key is held before it repeats, and then how often, as a PS/2
// keyboard does by default
#define REPEAT_DELAY_US 500000
#define REPEAT_US 92000

static UsbKbdEvent_t _events[EVENTS];
static uint32_t _event_in = 0, _event_out = 0;
// The keys held, a bit for each
static uint32_t _held[8];
static uint8_t _modifiers = 0;
// The lights, and whether they are still to be sent to the keyboard
static uint8_t _leds = 0;
static bool _leds_due = false;
// The keyboard, if one is plugged in
static bool _mounted = false;
static uint8_t _dev_addr = 0, _instance = 0;
// The key that repeats while held, and when it next does
static uint8_t _repeat_key = HID_KEY_NONE;
static uint32_t _repeat_at = 0;

static void add_event(uint8_t key, bool pressed, bool repeat) {
	const uint32_t next = (_event_in + 1) % EVENTS;
	if (next == _event_out) return;
	_events[_event_in] = (UsbKbdEvent_t){ key, _modifiers, pressed, repeat };
	_event_in = next;
}

static bool in_report(hid_keyboard_report_t const *report, uint8_t key) {
	for (uint32_t i = 0; i < 6; ++i) {
		if (report->keycode[i] == key) return true;
	}
	return false;
}

static void set_held(uint8_t key, bool held) {
	if (held) _held[key / 32] |= 1u << (key % 32);
	else _held[key / 32] &= ~(1u << (key % 32));
}

static void toggle_led(uint8_t key) {
	const uint8_t led =
		key == HID_KEY_CAPS_LOCK ? LED_CAPS :
		key == HID_KEY_NUM_LOCK ? LED_NUM :
		key == HID_KEY_SCROLL_LOCK ? LED_SCROLL : 0;
	if (led) {
		_leds ^= led;
		_leds_due = true;
	}
}

// From pico-hid-host, as a keyboard sends a report: the modifiers and up to
// six keys held
void process_kbd_report(hid_keyboard_report_t const *report, hid_keyboard_report_t const *prev_report) {
	// Too many keys held to tell which
	if (report->keycode[0] == 0x01) return;
	_modifiers = report->modifier;
	for (uint32_t i = 0; i < 6; ++i) {
		const uint8_t key = prev_report->keycode[i];
		if (key > 0x01 && !in_report(report, key)) {
			set_held(key, false);
			add_event(key, false, false);
			if (key == _repeat_key) _repeat_key = HID_KEY_NONE;
		}
	}
	for (uint32_t i = 0; i < 6; ++i) {
		const uint8_t key = report->keycode[i];
		if (key > 0x01 && !in_report(prev_report, key)) {
			set_held(key, true);
			add_event(key, true, false);
			toggle_led(key);
			_repeat_key = key;
			_repeat_at = time_us_32() + REPEAT_DELAY_US;
		}
	}
}

void process_kbd_mount(uint8_t dev_addr, uint8_t instance) {
	_mounted = true;
	_dev_addr = dev_addr;
	_instance = instance;
	_leds_due = true;
}

void process_kbd_unmount(uint8_t dev_addr, uint8_t instance) {
	(void)dev_addr;
	(void)instance;
	_mounted = false;
	for (uint32_t i = 0; i < 8; ++i) _held[i] = 0;
	_modifiers = 0;
	_repeat_key = HID_KEY_NONE;
}

void usbkbd_init(void) {
	tuh_hid_app_startup();
}

void usbkbd_tick(void) {
	tuh_task();
	if (_repeat_key != HID_KEY_NONE && (int32_t)(time_us_32() - _repeat_at) >= 0) {
		add_event(_repeat_key, true, true);
		_repeat_at += REPEAT_US;
	}
	if (_mounted && _leds_due) {
		// Kept, as it is sent after this returns
		static uint8_t leds;
		leds = _leds;
		// Tried again next time if the keyboard is busy
		if (tuh_hid_set_report(_dev_addr, _instance, 0, HID_REPORT_TYPE_OUTPUT, &leds, 1)) {
			_leds_due = false;
		}
	}
}

bool usbkbd_event(UsbKbdEvent_t* e) {
	if (_event_out == _event_in) return false;
	*e = _events[_event_out];
	_event_out = (_event_out + 1) % EVENTS;
	return true;
}

bool usbkbd_held(uint8_t key) {
	return _held[key / 32] & (1u << (key % 32));
}

uint8_t usbkbd_modifiers(void) {
	return _modifiers;
}

// From HID_KEY_1 to HID_KEY_SLASH, as on a UK keyboard
static const char _plain[] = "1234567890\r\x1b\b\t -=[]\\#;'`,./";
static const char _shifted[] = "!\"\0$%^&*()\r\x1b\b\t _+{}|~:@\0<>?";

char usbkbd_ascii(uint8_t k, uint8_t modifiers) {
	const bool shift = modifiers & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
	if (k >= HID_KEY_A && k <= HID_KEY_Z) {
		const bool upper = shift != ((_leds & LED_CAPS) != 0);
		return (char)((upper ? 'A' : 'a') + (k - HID_KEY_A));
	}
	// The pound and not signs are left out, as they aren't ASCII
	if (k >= HID_KEY_1 && k <= HID_KEY_SLASH) return (shift ? _shifted : _plain)[k - HID_KEY_1];
	if (k == HID_KEY_EUROPE_2) return shift ? '|' : '\\';
	if (k == HID_KEY_KEYPAD_ENTER) return '\r';
	if (k == HID_KEY_KEYPAD_DIVIDE) return '/';
	if (k == HID_KEY_KEYPAD_MULTIPLY) return '*';
	if (k == HID_KEY_KEYPAD_SUBTRACT) return '-';
	if (k == HID_KEY_KEYPAD_ADD) return '+';
	if (_leds & LED_NUM) {
		if (k >= HID_KEY_KEYPAD_1 && k <= HID_KEY_KEYPAD_9) return (char)('1' + (k - HID_KEY_KEYPAD_1));
		if (k == HID_KEY_KEYPAD_0) return '0';
		if (k == HID_KEY_KEYPAD_DECIMAL) return '.';
	}
	return 0;
}

#endif
