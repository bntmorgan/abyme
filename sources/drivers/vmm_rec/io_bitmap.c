#include "io_bitmap.h"

#include "string.h"
#include "debug.h"
#include "efiw.h"
#include "vmm.h"

uint8_t *io_bitmap_a;
uint8_t *io_bitmap_b;

uint8_t (*io_bitmap_a_pool)[0x1000];
uint8_t (*io_bitmap_b_pool)[0x1000];

void io_bitmap_setup(void) {
  io_bitmap_a = efi_allocate_pages(1);
  io_bitmap_b = efi_allocate_pages(1);
  io_bitmap_a_pool = efi_allocate_pages(VM_NB);
  io_bitmap_b_pool = efi_allocate_pages(VM_NB);
  memset(&io_bitmap_a[0], 0, 0x1000);
  memset(&io_bitmap_b[0], 0, 0x1000);
  memset(&io_bitmap_a_pool[0][0], 0, 0x1000 * VM_NB);
  memset(&io_bitmap_b_pool[0][0], 0, 0x1000 * VM_NB);
}

void io_bitmap_set_for_port(uint64_t port) {
  if (port < 0x8000) {
    io_bitmap_a[port / 8] |= (1 << (port % 8));
  } else {
    io_bitmap_b[(port - 0x8000) / 8] |= (1 << (port % 8));
  }
}

/**
 * Initialise the two I/O bitmaps a and b with the host configuration
 */
void io_bitmap_clone_a_b(uint8_t *a, uint8_t *b) {
  memcpy(a, io_bitmap_a, 0x1000);
  memcpy(b, io_bitmap_b, 0x1000);
}

/**
 * Create an executive I/O bitmap with the host configuration and the
 * virtualized hypervisor bitmap
 */
void io_bitmap_or(uint8_t *a_dst, uint8_t *a_src, uint8_t *b_dst,
    uint8_t *b_src) {
  uint32_t i;
  for (i = 0; i < 0x1000; i++) {
    a_dst[i] = io_bitmap_a[i] | a_src[i];
    b_dst[i] = io_bitmap_b[i] | b_src[i];
  }
}

uint64_t io_bitmap_get_ptr_a(void) {
  return (uint64_t) &io_bitmap_a[0];
}

uint64_t io_bitmap_get_ptr_b(void) {
  return (uint64_t) &io_bitmap_b[0];
}
