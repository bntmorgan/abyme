#include "debug.h"
#include "stdio.h"

void dump(void *fields, uint32_t fds, uint32_t fdss, uint32_t offset, uint32_t step) { \
  uint32_t i, j;
  uint32_t cycles = fdss / fds;
  for (i = 0; i < cycles; i++) {
    if (i % 4 == 0) {
      printk("%08x: ", i * step + offset);
    }
    for (j = 0; j < fds; j++) {
      printk("%02x", *((uint8_t*)fields + i * fds + (fds - j - 1)));
    }
    printk(" ");
    if (i % 4 == 3) {
      printk("\n");
    }
  }
  if (i % 4 != 3) {
    printk("\n");
  }
}
