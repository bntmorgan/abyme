#ifndef __MEM_H__
#define __MEM_H__

#include "vmm_info.h"

void pmem_print_info(pmem_mmap_t *pmem_info);

void pmem_fix_info(pmem_mmap_t *pmem_info, uint64_t vmm_start);

#endif
