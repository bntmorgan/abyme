#ifndef __STDIO_H__
#define __STDIO_H__

#include <efi.h>
#include "types.h"
#include "serial.h"
#include "debug.h"

#ifdef _DEBUG_SERVER
#include "debug_server/debug_server.h"
#endif

void _stdio_stop(void);

#define PRINTK(stop, msg, ...)                                       \
  do {                                                               \
    printk(msg);                                                     \
    printk("%s(%03d): ", __FUNCTION__, __LINE__);                    \
    /* printk("%s(%s,%03d): ", __FUNCTION__, __FILE__, __LINE__); */ \
    printk(__VA_ARGS__);                                             \
    if (stop == 1) {                                                 \
      _stdio_stop();                                             \
    }                                                                \
  } while (0)

#define ERROR(...)  PRINTK(1, "[error]",  __VA_ARGS__)
#define ACTION(...) PRINTK(0, "[action]", __VA_ARGS__)
#define INFO(...)   PRINTK(0, "[info]",   __VA_ARGS__)
#define WARN(...)   PRINTK(0, "[warn]",   __VA_ARGS__)
#ifdef _DEBUG
#define DBG(...)  PRINTK(0, "[debug]",   __VA_ARGS__)
#else
#define DBG(...)
#endif

#define PRINT_FIELD_PREFIX(__pf__, __st__, __fd__) \
  printk("%s%s: 0x%x\n", __pf__, #__fd__, (__st__)->__fd__);

#define PRINT_FIELD(__st__, __fd__) \
  PRINT_FIELD_PREFIX("  ", __st__, __fd__)

#define SERIAL_INFO(...) \
  debug_server_serial_on(); \
  INFO(__VA_ARGS__); \
  debug_server_serial_off();

#define SERIAL_DUMP(...) \
  debug_server_serial_on(); \
  dump(__VA_ARGS__); \
  debug_server_serial_off();

#define SERIAL_NEW_LINE \
  debug_server_serial_on(); \
  printk("\n"); \
  debug_server_serial_off();

void printk_string(int8_t *string, int8_t minimum_length, int8_t padding);

void printk(char *format, ...);

int getchar();

void printk_bin(uint32_t size, char *sep, uint8_t *data);

void no_putc(uint8_t value);

extern void (*putc)(uint8_t);

#define panic ERROR

#endif
