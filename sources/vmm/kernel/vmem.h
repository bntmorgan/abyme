#ifndef __VMEM_H__
#define __VMEM_H__

#include "types.h"

#include "vmm_info.h"

/*
 * The GDT is configured in flat mode by the loader and we use identity mapping
 * for pagination (without offset). As a consequence,
 * virtual address == physical address.
 * See [Intel_August_2012], volume 3, section 3.2.1.
 */
#define VMEM_ADDR_VIRTUAL_TO_PHYSICAL(addr)	((uint64_t) addr)
#define VMEM_ADDR_PHYSICAL_TO_VIRTUAL(addr)	((uint64_t) addr)

void vmem_print_info(void);

#endif
