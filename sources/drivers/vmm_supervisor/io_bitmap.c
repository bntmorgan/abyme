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

#include "io_bitmap.h"

#include "string.h"
#include "debug.h"

uint8_t io_bitmap_a[0x1000] __attribute__((aligned(0x1000)));
uint8_t io_bitmap_b[0x1000] __attribute__((aligned(0x1000)));

void io_bitmap_setup(void) {
  memset(&io_bitmap_a[0], 0, 0x1000);
  memset(&io_bitmap_b[0], 0, 0x1000);
}

void io_bitmap_set_for_port(uint64_t port) {
  if (port < 0x8000) {
    io_bitmap_a[port / 8] |= (1 << (port % 8));
  } else {
    io_bitmap_b[(port - 0x8000) / 8] |= (1 << (port % 8));
  }
}

uint64_t io_bitmap_get_ptr_a(void) {
  return (uint64_t) &io_bitmap_a[0];
}

uint64_t io_bitmap_get_ptr_b(void) {
  return (uint64_t) &io_bitmap_b[0];
}
