/*
Copyright (C) 2021  Benoît Morgan

This file is part of abyme

abyme is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

abyme is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with abyme.  If not, see <http://www.gnu.org/licenses/>.
*/

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
