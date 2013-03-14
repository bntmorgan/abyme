#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "types.h"
#include "hardware/cpu.h"

/**
 * Proc state
 */
struct core_gpr {
  uintptr_t rax;
  uintptr_t rbx;
  uintptr_t rcx;
  uintptr_t rdx;
  uintptr_t rsp;
  uintptr_t rbp;
  uintptr_t rsi;
  uintptr_t rdi;
  uintptr_t rip;
  uint16_t tr;
  uint16_t gs;
  uint16_t fs;
  uint16_t es;
  uint16_t ds;
  uint16_t ss;
  uint16_t cs;
};

struct core_state {
  uintptr_t cr0;
  uintptr_t cr2;
  uintptr_t cr3;
  uintptr_t cr4;
};

/**
 * Fields is the address of a 16 bits fields structure
 */
void dump(void *fields, uint32_t fds, uint32_t fdss, uint32_t offset, uint32_t step);

void dump_core_state(struct core_gpr *gpr);

#endif

