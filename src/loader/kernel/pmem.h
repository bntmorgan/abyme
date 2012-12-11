#ifndef __PMEM_H__
#define __PMEM_H__

#include "types.h"

#include "multiboot.h"

void pmem_setup(multiboot_info_t *multiboot_info);

void pmem_print_info(multiboot_info_t *multiboot_info);

uint64_t pmem_get_stealth_area(uint32_t size, uint32_t alignment);

void pmem_copy_info(pmem_mmap_t *dest);

#endif
