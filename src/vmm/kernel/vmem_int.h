#ifndef __VMEM_INT_H__
#define __VMEM_INT_H__

#include <stdint.h>

#include "include/vmem.h"

void vmem_setup(vmem_info_t *vmem_info, uint64_t physical_start, uint64_t virtual_start);

void vmem_print_info(void);

uint64_t vmem_addr_linear_to_logical_ds(uint64_t addr);

#endif
