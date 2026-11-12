// Copyright (c) 2026 fruit-bat

#pragma once

// A small Z80 disassembler, for the settings screen: all of the instruction
// set, the prefixed ones (CB, ED, DD, FD, DD CB, FD CB) and the undocumented
// IXH, IXL, IYH, IYL and SLL, in Zilog's syntax with hex numbers such as 0FFh.

#include <cstddef>
#include <cstdint>

// Writes the instruction at addr in the 64K of memory to out (up to size
// bytes, e.g. "LD A,(IX+05h)"), returning its length in bytes, 1 to 4
uint32_t z80_disasm(const uint8_t* memory, uint16_t addr, char* out, size_t size);
