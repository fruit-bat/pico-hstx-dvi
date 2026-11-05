// The MIDI OUT driver (game_synth.h): a General MIDI synth on a UART's TX,
// from a buffer the UART's interrupt empties as it can.
#include "game_synth.h"

#ifdef GAME_MIDI

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

static uint32_t uart_irq(void) {
	return GAME_MIDI_UART == uart0 ? UART0_IRQ : UART1_IRQ;
}

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

static void start(void) {
	_head = _tail = 0;
	_status = 0;
	uart_init(GAME_MIDI_UART, MIDI_BAUD);
	gpio_set_function(GAME_MIDI_TX_GPIO, GPIO_FUNC_UART);
	irq_set_exclusive_handler(uart_irq(), on_uart);
	irq_set_enabled(uart_irq(), true);
}

// What is still to go is lost, so the synth may be left with a note on,
// though the sequencer lets go of everything when it changes synth
static void stop(void) {
	irq_set_enabled(uart_irq(), false);
	irq_remove_handler(uart_irq(), on_uart);
	uart_deinit(GAME_MIDI_UART);
	gpio_set_function(GAME_MIDI_TX_GPIO, GPIO_FUNC_NULL);
}

static void send(uint8_t status, uint8_t a, uint8_t b) {
	const uint32_t irq = save_and_disable_interrupts();
	if (status != _status) put(status);
	_status = status;
	put(a);
	if (game_synth_message_length(status) == 3) put(b);
	pump();
	restore_interrupts(irq);
}

static void sysex(const uint8_t* data, uint32_t n) {
	const uint32_t irq = save_and_disable_interrupts();
	for (uint32_t i = 0; i < n; ++i) put(data[i]);
	_status = 0;
	pump();
	restore_interrupts(irq);
}

const GameSynthDriver_t game_synth_uart = { "MIDI OUT", start, stop, send, sysex, NULL };

#endif
