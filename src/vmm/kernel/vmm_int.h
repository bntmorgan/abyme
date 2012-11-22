#ifndef __VMM_INT_H__
#define __VMM_INT_H__

void vmm_create_vmcs(void);

void vmm_vmx_cr0_fixed(void);

void vmm_vmx_cr4_fixed(void);

void vmm_vmxon(void);

#endif
