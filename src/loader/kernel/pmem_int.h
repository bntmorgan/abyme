#ifndef __MEM_INT_H__
#define __MEM_INT_H__

#include <stdint.h>

#include "include/pmem.h"

#include "multiboot_int.h"

void pmem_setup(mb_info_t *mb_info);

void pmem_print_info(mb_info_t *mb_info);

uint64_t pmem_get_stealth_area(uint32_t size, uint32_t alignment);

void pmem_copy_info(pmem_mmap_t *dest);

#endif
