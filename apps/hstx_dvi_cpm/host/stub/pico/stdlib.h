// The games' pico stubs (apps/common/host/stub), with what the CP/M app uses
// besides: the time, waiting, and the serial port's keys
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include_next "pico/stdlib.h"

typedef uint64_t absolute_time_t;
static inline absolute_time_t from_us_since_boot(uint64_t us) { return us; }
uint64_t time_us_64(void);
void busy_wait_until(absolute_time_t t);
#define PICO_ERROR_TIMEOUT (-1)
int getchar_timeout_us(uint32_t us);
int putchar_raw(int c);

#ifdef __cplusplus
}
#endif
