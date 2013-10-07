#ifndef __STDIO_H__
#define __STDIO_H__

#include <efi.h>
#include "types.h"

#define PRINTK(stop, msg, ...)                                       \
  do {                                                               \
    printk("%s(%03d): ", __FUNCTION__, __LINE__);                    \
    /* printk("%s(%s,%03d): ", __FUNCTION__, __FILE__, __LINE__); */ \
    printk(msg);                                                     \
    printk(__VA_ARGS__);                                             \
    if (stop == 1) {                                                 \
      __asm__ __volatile__("cli");                                   \
      __asm__ __volatile__("hlt");                                   \
      while (1);                                                     \
    }                                                                \
  } while (0)

#define ERROR(...)  PRINTK(1, "<ERROR> ",  __VA_ARGS__)
#define ACTION(...) PRINTK(0, "<ACTION> ", __VA_ARGS__)
#define INFO(...)   PRINTK(0, "<INFO> ",   __VA_ARGS__)

void printk_string(int8_t *string, int8_t minimum_length, int8_t padding);

void printk(char *format, ...);

int getchar();

#define panic ERROR

#endif
