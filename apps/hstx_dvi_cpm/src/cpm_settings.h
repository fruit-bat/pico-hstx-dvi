// Copyright (c) 2026 fruit-bat

#pragma once

// The settings screen of the CP/M emulator, opened with Ctrl-\ (or F12 on a
// USB keyboard): the Z80's speed, its registers and memory, the code at PC
// disassembled, and stepping it an instruction at a time. It is drawn on the
// DVI terminal's alternate screen, so what CP/M had on the screen comes back
// as it was.

#include <cstdint>
#include <Z80.h>

// Thrown by the settings screen to restart CP/M, as if reset
struct CpmRestart {};

// The Z80's speed in kHz to start with, 0 for as fast as it goes, and where
// the BDOS and BIOS are (which the emulator runs itself), for the code shown
void cpm_settings_init(uint32_t khz, uint16_t bdos, uint16_t bios);

// The emulator's monitor (Computer::setMonitor), called before each step of
// the Z80: keeps it to the speed set, looks for the settings key, and stops
// it there when stepping
void cpm_settings_monitor(void* ctx, Z80& cpu, uint8_t* memory, uint64_t cycles);

// Opens the settings screen, the Z80 paused until it is closed
void cpm_settings_open();
