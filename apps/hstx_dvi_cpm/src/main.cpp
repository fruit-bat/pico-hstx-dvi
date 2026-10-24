// Copyright (c) 2026 fruit-bat

// CP/M 2.2 on the DVI screen, using the CPM-Emulator (a Z80 with an emulated BDOS).
// The drives are directories on an SD card and, for now, keys come from the USB
// serial port. Output goes to the screen and to the USB serial port.

// This example requires an external digital video connector connected to
// GPIOs 12 through 19 (the HSTX-capable GPIOs) with appropriate
// current-limiting resistors, e.g. 270 ohms. The pinout used in this example
// matches the Pico DVI Sock board, which can be soldered onto a Pico 2:
// https://github.com/Wren6991/Pico-DVI-Sock

#include "hstx_dvi_vt.h"
#include "app_led.h"
#include "pico/stdlib.h"
#include "pico/stdio/driver.h"
#include "ff.h"
#include "computer.h"
#include <cstdio>
#include <exception>

static vt_emu_t* _emu;

// Output-only stdio driver that echoes stdout to the screen;
// input still comes from the other enabled drivers (USB)
static void vt_out_chars(const char *buf, int length) {
    for (int i = 0; i < length; i++) {
        vt_emu_put_ch(_emu, buf[i]);
    }
}

static stdio_driver vt_driver;

static FATFS fatfs;

int main(void)
{
    app_led_on();

    hstx_dvi_vt_init_all();

    _emu = hstx_dvi_vt_emu_get();

    // The screen only mirrors stdout, so leave answering queries to the USB terminal
    vt_emu_set_reply(_emu, NULL, NULL);

    vt_driver.out_chars = vt_out_chars;
#if PICO_STDIO_ENABLE_CRLF_SUPPORT
    // For the emulator's own messages; CP/M output isn't translated (see host_console.h)
    vt_driver.crlf_enabled = PICO_STDIO_DEFAULT_CRLF;
#endif
    stdio_init_all();
    stdio_set_driver_enabled(&vt_driver, true);

    sleep_ms(2000); // Allow time for initialization

    printf("HSTX DVI CP/M\n");

    // 64K of memory, BDOS at FC00h and BIOS at FE00h, booting the CCP at F400h
    static Computer<64, 0xFC00, 0xFE00> computer("CCP-DR.64K", 0xF400);

    while (true) {
        try {
            // The drives are the directories A to P beside CCP-DR.64K, in the
            // directory CPM_DIR of the SD card if it has one, else at the top
            FRESULT r = f_mount(&fatfs, "", 1);
            if (r == FR_OK && f_chdir("/" CPM_DIR) != FR_OK) r = f_chdir("/");
            if (r != FR_OK) {
                hostfs::check(r);
                printf("\nCan't read the SD card: %s\n", hostfs::lastError());
            }
            else {
                computer.start(); // Only returns through an exception
            }
        }
        catch (const std::exception& e) {
            printf("\n%s\n", e.what());
        }
        catch (...) {
            printf("\nEmulator error\n");
        }
        printf("Press a key to restart CP/M\n");
        conctrl_getch_wait();
    }
}
