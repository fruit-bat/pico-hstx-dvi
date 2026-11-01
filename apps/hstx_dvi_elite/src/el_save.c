// Copyright (c) 2024 fruit-bat.

// See el_save.h. Each slot is a file, pico_dvi_elite/commander1.dat and so
// on, holding a Save_t as it is.

#include "el_save.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define SAVE_MAGIC 0x45544c45u // "ELTE"

#if defined(EL_SDCARD) || defined(EL_HOST_SAVE)

// Saves made before there were names are as long as this, and are Jameson
#define SAVE_UNNAMED offsetof(Save_t, name)

// Whether what was read is a save, of this build or one before names
static bool save_good(Save_t* s, size_t n) {
	if (n < SAVE_UNNAMED || s->magic != SAVE_MAGIC || s->size != n) return false;
	if (n == SAVE_UNNAMED) strcpy(s->name, "JAMESON");
	s->name[SAVE_NAME_MAX - 1] = 0;
	return n == SAVE_UNNAMED || n == sizeof(*s);
}

#endif

static bool _card = false;
static int _error = 0;

bool save_card(void) {
	return _card;
}

int save_error(void) {
	return _error;
}

#if defined(EL_SDCARD)

#include "ff.h"
#include "pico/stdlib.h"

#define SAVE_DIR "/pico_dvi_elite"
#define MOUNT_TRIES 5
#define MOUNT_WAIT_MS 250

static FATFS _fs;

bool save_built(void) {
	return true;
}

static void slot_path(char* path, size_t n, uint32_t slot) {
	snprintf(path, n, SAVE_DIR "/commander%u.dat", (unsigned)(slot + 1));
}

bool save_ready(void) {
	// Some cards do not answer the first time after power up, so it tries a few times
	FRESULT r = FR_NOT_READY;
	for (uint32_t i = 0; i < MOUNT_TRIES && r != FR_OK; ++i) {
		if (i) sleep_ms(MOUNT_WAIT_MS);
		r = f_mount(&_fs, "", 1);
		printf("SD card: mount %d\n", (int)r);
	}
	if (r == FR_OK) {
		r = f_mkdir(SAVE_DIR);
		printf("SD card: mkdir " SAVE_DIR " %d\n", (int)r);
		if (r == FR_EXIST) r = FR_OK;
	}
	_card = r == FR_OK;
	_error = (int)r;
	return _card;
}

bool save_read(uint32_t slot, Save_t* s) {
	char path[40];
	slot_path(path, sizeof(path), slot);
	FIL f;
	const FRESULT o = f_open(&f, path, FA_READ);
	printf("SD card: open %s %d\n", path, (int)o);
	if (o != FR_OK) return false;
	UINT n = 0;
	const FRESULT r = f_read(&f, s, sizeof(*s), &n);
	f_close(&f);
	return r == FR_OK && save_good(s, n);
}

bool save_write(uint32_t slot, Save_t* s) {
	s->magic = SAVE_MAGIC;
	s->size = sizeof(*s);
	char path[40];
	slot_path(path, sizeof(path), slot);
	FIL f;
	const FRESULT o = f_open(&f, path, FA_WRITE | FA_CREATE_ALWAYS);
	if (o != FR_OK) {
		printf("SD card: create %s %d\n", path, (int)o);
		return false;
	}
	UINT n = 0;
	FRESULT r = f_write(&f, s, sizeof(*s), &n);
	const FRESULT c = f_close(&f);
	printf("SD card: write %s %d %d\n", path, (int)r, (int)c);
	return r == FR_OK && c == FR_OK && n == sizeof(*s);
}

#elif defined(EL_HOST_SAVE)

// On the PC, for trying it: the files in pico_dvi_elite where it is run

static void slot_path(char* path, size_t n, uint32_t slot) {
	snprintf(path, n, "pico_dvi_elite/commander%u.dat", (unsigned)(slot + 1));
}

bool save_built(void) {
	return true;
}

bool save_ready(void) {
	FILE* f = fopen("pico_dvi_elite/.ready", "w");
	_card = f != NULL;
	if (!f) return false;
	fclose(f);
	return true;
}

bool save_read(uint32_t slot, Save_t* s) {
	char path[48];
	slot_path(path, sizeof(path), slot);
	FILE* f = fopen(path, "rb");
	if (!f) return false;
	const size_t n = fread(s, 1, sizeof(*s), f);
	fclose(f);
	return save_good(s, n);
}

bool save_write(uint32_t slot, Save_t* s) {
	s->magic = SAVE_MAGIC;
	s->size = sizeof(*s);
	char path[48];
	slot_path(path, sizeof(path), slot);
	FILE* f = fopen(path, "wb");
	if (!f) return false;
	const size_t n = fwrite(s, 1, sizeof(*s), f);
	return fclose(f) == 0 && n == sizeof(*s);
}

#else

bool save_built(void) {
	return false;
}

bool save_ready(void) {
	return false;
}

bool save_read(uint32_t slot, Save_t* s) {
	(void)slot;
	(void)s;
	return false;
}

bool save_write(uint32_t slot, Save_t* s) {
	(void)slot;
	(void)s;
	return false;
}

#endif
