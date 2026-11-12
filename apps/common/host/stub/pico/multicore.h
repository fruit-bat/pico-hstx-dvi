#pragma once
void multicore_launch_core1(void (*entry)(void));
// The cores' signalling, as threads
#include <sched.h>
static inline void __dmb(void) { __sync_synchronize(); }
static inline void __sev(void) {}
static inline void __wfe(void) { sched_yield(); }
