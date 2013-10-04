#ifndef __STDIO_H__
#define __STDIO_H__

#include <efi.h>
#include "types.h"
//TODO: on ne devrait pas avoir a inclure ce fichier, non???
#include "cpu.h"

//#include "hardware/cpu.h"

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

void printk(char *format, ...);

int getchar();

#define panic ERROR

#endif
