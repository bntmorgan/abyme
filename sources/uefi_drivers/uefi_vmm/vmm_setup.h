#ifndef __VMM_SETUP_H__
#define __VMM_SETUP_H__

#include "vmm_info.h"

void vmm_main(void);

void vmm_setup(void);

void vmm_create_vmxon_and_vmcs_regions(void);

void vmm_vm_setup_and_launch(void);

void vmm_ept_setup(ept_info_t *ept_info/*, uintptr_t vmm_physical_start, uintptr_t vmm_size*/);

#endif
