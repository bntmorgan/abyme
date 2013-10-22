#include <efi.h>
#include <efilib.h>

#include "debug_eth.h"

inline uint32_t debug_reg_get(uint32_t reg) {
  uint8_t *bar0 = eth_get_bar0();
  return cpu_mem_readd(bar0 + reg);
}

inline void debug_print_reg_stat() {
  Print(L"Crc error count 0x%x\n", debug_reg_get(STAT_CRCERRS));
  Print(L"Reception error count 0x%x\n", debug_reg_get(STAT_RXERRC));
  Print(L"Missed packet count 0x%x\n", debug_reg_get(STAT_MPC));
  Print(L"Carrier extention error count 0x%x\n", debug_reg_get(STAT_CEXTERR));
  Print(L"Receive unsupported count 0x%x\n", debug_reg_get(STAT_FCRUC));
  Print(L"Good packet received count 0x%x\n", debug_reg_get(STAT_GPRC));
}

void dump(void *fields, uint32_t fds, uint32_t fdss, uint64_t offset, uint32_t step) {
  uint32_t i, j;
  uint32_t cycles = fdss / fds;
  for (i = 0; i < cycles; i++) {
    if (i % 4 == 0) {
      Print(L"%08x: ", i * step + offset);
    }
    for (j = 0; j < fds; j++) {
      Print(L"%02x", *((uint8_t*)fields + i * fds + (fds - j - 1)));
    }
    Print(L" ");
    if (i % 4 == 3) {
      Print(L"\n");
    }
  }
  //if (i % 4 != 3) {
    Print(L"\n");
  //}
}
