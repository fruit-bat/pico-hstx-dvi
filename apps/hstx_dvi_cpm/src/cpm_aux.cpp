// Copyright (c) 2026 fruit-bat

// CP/M's reader (RDR:) and punch (PUN:) on UART1: see host_console.h.

#include "host_console.h"

#ifdef CPM_AUX

#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"

// What has come in, kept by the interrupt so none is lost while CP/M is busy
// (e.g. writing to the SD card)
static constexpr uint32_t RX_SIZE = 4096;
static uint8_t _rx[RX_SIZE];
static volatile uint32_t _rx_in = 0, _rx_out = 0;

static void __not_in_flash_func(aux_irq)() {
    while (uart_is_readable(uart1)) {
        const uint8_t c = (uint8_t)uart_get_hw(uart1)->dr;
        const uint32_t next = (_rx_in + 1) % RX_SIZE;
        if (next != _rx_out) { // Else full: dropped
            _rx[_rx_in] = c;
            _rx_in = next;
        }
    }
}

void conctrl_aux_init() {
    uart_init(uart1, CPM_AUX_BAUD);
    gpio_set_function(CPM_AUX_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(CPM_AUX_RX_PIN, GPIO_FUNC_UART);
    irq_set_exclusive_handler(UART1_IRQ, aux_irq);
    irq_set_enabled(UART1_IRQ, true);
    uart_set_irq_enables(uart1, true, false);
}

int conctrl_aux_in() {
    while (_rx_out == _rx_in) {
        // Still looking for the settings key, and Ctrl-C to give up waiting,
        // as end of file
        conctrl_status();
        if (conctrl_key == 0x03) {
            conctrl_key = -1;
            return 0x1A;
        }
    }
    const uint8_t c = _rx[_rx_out];
    _rx_out = (_rx_out + 1) % RX_SIZE;
    return c;
}

void conctrl_aux_out(int c) {
    uart_putc_raw(uart1, (char)c);
}

#endif
