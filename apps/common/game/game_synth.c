// See game_synth.h.
#include "game_synth.h"
#include "game_seq.h"

// Two bytes for a program change or channel pressure, else three
static uint32_t message_length(uint8_t status) {
	const uint8_t kind = status & 0xf0;
	return kind == 0xc0 || kind == 0xd0 ? 2 : 3;
}

#if defined(GAME_MIDI)

// On a UART, from a buffer the UART's interrupt empties as it can

#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/sync.h"

#define MIDI_BAUD 31250
#define BUFFER 512

static uint8_t _buffer[BUFFER];
static volatile uint32_t _head, _tail;
// The status last sent, which needn't be sent again before the next message
// (running status), or 0 to send it
static uint8_t _status;

static void pump(void) {
	while (_tail != _head && uart_is_writable(GAME_MIDI_UART)) {
		uart_get_hw(GAME_MIDI_UART)->dr = _buffer[_tail];
		_tail = (_tail + 1) % BUFFER;
	}
	uart_set_irq_enables(GAME_MIDI_UART, false, _tail != _head);
}

static void on_uart(void) {
	pump();
}

// A byte, or if there is no room, nothing, and the next status sent in full
static void put(uint8_t b) {
	const uint32_t next = (_head + 1) % BUFFER;
	if (next == _tail) {
		_status = 0;
		return;
	}
	_buffer[_head] = b;
	_head = next;
}

void game_synth_init(void) {
	uart_init(GAME_MIDI_UART, MIDI_BAUD);
	gpio_set_function(GAME_MIDI_TX_GPIO, GPIO_FUNC_UART);
	const uint32_t irq = GAME_MIDI_UART == uart0 ? UART0_IRQ : UART1_IRQ;
	irq_set_exclusive_handler(irq, on_uart);
	irq_set_enabled(irq, true);
}

void game_synth_send(uint8_t status, uint8_t a, uint8_t b) {
	const uint32_t irq = save_and_disable_interrupts();
	if (status != _status) put(status);
	_status = status;
	put(a);
	if (message_length(status) == 3) put(b);
	pump();
	restore_interrupts(irq);
}

void game_synth_sysex(const uint8_t* data, uint32_t n) {
	const uint32_t irq = save_and_disable_interrupts();
	for (uint32_t i = 0; i < n; ++i) put(data[i]);
	_status = 0;
	pump();
	restore_interrupts(irq);
}

void game_synth_tick(void) {
}

#elif defined(GAME_HOST)

// On the PC, a MIDI file, <mode>.mid, written as the program ends

#include <stdio.h>
#include <stdlib.h>

extern const char* host_mode;

static uint8_t* _mid;
static size_t _n, _max;
// Ticks since the last message
static uint32_t _delta;

static void put(uint8_t b) {
	if (_n == _max) {
		_max = _max ? _max * 2 : 4096;
		_mid = realloc(_mid, _max);
	}
	_mid[_n++] = b;
}

// A number in sevens, the top first, all but the last with the top bit set
static void put_number(uint32_t v) {
	uint8_t b[5];
	uint32_t n = 0;
	do {
		b[n++] = v & 0x7f;
		v >>= 7;
	} while (v);
	while (n--) put(b[n] | (n ? 0x80 : 0));
}

static void put_be(FILE* f, uint32_t v, uint32_t bytes) {
	while (bytes--) fputc((int)((v >> (8 * bytes)) & 0xff), f);
}

// One track, with a tick a quarter note's SEQ_HZ / 2th, as the tempo is the
// usual half a second a quarter note
static void save(void) {
	if (!_n) return;
	char name[64];
	snprintf(name, sizeof(name), "%s.mid", host_mode);
	FILE* f = fopen(name, "wb");
	if (!f) return;
	static const uint8_t end[] = { 0x00, 0xff, 0x2f, 0x00 };
	fwrite("MThd", 1, 4, f);
	put_be(f, 6, 4);
	put_be(f, 0, 2);
	put_be(f, 1, 2);
	put_be(f, SEQ_HZ / 2, 2);
	fwrite("MTrk", 1, 4, f);
	put_be(f, (uint32_t)(_n + sizeof(end)), 4);
	fwrite(_mid, 1, _n, f);
	fwrite(end, 1, sizeof(end), f);
	fclose(f);
}

void game_synth_init(void) {
	atexit(save);
}

void game_synth_send(uint8_t status, uint8_t a, uint8_t b) {
	put_number(_delta);
	_delta = 0;
	put(status);
	put(a);
	if (message_length(status) == 3) put(b);
}

void game_synth_sysex(const uint8_t* data, uint32_t n) {
	if (n < 2) return;
	put_number(_delta);
	_delta = 0;
	put(0xf0);
	put_number(n - 1);
	for (uint32_t i = 1; i < n; ++i) put(data[i]);
}

void game_synth_tick(void) {
	++_delta;
}

#else

// Nowhere to play

void game_synth_init(void) {
}

void game_synth_send(uint8_t status, uint8_t a, uint8_t b) {
	(void)message_length;
	(void)status;
	(void)a;
	(void)b;
}

void game_synth_sysex(const uint8_t* data, uint32_t n) {
	(void)data;
	(void)n;
}

void game_synth_tick(void) {
}

#endif
