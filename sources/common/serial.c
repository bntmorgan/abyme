#include "stdio.h"

extern void (*putc)(uint8_t);

#define PORT 0x3F8

int debug_server_is_transmit_empty(void) {
  int port = PORT + 5, res;
  __asm__ __volatile("in %%dx, %%al" : "=a"(res) : "d"(port));
  return res & 0x20;
}

// Serial pig debug
void debug_server_serial_putc(uint8_t c) {
  if (c == '\n') {
    while(debug_server_is_transmit_empty() == 0);
    __asm__ __volatile__("out %%al, %%dx" : : "a"('\r'), "d"(PORT));
  }
  while(debug_server_is_transmit_empty() == 0);
  __asm__ __volatile__("out %%al, %%dx" : : "a"(c), "d"(PORT));
}

static void (*putc_saved)(uint8_t);

void debug_server_serial_on(void) {
  putc_saved = putc;
  putc = debug_server_serial_putc;
}

void debug_server_serial_off(void) {
  putc = putc_saved;
}
