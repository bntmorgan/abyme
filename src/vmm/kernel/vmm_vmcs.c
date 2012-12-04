#include "vmm_int.h"
#include "vmem_int.h"

#include "arch/cpu_int.h"
#include "arch/msr_int.h"

#define FIXME 0

void vmm_vmcs_fill_guest_state_fields(void) {
  uint32_t eax;
  uint32_t edx;

  /**
   * 24.4: Guest-state area.
   */
  // 24.4.1: Guest Register State.
  // Reflects the state of a physical processor at power-on reset, as described
  // in Vol 3A, 9-2, p294 (Table 9-1. IA-32 Processor States Following
  // Power-up, Reset, or INIT)
  vmm_vmcs_write(GUEST_CR0, 0x60000010);
  vmm_vmcs_write(GUEST_CR3, 0);
  vmm_vmcs_write(GUEST_CR4, 0);
  vmm_vmcs_write(GUEST_DR7, 0x00000400);
  vmm_vmcs_write(GUEST_RSP, 0);
  vmm_vmcs_write(GUEST_RIP, 0x0000FFF0);
  vmm_vmcs_write(GUEST_RFLAGS, 0x00000002);

  vmm_vmcs_write(GUEST_CS_SELECTOR, 0xF000);
  vmm_vmcs_write(GUEST_CS_BASE, 0xFFFF0000);
  vmm_vmcs_write(GUEST_CS_LIMIT, 0xFFFF);
  vmm_vmcs_write(GUEST_CS_AR_BYTES, 0x93 /* Present, R/W, Accessed */);
  vmm_vmcs_write(GUEST_SS_SELECTOR, 0);
  vmm_vmcs_write(GUEST_SS_BASE, 0);
  vmm_vmcs_write(GUEST_SS_LIMIT, 0xFFFF);
  vmm_vmcs_write(GUEST_SS_AR_BYTES, 0x93 /* Present, R/W, Accessed */);
  vmm_vmcs_write(GUEST_DS_SELECTOR, 0);
  vmm_vmcs_write(GUEST_DS_BASE, 0);
  vmm_vmcs_write(GUEST_DS_LIMIT, 0xFFFF);
  vmm_vmcs_write(GUEST_DS_AR_BYTES, 0x93 /* Present, R/W, Accessed */);
  vmm_vmcs_write(GUEST_ES_SELECTOR, 0);
  vmm_vmcs_write(GUEST_ES_BASE, 0);
  vmm_vmcs_write(GUEST_ES_LIMIT, 0xFFFF);
  vmm_vmcs_write(GUEST_ES_AR_BYTES, 0x93 /* Present, R/W, Accessed */);
  vmm_vmcs_write(GUEST_FS_SELECTOR, 0);
  vmm_vmcs_write(GUEST_FS_BASE, 0);
  vmm_vmcs_write(GUEST_FS_LIMIT, 0xFFFF);
  vmm_vmcs_write(GUEST_FS_AR_BYTES, 0x93 /* Present, R/W, Accessed */);
  vmm_vmcs_write(GUEST_GS_SELECTOR, 0);
  vmm_vmcs_write(GUEST_GS_BASE, 0);
  vmm_vmcs_write(GUEST_GS_LIMIT, 0xFFFF);
  vmm_vmcs_write(GUEST_GS_AR_BYTES, 0x93 /* Present, R/W, Accessed */);
  vmm_vmcs_write(GUEST_LDTR_SELECTOR, 0);
  vmm_vmcs_write(GUEST_LDTR_BASE, 0);
  vmm_vmcs_write(GUEST_LDTR_LIMIT, 0xFFFF);
  vmm_vmcs_write(GUEST_LDTR_AR_BYTES, 0x82 /* Present, R/W */);
  vmm_vmcs_write(GUEST_TR_SELECTOR, 0);
  vmm_vmcs_write(GUEST_TR_BASE, 0);
  vmm_vmcs_write(GUEST_TR_LIMIT, 0xFFFF);
  vmm_vmcs_write(GUEST_TR_AR_BYTES, 0x82 /* Present, R/W */);

  vmm_vmcs_write(GUEST_GDTR_BASE, 0);
  vmm_vmcs_write(GUEST_GDTR_LIMIT, 0xFFFF);
  vmm_vmcs_write(GUEST_IDTR_BASE, 0);
  vmm_vmcs_write(GUEST_IDTR_LIMIT, 0xFFFF);

  // XXX: BOCHS doesn't know MSR_IA32_DEBUGCTL
  //msr_read(MSR_ADDRESS_IA32_DEBUGCTL, &eax, &edx);
  eax = 0; edx = 0;
  vmm_vmcs_write(GUEST_IA32_DEBUGCTL, eax);
  vmm_vmcs_write(GUEST_IA32_DEBUGCTL_HIGH, edx);
  msr_read(MSR_ADDRESS_IA32_SYSENTER_CS, &eax, &edx);
  vmm_vmcs_write(GUEST_SYSENTER_CS, eax);
  msr_read(MSR_ADDRESS_IA32_SYSENTER_ESP, &eax, &edx);
  vmm_vmcs_write(GUEST_SYSENTER_ESP, eax);
  msr_read(MSR_ADDRESS_IA32_SYSENTER_EIP, &eax, &edx);
  vmm_vmcs_write(GUEST_SYSENTER_EIP, eax);
  // GUEST_IA32_PERF_GLOBAL_CTRL{,_HIGH}, GUEST_IA32_PAT{,_HIGH},
  // GUEST_IA32_EFER{,_HIGH}, GUEST_SMBASE unused

  // 24.4.2: Guest Non-Register State.
  vmm_vmcs_write(GUEST_ACTIVITY_STATE, 0);
  vmm_vmcs_write(GUEST_INTERRUPTIBILITY_INFO, 0);
  vmm_vmcs_write(GUEST_PENDING_DBG_EXCEPTIONS, 0);
  vmm_vmcs_write(VMCS_LINK_POINTER, 0xffffffff);
  vmm_vmcs_write(VMCS_LINK_POINTER_HIGH, 0xffffffff);
  // GUEST_VMX_PREEMPT_TIMER_VAL, GUEST_PDPTR[0-3]{,_HIGH},
  // GUEST_INTERRUPT_STATUS unused
}

void vmm_vmcs_fill_host_state_fields(void) {
  uint32_t eax;
  uint32_t edx;
  uint64_t reg;
  gdt_lm_ptr_t gdt_ptr, idt_ptr; // TODO: create idt_ptr_t?

  /**
   * 24.5: Host-state area.
   */
  cpu_read_cr0(&reg);
  vmm_vmcs_write(HOST_CR0, reg);
  cpu_read_cr3(&reg);
  vmm_vmcs_write(HOST_CR3, reg);
  cpu_read_cr4(&reg);
  vmm_vmcs_write(HOST_CR4, reg);
  vmm_vmcs_write(HOST_RSP, vmm_stack);
  vmm_vmcs_write(HOST_RIP, (uint64_t) vmm_vm_exit_handler);

  cpu_read_cs(&reg);
  vmm_vmcs_write(HOST_CS_SELECTOR, reg & 0xf8);
  cpu_read_ss(&reg);
  vmm_vmcs_write(HOST_SS_SELECTOR, reg & 0xf8);
  cpu_read_ds(&reg);
  vmm_vmcs_write(HOST_DS_SELECTOR, reg & 0xf8);
  cpu_read_es(&reg);
  vmm_vmcs_write(HOST_ES_SELECTOR, reg & 0xf8);
  cpu_read_fs(&reg);
  vmm_vmcs_write(HOST_FS_SELECTOR, reg & 0xf8);
  cpu_read_gs(&reg);
  vmm_vmcs_write(HOST_GS_SELECTOR, reg & 0xf8);
  cpu_read_tr(&reg);
  vmm_vmcs_write(HOST_TR_SELECTOR, reg & 0xf8);

  cpu_read_gdt((uint8_t*) &gdt_ptr);
  cpu_read_idt((uint8_t*) &idt_ptr);
  cpu_read_fs(&reg);
  vmm_vmcs_write(HOST_FS_BASE, cpu_get_seg_desc_base(gdt_ptr.base, reg));
  cpu_read_gs(&reg);
  vmm_vmcs_write(HOST_GS_BASE, cpu_get_seg_desc_base(gdt_ptr.base, reg));
  cpu_read_tr(&reg);
  vmm_vmcs_write(HOST_TR_BASE, cpu_get_seg_desc_base(gdt_ptr.base, reg));
  vmm_vmcs_write(HOST_GDTR_BASE, gdt_ptr.base);
  vmm_vmcs_write(HOST_IDTR_BASE, idt_ptr.base);

  msr_read(MSR_ADDRESS_IA32_SYSENTER_CS, &eax, &edx);
  vmm_vmcs_write(HOST_IA32_SYSENTER_CS, eax);
  msr_read(MSR_ADDRESS_IA32_SYSENTER_ESP, &eax, &edx);
  vmm_vmcs_write(HOST_IA32_SYSENTER_ESP, eax);
  msr_read(MSR_ADDRESS_IA32_SYSENTER_EIP, &eax, &edx);
  vmm_vmcs_write(HOST_IA32_SYSENTER_EIP, eax);
  // HOST_IA32_PERF_GLOBAL_CTRL{,_HIGH}, HOST_IA32_PAT{,_HIGH},
  // HOST_IA32_EFER{,_HIGH} unused
}

void vmm_vmcs_fill_vm_exec_control_fields(void) {
  uint32_t eax, ebx, ecx;
  int cdefault;

  // We need to read the bit 55 of the VMX_BASIC MSR
  // to know if we are in default0 or default1 mode
  msr_read(MSR_ADDRESS_IA32_VMX_BASIC, &eax, &ebx);
  cdefault = (0x1 << 23) & ebx;

  /**
   * 24.6: VM-Execution Control Fields.
   */
  // 24.6.1: Pin-Based VM-Execution Controls
  if (cdefault) {
    msr_read(MSR_ADDRESS_IA32_VMX_TRUE_PINBASED_CTLS, &eax, &ebx);
    // Set to 1 the non 0-allowed bits
    ecx = eax;
    // Set to 0 the non 1-allowed bits
    ecx &= ebx;
  } else {
    msr_read(MSR_ADDRESS_IA32_VMX_PINBASED_CTLS, &eax, &ebx);
    ecx = eax;
    ecx &= ebx;
  }
  vmm_vmcs_write(PIN_BASED_VM_EXEC_CONTROL, ecx); // From MSRs IA32_VMX_PINBASED_CTLS and IA32_VMX_TRUE_PINBASED_CTLS

  // 24.6.2: Processor-Based VM-Execution Controls
  if (cdefault) {
    msr_read(MSR_ADDRESS_IA32_VMX_TRUE_PROCBASED_CTLS, &eax, &ebx);
    ecx = eax;
    ecx &= ebx;
  } else {
    msr_read(MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS, &eax, &ebx);
    ecx = eax;
    ecx &= ebx;
  }
  vmm_vmcs_write(CPU_BASED_VM_EXEC_CONTROL, ecx); // From MSRs IA32_VMX_PROCBASED_CTLS and IA32_VMX_TRUE_PROCBASED_CTLS

  vmm_vmcs_write(SECONDARY_VM_EXEC_CONTROL, FIXME); // From MSR IA32_VMX_PROCBASED_CTLS2

  // 24.6.3: Exception Bitmap
  vmm_vmcs_write(EXCEPTION_BITMAP, 0);
  vmm_vmcs_write(PAGE_FAULT_ERROR_CODE_MASK, 0);
  vmm_vmcs_write(PAGE_FAULT_ERROR_CODE_MATCH, 0);

  // 24.6.4: I/O-Bitmap Addresses
  vmm_vmcs_write(IO_BITMAP_A, 0);
  vmm_vmcs_write(IO_BITMAP_A_HIGH, 0);
  vmm_vmcs_write(IO_BITMAP_B, 0);
  vmm_vmcs_write(IO_BITMAP_B_HIGH, 0);

  // 24.6.5: Time-Stamp Counter Offset
  vmm_vmcs_write(TSC_OFFSET, 0);
  vmm_vmcs_write(TSC_OFFSET_HIGH, 0);

  // 24.6.6: Guest/Host Masks and Read Shadows for CR0 and CR4
  vmm_vmcs_write(CR0_GUEST_HOST_MASK, 0);
  vmm_vmcs_write(CR0_READ_SHADOW, 0);
  vmm_vmcs_write(CR4_GUEST_HOST_MASK, 0);
  vmm_vmcs_write(CR4_READ_SHADOW, 0);

  // 24.6.7: CR3-Target Controls
  vmm_vmcs_write(CR3_TARGET_COUNT, 0);
  vmm_vmcs_write(CR3_TARGET_VALUE0, 0);
  vmm_vmcs_write(CR3_TARGET_VALUE1, 0);
  vmm_vmcs_write(CR3_TARGET_VALUE2, 0);
  vmm_vmcs_write(CR3_TARGET_VALUE3, 0);

  // 24.6.8: Controls for APIC Virtualization (optional, unused)
  // APIC_ACCESS_ADDR{,_HIGH}, VIRTUAL_APIC_PAGE_ADDR{,_HIGH}, TPR_THRESHOLD,
  // EOI_EXIT_BITMAP[0-4]{,_HIGH}, POSTED_INTR_NOTIF_VECTOR,
  // POSTED_INTR_DESC_ADDRESS{,_HIGH} unused

  // 24.6.9: MSR-Bitmap Address (optional, unused)
  // MSR_BITMAP{,_HIGH} unused

  // 24.6.10: Executive-VMCS Pointer (optional, unused)
  // EXEC_VMCS_PTR{,_HIGH} unused

  // 24.6.11: Extended-Page-Table Pointer (EPTP)
  //vmm_vmcs_write(EPT_POINTER, FIXME);
  //vmm_vmcs_write(EPT_POINTER_HIGH, FIXME);

  // 24.6.12: Virtual-Processor Identifier (VPID) (optional, unused)
  // VIRTUAL_PROCESSOR_ID unused

  // 24.6.13: Controls for PAUSE-Loop Exiting (optional, unused)
  // PLE_GAP, PLE_WINDOW unused

  // 24.6.14: VM-Function Controls (optional, unused)
  // VM_FUNCTION_CONTROLS{,_HIGH}, EPTP_LIST_ADDR{,_HIGH} unused
}

void vmm_vmcs_fill_vm_exit_control_fields(void) {
  /**
   * 24.7: VM-Exit Control Fields.
   */
  // 24.7.1: VM-Exit Controls
  vmm_vmcs_write(VM_EXIT_CONTROLS, FIXME); // From MSRs IA32_VMX_EXIT_CTLS and IA32_VMX_TRUE_EXIT_CTLS

  // 24.7.2: VM-Exit Controls for MSRs
  vmm_vmcs_write(VM_EXIT_MSR_STORE_COUNT, 0);
  // VM_EXIT_MSR_STORE_ADDR{,_HIGH} unused
  vmm_vmcs_write(VM_EXIT_MSR_LOAD_COUNT, 0);
  // VM_EXIT_MSR_LOAD_ADDR{,_HIGH} unused
}

void vmm_vmcs_fill_vm_entry_control_fields(void) {
  /**
   * 24.8: VM-Entry Control fields.
   */
  // 24.8.1: VM-Entry Controls
  vmm_vmcs_write(VM_EXIT_CONTROLS, FIXME); // From MSRs IA32_VMX_ENTRY_CTLS and IA32_VMX_TRUE_ENTRY_CTLS

  // 24.8.2: VM-Entry Controls for MSRs.
  vmm_vmcs_write(VM_EXIT_MSR_LOAD_COUNT, 0);
  // VM_ENTRY_MSR_LOAD_ADDR{,_HIGH} unused

  // 24.8.3: VM-Entry Controls for Event Injection
  vmm_vmcs_write(VM_ENTRY_INTR_INFO_FIELD, 0);
  // VM_ENTRY_EXCEPTION_ERROR_CODE and VM_ENTRY_INSTRUCTION_LEN unused
}
