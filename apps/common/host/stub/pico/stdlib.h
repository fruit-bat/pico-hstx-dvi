#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
typedef unsigned int uint;
#define __force_inline inline __attribute__((always_inline))
#define __not_in_flash_func(f) f
#define __scratch_y(s)
#define __scratch_x(s)
#define __mul_instruction(a, b) ((a) * (b))
#define GPIO_IN 0
#define GPIO_OUT 1
void gpio_init(uint32_t g);
void gpio_set_dir(uint32_t g, int d);
void gpio_pull_up(uint32_t g);
void gpio_put(uint32_t g, int v);
uint32_t gpio_get_all(void);
uint32_t time_us_32(void);
void sleep_ms(uint32_t ms);
void stdio_init_all(void);
