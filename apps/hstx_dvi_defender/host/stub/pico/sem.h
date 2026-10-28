#pragma once
#include "pico/stdlib.h"
struct semaphore { void* impl; };
#define sem_init host_sem_init
#define sem_release host_sem_release
#define sem_acquire_blocking host_sem_acquire_blocking
#define sem_try_acquire host_sem_try_acquire
void host_sem_init(struct semaphore* s, int16_t initial, int16_t max);
bool host_sem_release(struct semaphore* s);
void host_sem_acquire_blocking(struct semaphore* s);
bool host_sem_try_acquire(struct semaphore* s);
