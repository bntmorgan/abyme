#ifndef __VMM_SETUP_H__
#define __VMM_SETUP_H__

void vmm_main(void);

void vmm_setup(void);

void vmm_create_vmxon_and_vmcs_regions(void);

void vmm_vm_setup_and_launch(void);

#endif
