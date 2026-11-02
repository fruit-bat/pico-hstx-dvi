// Copyright (c) 2024 fruit-bat.

// Saving commanders, on an SD card when built with EL_SDCARD, in the
// directory pico_dvi_harmless. Without it, or without a card, there is nowhere
// to save, and the game goes on without.
#pragma once

#include "el_trade.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SAVE_SLOTS 4
// The longest name, 8 letters, and the end
#define SAVE_NAME_MAX 9

// A commander, as saved when docked. The commander goes last, so as more
// is fitted it can grow, and a save made before takes the rest from a new
// commander.
typedef struct {
	uint32_t magic;
	uint32_t size; // of the whole, which tells what it holds
	uint32_t galaxy;
	uint32_t home;
	uint32_t kills;
	uint32_t offender;
	char name[SAVE_NAME_MAX];
	Commander_t cmdr;
} Save_t;

// Whether saving is built in, and whether the card was there when last tried,
// and if not the error from FatFs
bool save_built(void);
bool save_card(void);
int save_error(void);
// Whether there is somewhere to save, trying the card again each time
bool save_ready(void);
// The options, read when there is a card and written when they are left
void load_settings(void);
void save_settings(void);
// Returns false if the slot is empty, or cannot be read
bool save_read(uint32_t slot, Save_t* s);
bool save_write(uint32_t slot, Save_t* s);

#ifdef __cplusplus
}
#endif
