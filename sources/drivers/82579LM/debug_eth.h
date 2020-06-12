#ifndef __DEBUG_ETH_H__
#define __DEBUG_ETH_H__

#include "82579LM.h"
#include "cpu.h"

static inline uint32_t debug_reg_get(uint32_t reg) {
  uint8_t *bar0 = eth_get_bar0();
  return cpu_mem_readd(bar0 + reg);
}

void debug_print_reg_stat();

#endif
