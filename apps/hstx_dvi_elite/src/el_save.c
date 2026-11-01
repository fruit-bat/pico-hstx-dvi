// Copyright (c) 2024 fruit-bat.

// See el_save.h. Each slot is a file, pico_dvi_elite/commander1.dat and so
// on, holding a Save_t as it is.

#include "el_save.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define SAVE_MAGIC 0x32544c45u // "ELT2"

#if defined(EL_SDCARD) || defined(EL_HOST_SAVE)

// The commander when there was one laser, in saves made before
#define OLD_CMDR (offsetof(Commander_t, laser) + sizeof(Laser_e))

// Saves made before, with the commander first, and then with a name after
#define OLD_MAGIC 0x45544c45u // "ELTE"
typedef struct {
	uint32_t magic;
	uint32_t size;
	uint8_t cmdr[OLD_CMDR];
	uint32_t galaxy;
	uint32_t home;
	uint32_t kills;
	uint32_t offender;
	char name[SAVE_NAME_MAX];
} OldSave_t;

// Big enough for any save, and static, as too big for the stack
static union {
	Save_t now;
	OldSave_t old;
} _read;

// Makes a save from what was read, which may be from an older build
static bool save_good(Save_t* s, size_t n) {
	if (n < sizeof(uint32_t) * 2 || _read.now.size != n) return false;
	trade_commander_start(&s->cmdr);
	strcpy(s->name, "JAMESON");
	if (_read.now.magic == SAVE_MAGIC) {
		// The commander, as far as it was saved
		if (n < offsetof(Save_t, cmdr) + OLD_CMDR || n > sizeof(Save_t)) return false;
		memcpy(s, &_read.now, n);
	}
	else if (_read.old.magic == OLD_MAGIC) {
		if (n != offsetof(OldSave_t, name) && n != sizeof(OldSave_t)) return false;
		memcpy(&s->cmdr, _read.old.cmdr, OLD_CMDR);
		s->galaxy = _read.old.galaxy;
		s->home = _read.old.home;
		s->kills = _read.old.kills;
		s->offender = _read.old.offender;
		if (n == sizeof(OldSave_t)) memcpy(s->name, _read.old.name, SAVE_NAME_MAX);
	}
	else {
		return false;
	}
	s->name[SAVE_NAME_MAX - 1] = 0;
	return true;
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
	const FRESULT r = f_read(&f, &_read, sizeof(_read), &n);
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
	const size_t n = fread(&_read, 1, sizeof(_read), f);
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
