#ifndef __VMEM_H__
#define __VMEM_H__

#include "types.h"

#include "vmm_info.h"

void vmem_print_info(void);

uint64_t vmem_addr_linear_to_logical_ds(uint64_t addr);

uint64_t vmem_virtual_address_to_physical_address(void *addr);

#endif
