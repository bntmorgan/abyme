#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "types.h"
#include "hardware/cpu.h"

/**
 * Proc state
 */
struct core_state {
  gpr64_t gpr;
  uintptr_t cr0;
  uintptr_t cr2;
  uintptr_t cr3;
  uintptr_t cr4;
  uintptr_t cs;
  uintptr_t ds;
  uintptr_t es;
  uintptr_t fs;
  uintptr_t gs;
  uintptr_t tr;
};

/**
 * Fields is the address of a 16 bits fields structure
 */
void dump(void *fields, uint32_t fds, uint32_t fdss, uint32_t offset, uint32_t step);

void dump_core_state();

#endif

