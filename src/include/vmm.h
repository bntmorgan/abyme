#ifndef __VMM_H__
#define __VMM_H__

#include "mod.h"
#include "vmem.h"
#include "pmem.h"
#include "kernel.h"

#define VMM_STACK_SIZE 0x4000

typedef struct {
  uint64_t PML4[512] __attribute__((aligned(0x1000)));
  uint64_t PDPT_PML40[512] __attribute__((aligned(0x1000)));
  uint64_t PD_PDPT0_PML40[512] __attribute__((aligned(0x1000)));
} __attribute__((aligned(8))) ept_info_t;

typedef struct {
  mod_info_t mod_info;
  pmem_mmap_t pmem_mmap;
  vmem_info_t vmem_info;
  kernel_info_t kernel_info;
  ept_info_t ept_info;
} vmm_info_t;

#endif
