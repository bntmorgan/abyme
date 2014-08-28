#ifndef __NESTED_VMX_H__
#define __NESTED_VMX_H__

#define NESTED_GUEST_FIELDS \
          GUEST_ES_SELECTOR, \
          GUEST_CS_SELECTOR, \
          GUEST_SS_SELECTOR, \
          GUEST_DS_SELECTOR, \
          GUEST_FS_SELECTOR, \
          GUEST_GS_SELECTOR, \
          GUEST_LDTR_SELECTOR, \
          GUEST_TR_SELECTOR, \
          GUEST_INTERRUPT_STATUS, \
          /*VMCS_LINK_POINTER,*/ \
          /*VMCS_LINK_POINTER_HIGH,*/ \
          GUEST_IA32_DEBUGCTL, \
          GUEST_IA32_DEBUGCTL_HIGH, \
          GUEST_IA32_PAT, \
          GUEST_IA32_PAT_HIGH, \
          GUEST_IA32_EFER, \
          GUEST_IA32_EFER_HIGH, \
          GUEST_IA32_PERF_GLOBAL_CTRL, \
          GUEST_IA32_PERF_GLOBAL_CTRL_HIGH, \
          GUEST_PDPTR0, \
          GUEST_PDPTR0_HIGH, \
          GUEST_PDPTR1, \
          GUEST_PDPTR1_HIGH, \
          GUEST_PDPTR2, \
          GUEST_PDPTR2_HIGH, \
          GUEST_PDPTR3, \
          GUEST_PDPTR3_HIGH, \
          GUEST_ES_LIMIT, \
          GUEST_CS_LIMIT, \
          GUEST_SS_LIMIT, \
          GUEST_DS_LIMIT, \
          GUEST_FS_LIMIT, \
          GUEST_GS_LIMIT, \
          GUEST_LDTR_LIMIT, \
          GUEST_TR_LIMIT, \
          GUEST_GDTR_LIMIT, \
          GUEST_IDTR_LIMIT, \
          GUEST_ES_AR_BYTES, \
          GUEST_CS_AR_BYTES, \
          GUEST_SS_AR_BYTES, \
          GUEST_DS_AR_BYTES, \
          GUEST_FS_AR_BYTES, \
          GUEST_GS_AR_BYTES, \
          GUEST_LDTR_AR_BYTES, \
          GUEST_TR_AR_BYTES, \
          GUEST_INTERRUPTIBILITY_INFO, \
          GUEST_ACTIVITY_STATE, \
          GUEST_SMBASE, \
          GUEST_SYSENTER_CS, \
          /*VMX_PREEMPTION_TIMER_VALUE,*/ \
          GUEST_CR0, \
          GUEST_CR3, \
          GUEST_CR4, \
          GUEST_ES_BASE, \
          GUEST_CS_BASE, \
          GUEST_SS_BASE, \
          GUEST_DS_BASE, \
          GUEST_FS_BASE, \
          GUEST_GS_BASE, \
          GUEST_LDTR_BASE, \
          GUEST_TR_BASE, \
          GUEST_GDTR_BASE, \
          GUEST_IDTR_BASE, \
          GUEST_DR7, \
          GUEST_RSP, \
          GUEST_RIP, \
          GUEST_RFLAGS, \
          GUEST_PENDING_DBG_EXCEPTIONS, \
          GUEST_SYSENTER_ESP, \
          GUEST_SYSENTER_EIP, \
          VM_ENTRY_CONTROLS //XXX : pour le guest_ia32_mode

#define NESTED_HOST_FIELDS \
          HOST_ES_SELECTOR, \
          HOST_CS_SELECTOR, \
          HOST_SS_SELECTOR, \
          HOST_DS_SELECTOR, \
          HOST_FS_SELECTOR, \
          HOST_GS_SELECTOR, \
          HOST_TR_SELECTOR, \
          HOST_IA32_PAT, \
          HOST_IA32_PAT_HIGH, \
          HOST_IA32_EFER, \
          HOST_IA32_EFER_HIGH, \
          HOST_IA32_PERF_GLOBAL_CTRL, \
          HOST_IA32_PERF_GLOBAL_CTRL_HIGH, \
          HOST_IA32_SYSENTER_CS, \
          HOST_CR0, \
          HOST_CR3, \
          HOST_CR4, \
          HOST_FS_BASE, \
          HOST_GS_BASE, \
          HOST_TR_BASE, \
          HOST_GDTR_BASE, \
          HOST_IDTR_BASE, \
          HOST_IA32_SYSENTER_ESP, \
          HOST_IA32_SYSENTER_EIP, \
          HOST_RSP, \
          HOST_RIP

#define NESTED_HOST_FIELDS_DEST \
          GUEST_ES_SELECTOR, \
          GUEST_CS_SELECTOR, \
          GUEST_SS_SELECTOR, \
          GUEST_DS_SELECTOR, \
          GUEST_FS_SELECTOR, \
          GUEST_GS_SELECTOR, \
          GUEST_TR_SELECTOR, \
          GUEST_IA32_PAT, \
          GUEST_IA32_PAT_HIGH, \
          GUEST_IA32_EFER, \
          GUEST_IA32_EFER_HIGH, \
          GUEST_IA32_PERF_GLOBAL_CTRL, \
          GUEST_IA32_PERF_GLOBAL_CTRL_HIGH, \
          GUEST_SYSENTER_CS, \
          GUEST_CR0, \
          GUEST_CR3, \
          GUEST_CR4, \
          GUEST_FS_BASE, \
          GUEST_GS_BASE, \
          GUEST_TR_BASE, \
          GUEST_GDTR_BASE, \
          GUEST_IDTR_BASE, \
          GUEST_SYSENTER_ESP, \
          GUEST_SYSENTER_EIP, \
          GUEST_RSP, \
          GUEST_RIP

#define NESTED_READ_ONLY_DATA_FIELDS \
          GUEST_PHYSICAL_ADDRESS, \
          GUEST_PHYSICAL_ADDRESS_HIGH, \
          VM_INSTRUCTION_ERROR, \
          VM_EXIT_REASON, \
          VM_EXIT_INTR_INFO, \
          VM_EXIT_INTR_ERROR_CODE, \
          IDT_VECTORING_INFO_FIELD, \
          IDT_VECTORING_ERROR_CODE, \
          VM_EXIT_INSTRUCTION_LEN, \
          VMX_INSTRUCTION_INFO, \
          EXIT_QUALIFICATION, \
          IO_RCX, \
          IO_RSI, \
          IO_RDI, \
          IO_RIP, \
          GUEST_LINEAR_ADDRESS


enum NESTED_STATE {
  NESTED_DISABLED,
  NESTED_HOST_RUNNING,
  NESTED_GUEST_RUNNING
};

extern uint8_t nested_state;

void nested_vmxon(void);

void nested_vmclear(uint8_t *shadow_vmcs);

void nested_vmptrld(uint8_t *shadow_vmcs);

inline void nested_load_shadow_vmcs(void);

void nested_copy_guest_fields(void);

void nested_copy_host_fields(void);

void nested_forward_exit_infos(void);

#endif
