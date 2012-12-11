#ifndef __VMM_VMCS_H__
#define __VMM_VMCS_H__

void vmm_vmcs_fill_guest_state_fields(void);

void vmm_vmcs_fill_host_state_fields(void);

void vmm_vmcs_fill_vm_exec_control_fields(void);

void vmm_vmcs_fill_vm_exit_control_fields(void);

void vmm_vmcs_fill_vm_entry_control_fields(void);

#endif
