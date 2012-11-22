#ifndef __STRING_H__
#define __STRING_H__

#include <stdint.h>

#include "arch/cpu_int.h"

#define PRINTK(stop, msg, ...)                                       \
  do {                                                               \
    printk("%s(%03d): ", __FUNCTION__, __LINE__);                    \
    /* printk("%s(%s,%03d): ", __FUNCTION__, __FILE__, __LINE__); */ \
    printk(msg);                                                     \
    printk(__VA_ARGS__);                                             \
    if (stop == 1) {                                                 \
      cpu_stop();                                                    \
    }                                                                \
  } while (0)

#define ERROR(...)  PRINTK(1, "<ERROR> ",  __VA_ARGS__)
#define ACTION(...) PRINTK(0, "<ACTION> ", __VA_ARGS__)
#define INFO(...)   PRINTK(0, "<INFO> ",   __VA_ARGS__)

void printk_string(int8_t *string, int8_t minimum_length, int8_t padding);

void itoa(int8_t *dst, uint8_t base, int32_t value);

void printk(char *format, ...);

#endif
