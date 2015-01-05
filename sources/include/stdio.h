#ifndef __STDIO_H__
#define __STDIO_H__

#include <efi.h>
#include "types.h"

static inline void _stdio_stop(void) {
  __asm__ __volatile__("cli");
  __asm__ __volatile__("hlt");
  while (1);
}

/* WHY IT DOESN'T WORK ?
 TODO
      __asm__ __volatile__("cli");                                   
      __asm__ __volatile__("hlt");                                   
      while (1);                                                     */

#define PRINTK(stop, msg, ...)                                       \
  do {                                                               \
    printk("%s(%03d): ", __FUNCTION__, __LINE__);                    \
    /* printk("%s(%s,%03d): ", __FUNCTION__, __FILE__, __LINE__); */ \
    printk(msg);                                                     \
    printk(__VA_ARGS__);                                             \
    if (stop == 1) {                                                 \
      _stdio_stop();                                                 \
    }                                                                \
  } while (0)

#define ERROR(...)  PRINTK(1, "<ERROR> ",  __VA_ARGS__)
#define ACTION(...) PRINTK(0, "<ACTION> ", __VA_ARGS__)
#define INFO(...)   PRINTK(0, "<INFO> ",   __VA_ARGS__)
#ifdef _DEBUG
#define DBG(...)  PRINTK(0, "<DEBUG> ",   __VA_ARGS__)
#else
#define DBG(...)
#endif

void printk_string(int8_t *string, int8_t minimum_length, int8_t padding);

void printk(char *format, ...);

int getchar();

void printk_bin(uint32_t size, char *sep, uint8_t *data);

#define panic ERROR

#endif
