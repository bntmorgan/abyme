#ifndef __VMEM_H__
#define __VMEM_H__

#include "types.h"

#include "vmm_info.h"

/*
 * The GDT is configured in flat mode. As a consequence,
 * linear address == logical address.
 * See [Intel_August_2012], volume 3, section 3.2.1.
 */
#define VMEM_ADDR_LINEAR_TO_LOGICAL_DS(addr) addr
#define VMEM_ADDR_LOGICAL_TO_LINEAR_DS(addr) addr

void vmem_setup(vmem_info_t *vmem_info);

void vmem_print_info(void);

#endif
