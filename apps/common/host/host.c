// Runs a game on the PC, saving frames as PPM images.
// The sprite renderer runs in a second thread in place of core 1, and the
// buttons are pressed by the game's scenario, see host.h.
//
// <game>_host [mode] [frames] [every] [out dir]
//
// mode   which scenario to run, see the game's scenario.c
// frames how many frames to run for
// every  save a frame this often
// out    where to save the frames
//
// HOST_FRAMES can list the frames to save instead, as in "430,470,1530".
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "pico/stdlib.h"
#include "pico/sem.h"
#include "hstx_dvi_core.h"
#include "hstx_dvi_sprite.h"
#include "hstx_dvi_row_fifo.h"
#include "hstx_dvi_row_buf.h"

#include "host.h"

typedef struct { pthread_mutex_t m; pthread_cond_t c; int v; } HostSem;

void host_sem_init(struct semaphore* s, int16_t initial, int16_t max) {
	HostSem* h = calloc(1, sizeof(HostSem));
	pthread_mutex_init(&h->m, NULL);
	pthread_cond_init(&h->c, NULL);
	h->v = initial;
	s->impl = h;
}
bool host_sem_release(struct semaphore* s) {
	HostSem* h = s->impl;
	pthread_mutex_lock(&h->m);
	if (h->v < 1) h->v++;
	pthread_cond_broadcast(&h->c);
	pthread_mutex_unlock(&h->m);
	return true;
}
void host_sem_acquire_blocking(struct semaphore* s) {
	HostSem* h = s->impl;
	pthread_mutex_lock(&h->m);
	while (h->v == 0) pthread_cond_wait(&h->c, &h->m);
	h->v--;
	pthread_mutex_unlock(&h->m);
}
bool host_sem_try_acquire(struct semaphore* s) {
	HostSem* h = s->impl;
	pthread_mutex_lock(&h->m);
	bool ok = h->v > 0;
	if (ok) h->v--;
	pthread_mutex_unlock(&h->m);
	return ok;
}

static void* core1(void* a) { ((void (*)(void))a)(); return NULL; }
void multicore_launch_core1(void (*entry)(void)) {
	pthread_t t;
	pthread_create(&t, NULL, core1, (void*)entry);
}

static hstx_dvi_row_t _rows[4];
static uint32_t _row_i;
void hstx_dvi_row_buf_init(void) {}
hstx_dvi_row_t* hstx_dvi_row_buf_get(void) { return &_rows[(_row_i++) & 3]; }
void hstx_dvi_row_fifo_init1(PIO pio) {}
hstx_dvi_pixel_row_fetcher hstx_dvi_row_fifo_get_row_fetcher(void) { return NULL; }
void hstx_dvi_init(hstx_dvi_pixel_row_fetcher f) {}
void hstx_dvi_fill_row(hstx_dvi_row_t* row, hstx_dvi_pixel_t p) { memset(row->b, p, sizeof(row->b)); }

const char* host_mode = "attract";
static uint32_t _max_frames = 3000;
static uint32_t _every = 300;
static const char* _out = "frames";
static volatile uint32_t _frames = 0;

static uint8_t _frame_buf[MODE_V_ACTIVE_LINES][MODE_H_ACTIVE_PIXELS];
static uint32_t _y = 0;
static uint32_t _max_sprites = 0;
// The first button, as set up by the game
static uint32_t _gpio_first = 32;

// Whether to save a frame, see HOST_FRAMES
static bool wanted(uint32_t n) {
	static const char* list = NULL;
	static bool looked = false;
	if (!looked) {
		list = getenv("HOST_FRAMES");
		looked = true;
	}
	if (!list) return n % _every == 0;
	for (const char* s = list; *s; ) {
		char* end;
		const unsigned long f = strtoul(s, &end, 10);
		if (end == s) break;
		if (f == n) return true;
		s = *end ? end + 1 : end;
	}
	return false;
}

static void save(uint32_t n) {
	char name[256];
	snprintf(name, sizeof(name), "%s/%s_%05u.ppm", _out, host_mode, n);
	FILE* f = fopen(name, "wb");
	if (!f) {
		perror(name);
		exit(1);
	}
	fprintf(f, "P6 %d %d 255\n", MODE_H_ACTIVE_PIXELS, MODE_V_ACTIVE_LINES);
	for (int y = 0; y < MODE_V_ACTIVE_LINES; ++y) {
		for (int x = 0; x < MODE_H_ACTIVE_PIXELS; ++x) {
			const uint8_t p = _frame_buf[y][x];
			const uint8_t rgb[3] = { (p & 0xe0), (p & 0x1c) << 3, (p & 3) << 6 };
			fwrite(rgb, 1, 3, f);
		}
	}
	fclose(f);
}

void hstx_dvi_row_fifo_put_blocking(hstx_dvi_row_t* r) {
	memcpy(_frame_buf[_y], r->b, MODE_H_ACTIVE_PIXELS);
	if (++_y == MODE_V_ACTIVE_LINES) {
		_y = 0;
		uint32_t n = 0;
		for (uint32_t i = 0; i < MAX_SPRITES; ++i) if (_sprites[i].f & SF_ENABLE) ++n;
		if (n > _max_sprites) _max_sprites = n;
		if (wanted(_frames)) save(_frames);
		host_frame(_frames, n);
		if (++_frames >= _max_frames) {
			printf("frames %u, most sprites %u\n", _frames, _max_sprites);
			exit(0);
		}
	}
}

void gpio_init(uint32_t g) { if (g < _gpio_first) _gpio_first = g; }
void gpio_set_dir(uint32_t g, int d) {}
void gpio_pull_up(uint32_t g) {}
void gpio_put(uint32_t g, int v) {}
uint32_t time_us_32(void) { return 12345; }
void sleep_ms(uint32_t ms) {}
void stdio_init_all(void) {}

uint32_t gpio_get_all(void) {
	return ~(host_input(_frames) << _gpio_first);
}

int main_game(void);

int main(int argc, char** argv) {
	if (argc > 1) host_mode = argv[1];
	if (argc > 2) _max_frames = atoi(argv[2]);
	if (argc > 3) _every = atoi(argv[3]);
	if (argc > 4) _out = argv[4];
	if (_every == 0) _every = 1;
	mkdir(_out, 0755);
	return main_game();
}
