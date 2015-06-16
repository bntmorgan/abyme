#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "82579LM.h"
#include "cpu.h"

static inline uint32_t debug_reg_get(uint32_t reg) {
  uint8_t *bar0 = eth_get_bar0();
  return cpu_mem_readd(bar0 + reg);
}

void debug_print_reg_stat();
void dump(void *fields, uint32_t fds, uint32_t fdss, uint64_t offset, uint32_t step);

#endif
