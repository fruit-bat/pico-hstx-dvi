// Copyright (c) 2024 fruit-bat.

// Saving commanders, on an SD card when built with EL_SDCARD, in the
// directory pico_dvi_elite. Without it, or without a card, there is nowhere
// to save, and the game goes on without.
#pragma once

#include "el_trade.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SAVE_SLOTS 4

// A commander, as saved when docked
typedef struct {
	uint32_t magic;
	uint32_t size; // of the whole, so a file from an older build is not read
	Commander_t cmdr;
	uint32_t galaxy;
	uint32_t home;
	uint32_t kills;
	uint32_t offender;
} Save_t;

// Whether saving is built in, and whether the card was there when last tried,
// and if not the error from FatFs
bool save_built(void);
bool save_card(void);
int save_error(void);
// Whether there is somewhere to save, trying the card again each time
bool save_ready(void);
// Returns false if the slot is empty, or cannot be read
bool save_read(uint32_t slot, Save_t* s);
bool save_write(uint32_t slot, Save_t* s);

#ifdef __cplusplus
}
#endif
