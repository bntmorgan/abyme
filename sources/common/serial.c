#include "stdio.h"

extern void (*putc)(uint8_t);

// Serial pig debug
void debug_server_serial_putc(uint8_t c) {
  if (c == '\n') {
    __asm__ __volatile__("out %%al, %%dx" : : "a"('\r'), "d"(0x3F8));
  }
  __asm__ __volatile__("out %%al, %%dx" : : "a"(c), "d"(0x3F8));
}

static void (*putc_saved)(uint8_t);

void debug_server_serial_on(void) {
  putc_saved = putc;
  putc = debug_server_serial_putc;
}

void debug_server_serial_off(void) {
  putc = putc_saved;
}
