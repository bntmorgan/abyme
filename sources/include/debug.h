#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "types.h"
#include "hardware/cpu.h"

/**
 * Proc state
 */
struct core_gpr {
  uintptr_t cs;
  uintptr_t ds;
  uintptr_t es;
  uintptr_t fs;
  uintptr_t gs;
  uintptr_t tr;
  uintptr_t rip;
  uintptr_t rdi;
  uintptr_t rsi;
  uintptr_t rbp;
  uintptr_t rsp;
  uintptr_t rdx;
  uintptr_t rcx;
  uintptr_t rbx;
  uintptr_t rax;
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

