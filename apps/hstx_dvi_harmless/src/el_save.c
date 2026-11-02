// Copyright (c) 2024 fruit-bat.

// See el_save.h. Each slot is a file, pico_dvi_harmless/commander1.dat and so
// on, holding a Save_t as it is.

#include "el_save.h"
#include "el_draw.h"
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

#define SAVE_DIR "/pico_dvi_harmless"
// Where the saves were before the game was called Harmless, moved on mounting
#define OLD_DIR "/pico_dvi_elite"
#define MOUNT_TRIES 5
#define MOUNT_WAIT_MS 250

static FATFS _fs;

bool save_built(void) {
	return true;
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
		FILINFO info;
		if (f_stat(SAVE_DIR, &info) != FR_OK && f_stat(OLD_DIR, &info) == FR_OK) {
			printf("SD card: rename " OLD_DIR " %d\n", (int)f_rename(OLD_DIR, SAVE_DIR));
		}
		r = f_mkdir(SAVE_DIR);
		printf("SD card: mkdir " SAVE_DIR " %d\n", (int)r);
		if (r == FR_EXIST) r = FR_OK;
	}
	_card = r == FR_OK;
	_error = (int)r;
	return _card;
}

// Reads up to max bytes of a file in the directory, returning how many in n
static bool file_read(const char* name, void* buf, size_t max, size_t* n) {
	char path[48];
	snprintf(path, sizeof(path), SAVE_DIR "/%s", name);
	FIL f;
	const FRESULT o = f_open(&f, path, FA_READ);
	printf("SD card: open %s %d\n", path, (int)o);
	if (o != FR_OK) return false;
	UINT got = 0;
	const FRESULT r = f_read(&f, buf, max, &got);
	f_close(&f);
	*n = got;
	return r == FR_OK;
}

static bool file_write(const char* name, const void* buf, size_t n) {
	char path[48];
	snprintf(path, sizeof(path), SAVE_DIR "/%s", name);
	FIL f;
	const FRESULT o = f_open(&f, path, FA_WRITE | FA_CREATE_ALWAYS);
	if (o != FR_OK) {
		printf("SD card: create %s %d\n", path, (int)o);
		return false;
	}
	UINT put = 0;
	const FRESULT r = f_write(&f, buf, n, &put);
	const FRESULT c = f_close(&f);
	printf("SD card: write %s %d %d\n", path, (int)r, (int)c);
	return r == FR_OK && c == FR_OK && put == n;
}

#elif defined(EL_HOST_SAVE)

// On the PC, for trying it: the files in pico_dvi_harmless where it is run

#define SAVE_DIR "pico_dvi_harmless"

bool save_built(void) {
	return true;
}

bool save_ready(void) {
	FILE* f = fopen(SAVE_DIR "/.ready", "w");
	_card = f != NULL;
	if (!f) return false;
	fclose(f);
	return true;
}

static bool file_read(const char* name, void* buf, size_t max, size_t* n) {
	char path[48];
	snprintf(path, sizeof(path), SAVE_DIR "/%s", name);
	FILE* f = fopen(path, "rb");
	if (!f) return false;
	*n = fread(buf, 1, max, f);
	fclose(f);
	return true;
}

static bool file_write(const char* name, const void* buf, size_t n) {
	char path[48];
	snprintf(path, sizeof(path), SAVE_DIR "/%s", name);
	FILE* f = fopen(path, "wb");
	if (!f) return false;
	const size_t put = fwrite(buf, 1, n, f);
	return fclose(f) == 0 && put == n;
}

#endif

#if defined(EL_SDCARD) || defined(EL_HOST_SAVE)

static void slot_name(char* name, size_t n, uint32_t slot) {
	snprintf(name, n, "commander%u.dat", (unsigned)(slot + 1));
}

bool save_read(uint32_t slot, Save_t* s) {
	char name[20];
	slot_name(name, sizeof(name), slot);
	size_t n = 0;
	return file_read(name, &_read, sizeof(_read), &n) && save_good(s, n);
}

bool save_write(uint32_t slot, Save_t* s) {
	s->magic = SAVE_MAGIC;
	s->size = sizeof(*s);
	char name[20];
	slot_name(name, sizeof(name), slot);
	return file_write(name, s, sizeof(*s));
}

// The options, in settings.dat
#define SETTINGS_MAGIC 0x504f4c45u // "ELOP"
#define SETTINGS_FILE "settings.dat"

typedef struct {
	uint32_t magic;
	uint32_t size;
	uint32_t style;
	uint32_t dither;
	uint32_t hud_filled;
	uint32_t wide;
	uint32_t lens;    // added later, so older settings are without it
} Settings_t;

void load_settings(void) {
	if (!_card) return;
	Settings_t t;
	size_t n = 0;
	t.lens = LENS_NORMAL;
	if (!file_read(SETTINGS_FILE, &t, sizeof(t), &n) || n < offsetof(Settings_t, lens) || n > sizeof(t)) return;
	if (t.magic != SETTINGS_MAGIC || t.size != n || t.style >= STYLE_COUNT) return;
	draw_style = (Style_e)t.style;
	draw_dither = t.dither != 0;
	draw_hud_filled = t.hud_filled != 0;
	draw_aspect = t.wide ? ASPECT_16_9 : ASPECT_4_3;
	draw_set_lens((Lens_e)t.lens);
}

void save_settings(void) {
	if (!_card) return;
	const Settings_t t = {
		SETTINGS_MAGIC, sizeof(t), (uint32_t)draw_style, draw_dither, draw_hud_filled, draw_aspect == ASPECT_16_9, (uint32_t)draw_lens
	};
	file_write(SETTINGS_FILE, &t, sizeof(t));
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

void load_settings(void) {
}

void save_settings(void) {
}

#endif
