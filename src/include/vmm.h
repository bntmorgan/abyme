#ifndef __VMM_H__
#define __VMM_H__

#include "mod.h"
#include "vmem.h"
#include "pmem.h"
#include "kernel.h"

#define VMM_STACK_SIZE 0x4000

typedef struct {
  mod_info_t mod_info;
  pmem_mmap_t pmem_mmap;
  vmem_info_t vmem_info;
  kernel_info_t kernel_info;
} vmm_info_t;

#endif
