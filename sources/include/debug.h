#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <efi.h>
#include "types.h"
#include "hardware/cpu.h"

/**
 * Proc state
 */
struct core_gpr {
  uint64_t rax;
  uint64_t rbx;
  uint64_t rcx;
  uint64_t rdx;
  uint64_t rsp;
  uint64_t rbp;
  uint64_t rsi;
  uint64_t rdi;
  uint64_t rip;
  uint16_t tr;
  uint16_t gs;
  uint16_t fs;
  uint16_t es;
  uint16_t ds;
  uint16_t ss;
  uint16_t cs;
};

struct core_cr {
  uint64_t cr0;
  uint64_t cr2;
  uint64_t cr3;
  uint64_t cr4;
};

/**
 * Fields is the address of a 16 bits fields structure
 */
void dump(void *fields, uint32_t fds, uint32_t fdss, uint32_t offset, uint32_t step);

void dump_core_state(struct core_gpr *gpr, struct core_cr *cr);
void read_core_state(struct core_gpr *gpr, struct core_cr *cr);

#endif

