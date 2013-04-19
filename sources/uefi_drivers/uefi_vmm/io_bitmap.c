#include "io_bitmap.h"

#include "string.h"
#include "debug.h"

uint8_t io_bitmap_a[0x1000] __attribute__((aligned(0x1000)));
uint8_t io_bitmap_b[0x1000] __attribute__((aligned(0x1000)));

void io_bitmap_setup(void) {
  memset(&io_bitmap_a[0], 0, 0x1000);
  memset(&io_bitmap_b[0], 0, 0x1000);
}

void msr_bitmap_set_for_port(uint64_t port) {
  if (port < 0x8000) {
    io_bitmap_a[port / 8] |= (1 << (port % 8));
  } else {
    io_bitmap_b[port / 8] |= (1 << (port % 8));
  }
}

uint64_t io_bitmap_get_ptr_a(void) {
  return (uint64_t) &io_bitmap_a[0];
}

uint64_t io_bitmap_get_ptr_b(void) {
  return (uint64_t) &io_bitmap_b[0];
}
