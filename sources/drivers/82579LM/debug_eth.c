#include <efi.h>
#include <efilib.h>

#include "debug_eth.h"
#include "stdio.h"

void debug_print_reg_stat() {
  INFO("Crc error count 0x%x\n", debug_reg_get(REG_CRCERRS));
  INFO("Reception error count 0x%x\n", debug_reg_get(REG_RXERRC));
  INFO("Missed packet count 0x%x\n", debug_reg_get(REG_MPC));
  INFO("Carrier extention error count 0x%x\n", debug_reg_get(REG_CEXTERR));
  INFO("Receive unsupported count 0x%x\n", debug_reg_get(REG_FCRUC));
  INFO("Good packet received count 0x%x\n", debug_reg_get(REG_GPRC));
}

void dump(void *fields, uint32_t fds, uint32_t fdss, uint64_t offset, uint32_t step) {
  uint32_t i, j;
  uint32_t cycles = fdss / fds;
  for (i = 0; i < cycles; i++) {
    if (i % 4 == 0) {
      INFO("%08x: ", i * step + offset);
    }
    for (j = 0; j < fds; j++) {
      INFO("%02x", *((uint8_t*)fields + i * fds + (fds - j - 1)));
    }
    INFO(" ");
    if (i % 4 == 3) {
      INFO("\n");
    }
  }
  //if (i % 4 != 3) {
    INFO("\n");
  //}
}
