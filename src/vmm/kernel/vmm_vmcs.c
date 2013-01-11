#include "vmm.h"
#include "vmm_setup.h"
#include "vmem.h"

#include "string.h"

#include "hardware/cpu.h"
#include "hardware/msr.h"

uint8_t io_bitmap_a[0x1000] __attribute__((aligned(0x1000)));
uint8_t io_bitmap_b[0x1000] __attribute__((aligned(0x1000)));

struct {
  uint8_t low_msrs_read_bitmap[128];
  uint8_t high_msrs_read_bitmap[128];
  uint8_t low_msrs_write_bitmap[128];
  uint8_t high_msrs_write_bitmap[128];
} __attribute__((packed)) msr_bitmaps __attribute__((aligned(0x1000)));

void vmm_vmcs_fill_guest_state_fields(void) {
  /**
   * 24.4: Guest-state area.
   */
  // 24.4.1: Guest Register State.
  // Reflects the state of a physical processor at power-on reset, as described
  // in Vol 3A, 9-2, p294 (Table 9-1. IA-32 Processor States Following
  // Power-up, Reset, or INIT)

  /*
   * Guest CR0 - we have to set CR0 bits according to IA32_VMX_CR0_FIXED0
   * and IA32_VMX_CR0_FIXED1 MSRs (will only enable NE in our case).
   * See [Intel_August_2012], volume 3, section 26.3.1.1.
   * See [Intel_August_2012], volume 3, section 26.8.
   */
  uint64_t cr0 = cpu_adjust64(0x60000010, MSR_ADDRESS_VMX_CR0_FIXED0, MSR_ADDRESS_VMX_CR0_FIXED1);
  cr0 &= ~((1 << 0) | (1 << 31)); // Disable PE and PG (unrestricted guest)
  cpu_vmwrite(GUEST_CR0, cr0);

  /*
   * Guest CR3 - nothing special to do.
   */
  cpu_vmwrite(GUEST_CR3, 0);

  /*
   * Guest CR4 - we have to set CR4 bits according to IA32_VMX_CR4_FIXED0
   * and IA32_VMX_CR4_FIXED1 MSRs (will only enable VMXE in our case).
   * See [Intel_August_2012], volume 3, section 26.3.1.1.
   * See [Intel_August_2012], volume 3, section 26.8.
   */
  uint64_t cr4 = cpu_adjust64(0, MSR_ADDRESS_VMX_CR4_FIXED0, MSR_ADDRESS_VMX_CR4_FIXED1);
  cpu_vmwrite(GUEST_CR4, cr4);

  cpu_vmwrite(GUEST_DR7, 0x00000400);
  cpu_vmwrite(GUEST_RSP, 0);
  //cpu_vmwrite(GUEST_RIP, 0x0000FFF0);
  cpu_vmwrite(GUEST_RIP, 254 * 4);
  cpu_vmwrite(GUEST_RFLAGS, 0x00000002);

  //cpu_vmwrite(GUEST_CS_SELECTOR, 0xF000);
  //cpu_vmwrite(GUEST_CS_BASE, 0xFFFF0000);
  cpu_vmwrite(GUEST_CS_SELECTOR, 0);
  cpu_vmwrite(GUEST_CS_BASE, 0);
  cpu_vmwrite(GUEST_CS_LIMIT, 0xFFFF);
  cpu_vmwrite(GUEST_CS_AR_BYTES, 0x93 /* Present, R/W, Accessed */);
  cpu_vmwrite(GUEST_SS_SELECTOR, 0);
  cpu_vmwrite(GUEST_SS_BASE, 0);
  cpu_vmwrite(GUEST_SS_LIMIT, 0xFFFF);
  cpu_vmwrite(GUEST_SS_AR_BYTES, 0x93 /* Present, R/W, Accessed */);
  cpu_vmwrite(GUEST_DS_SELECTOR, 0);
  cpu_vmwrite(GUEST_DS_BASE, 0);
  cpu_vmwrite(GUEST_DS_LIMIT, 0xFFFF);
  cpu_vmwrite(GUEST_DS_AR_BYTES, 0x93 /* Present, R/W, Accessed */);
  cpu_vmwrite(GUEST_ES_SELECTOR, 0);
  cpu_vmwrite(GUEST_ES_BASE, 0);
  cpu_vmwrite(GUEST_ES_LIMIT, 0xFFFF);
  cpu_vmwrite(GUEST_ES_AR_BYTES, 0x93 /* Present, R/W, Accessed */);
  cpu_vmwrite(GUEST_FS_SELECTOR, 0);
  cpu_vmwrite(GUEST_FS_BASE, 0);
  cpu_vmwrite(GUEST_FS_LIMIT, 0xFFFF);
  cpu_vmwrite(GUEST_FS_AR_BYTES, 0x93 /* Present, R/W, Accessed */);
  cpu_vmwrite(GUEST_GS_SELECTOR, 0);
  cpu_vmwrite(GUEST_GS_BASE, 0);
  cpu_vmwrite(GUEST_GS_LIMIT, 0xFFFF);
  cpu_vmwrite(GUEST_GS_AR_BYTES, 0x93 /* Present, R/W, Accessed */);
  cpu_vmwrite(GUEST_LDTR_SELECTOR, 0);
  cpu_vmwrite(GUEST_LDTR_BASE, 0);
  cpu_vmwrite(GUEST_LDTR_LIMIT, 0xFFFF);
  cpu_vmwrite(GUEST_LDTR_AR_BYTES, 0x82 /* Present, R/W */);
  cpu_vmwrite(GUEST_TR_SELECTOR, 0);
  cpu_vmwrite(GUEST_TR_BASE, 0);
  cpu_vmwrite(GUEST_TR_LIMIT, 0xFFFF);
  cpu_vmwrite(GUEST_TR_AR_BYTES, 0x83 /* Present, 16-bit busy TSS */);

  cpu_vmwrite(GUEST_GDTR_BASE, 0);
  cpu_vmwrite(GUEST_GDTR_LIMIT, 0xFFFF);
  cpu_vmwrite(GUEST_IDTR_BASE, 0);
  cpu_vmwrite(GUEST_IDTR_LIMIT, 0xFFFF);

  cpu_vmwrite(GUEST_IA32_DEBUGCTL, msr_read32(MSR_ADDRESS_IA32_DEBUGCTL));
  cpu_vmwrite(GUEST_IA32_DEBUGCTL_HIGH, msr_read64(MSR_ADDRESS_IA32_DEBUGCTL) >> 32);
  cpu_vmwrite(GUEST_SYSENTER_CS, msr_read32(MSR_ADDRESS_IA32_SYSENTER_CS));
  cpu_vmwrite(GUEST_SYSENTER_ESP, msr_read32(MSR_ADDRESS_IA32_SYSENTER_ESP));
  cpu_vmwrite(GUEST_SYSENTER_EIP, msr_read32(MSR_ADDRESS_IA32_SYSENTER_EIP));
  cpu_vmwrite(GUEST_IA32_EFER, 0);
  cpu_vmwrite(GUEST_IA32_EFER_HIGH, 0);
  // GUEST_IA32_PERF_GLOBAL_CTRL{,_HIGH}, GUEST_IA32_PAT{,_HIGH},
  // GUEST_SMBASE unused

  // 24.4.2: Guest Non-Register State.
  cpu_vmwrite(GUEST_ACTIVITY_STATE, 0);
  cpu_vmwrite(GUEST_INTERRUPTIBILITY_INFO, 0);
  cpu_vmwrite(GUEST_PENDING_DBG_EXCEPTIONS, 0);
  cpu_vmwrite(VMCS_LINK_POINTER, 0xFFFFFFFF);
  cpu_vmwrite(VMCS_LINK_POINTER_HIGH, 0xFFFFFFFF);
  // GUEST_VMX_PREEMPT_TIMER_VAL, GUEST_PDPTR[0-3]{,_HIGH},
  // GUEST_INTERRUPT_STATUS unused
}

void vmm_vmcs_fill_host_state_fields(void) {
  /**
   * 24.5: Host-state area.
   */
  cpu_vmwrite(HOST_CR0, cpu_read_cr0());
  cpu_vmwrite(HOST_CR3, cpu_read_cr3());
  cpu_vmwrite(HOST_CR4, cpu_read_cr4());
  cpu_vmwrite(HOST_RSP, vmm_info->vmm_stack);
  cpu_vmwrite(HOST_RIP, (uint64_t) vmm_vm_exit_handler);

  cpu_vmwrite(HOST_CS_SELECTOR, cpu_read_cs() & 0xf8);
  cpu_vmwrite(HOST_SS_SELECTOR, cpu_read_ss() & 0xf8);
  cpu_vmwrite(HOST_DS_SELECTOR, cpu_read_ds() & 0xf8);
  cpu_vmwrite(HOST_ES_SELECTOR, cpu_read_es() & 0xf8);
  cpu_vmwrite(HOST_FS_SELECTOR, cpu_read_fs() & 0xf8);
  cpu_vmwrite(HOST_GS_SELECTOR, cpu_read_gs() & 0xf8);
  cpu_vmwrite(HOST_TR_SELECTOR, cpu_read_tr() & 0xf8);

  gdt_lm_ptr_t gdt_ptr, idt_ptr; // TODO: create idt_ptr_t?
  cpu_read_gdt((uint8_t*) &gdt_ptr);
  cpu_read_idt((uint8_t*) &idt_ptr);
  cpu_vmwrite(HOST_FS_BASE, cpu_get_seg_desc_base(gdt_ptr.base, cpu_read_fs()));
  cpu_vmwrite(HOST_GS_BASE, cpu_get_seg_desc_base(gdt_ptr.base, cpu_read_gs()));
  cpu_vmwrite(HOST_TR_BASE, cpu_get_seg_desc_base(gdt_ptr.base, cpu_read_tr()));
  cpu_vmwrite(HOST_GDTR_BASE, gdt_ptr.base);
  cpu_vmwrite(HOST_IDTR_BASE, idt_ptr.base);

  cpu_vmwrite(HOST_IA32_SYSENTER_CS, msr_read32(MSR_ADDRESS_IA32_SYSENTER_CS));
  cpu_vmwrite(HOST_IA32_SYSENTER_ESP, msr_read32(MSR_ADDRESS_IA32_SYSENTER_ESP));
  cpu_vmwrite(HOST_IA32_SYSENTER_EIP, msr_read32(MSR_ADDRESS_IA32_SYSENTER_EIP));
  cpu_vmwrite(HOST_IA32_EFER, msr_read32(MSR_ADDRESS_IA32_EFER)),
  cpu_vmwrite(HOST_IA32_EFER_HIGH, msr_read64(MSR_ADDRESS_IA32_EFER) >> 32);
  // HOST_IA32_PERF_GLOBAL_CTRL{,_HIGH}, HOST_IA32_PAT{,_HIGH},
}

void vmm_vmcs_fill_vm_exec_control_fields(void) {
  /**
   * 24.6: VM-Execution Control Fields.
   */
  // 24.6.1: Pin-Based VM-Execution Controls
  uint32_t pinbased_ctls = 0;
  cpu_vmwrite(PIN_BASED_VM_EXEC_CONTROL, cpu_adjust32(pinbased_ctls, MSR_ADDRESS_IA32_VMX_PINBASED_CTLS));

  // 24.6.2: Processor-Based VM-Execution Controls
  uint32_t procbased_ctls = ACT_SECONDARY_CONTROLS | USE_MSR_BITMAPS;
  procbased_ctls = cpu_adjust32(procbased_ctls, MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS);
  procbased_ctls &= ~(CR3_LOAD_EXITING | CR3_STORE_EXITING);
  cpu_vmwrite(CPU_BASED_VM_EXEC_CONTROL, procbased_ctls);

  uint32_t procbased_ctls_2 = ENABLE_EPT | UNRESTRICTED_GUEST;
  cpu_vmwrite(SECONDARY_VM_EXEC_CONTROL, cpu_adjust32(procbased_ctls_2, MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS2));

  // 24.6.3: Exception Bitmap
  cpu_vmwrite(EXCEPTION_BITMAP, 0);
  cpu_vmwrite(PAGE_FAULT_ERROR_CODE_MASK, 0);
  cpu_vmwrite(PAGE_FAULT_ERROR_CODE_MATCH, 0);

  // 24.6.4: I/O-Bitmap Addresses
  memset(&io_bitmap_a[0], 0, 0x1000);
  memset(&io_bitmap_b[0], 0, 0x1000);
  //io_bitmap_a[0x70 / 8] = io_bitmap_a[0x70 / 8] | (1 << (0x70 % 8));
  //io_bitmap_a[0x71 / 8] = io_bitmap_a[0x71 / 8] | (1 << (0x71 % 8));
  // 0x20 = PORT_PIC1_CMD, used at the end of the POST, when the IVT has already been setup
  io_bitmap_a[0x20 / 8] = io_bitmap_a[0x20 / 8] | (1 << (0x20 % 8));
  cpu_vmwrite(IO_BITMAP_A, (uint32_t) (((uint64_t) &io_bitmap_a[0]) & 0xffffffff));
  cpu_vmwrite(IO_BITMAP_A_HIGH, (uint32_t) ((((uint64_t) &io_bitmap_a[0]) >> 32) & 0xffffffff));
  cpu_vmwrite(IO_BITMAP_B, (uint32_t) (((uint64_t) &io_bitmap_b[0]) & 0xffffffff));
  cpu_vmwrite(IO_BITMAP_B_HIGH, (uint32_t) ((((uint64_t) &io_bitmap_b[0]) >> 32) & 0xffffffff));
  //cpu_vmwrite(IO_BITMAP_A, 0);
  //cpu_vmwrite(IO_BITMAP_A_HIGH, 0);
  //cpu_vmwrite(IO_BITMAP_B, 0);
  //cpu_vmwrite(IO_BITMAP_B_HIGH, 0);

  // 24.6.5: Time-Stamp Counter Offset
  cpu_vmwrite(TSC_OFFSET, 0);
  cpu_vmwrite(TSC_OFFSET_HIGH, 0);

  // 24.6.6: Guest/Host Masks and Read Shadows for CR0 and CR4
  cpu_vmwrite(CR0_GUEST_HOST_MASK, 0);
  cpu_vmwrite(CR0_READ_SHADOW, 0);
  cpu_vmwrite(CR4_GUEST_HOST_MASK, 0);
  cpu_vmwrite(CR4_READ_SHADOW, 0);

  // 24.6.7: CR3-Target Controls
  cpu_vmwrite(CR3_TARGET_COUNT, 0);
  cpu_vmwrite(CR3_TARGET_VALUE0, 0);
  cpu_vmwrite(CR3_TARGET_VALUE1, 0);
  cpu_vmwrite(CR3_TARGET_VALUE2, 0);
  cpu_vmwrite(CR3_TARGET_VALUE3, 0);

  // 24.6.8: Controls for APIC Virtualization (optional, unused)
  // APIC_ACCESS_ADDR{,_HIGH}, VIRTUAL_APIC_PAGE_ADDR{,_HIGH}, TPR_THRESHOLD,
  // EOI_EXIT_BITMAP[0-4]{,_HIGH}, POSTED_INTR_NOTIF_VECTOR,
  // POSTED_INTR_DESC_ADDRESS{,_HIGH} unused

  // 24.6.9: MSR-Bitmap Address
  memset(&msr_bitmaps, 0, sizeof(msr_bitmaps));
  cpu_vmwrite(MSR_BITMAP, (uint64_t) &msr_bitmaps & 0xFFFFFFFF);
  cpu_vmwrite(MSR_BITMAP_HIGH, (uint64_t) &msr_bitmaps >> 32);

  // 24.6.10: Executive-VMCS Pointer (optional, unused)
  // EXEC_VMCS_PTR{,_HIGH} unused

  // 24.6.11: Extended-Page-Table Pointer (EPTP)
  uint64_t eptp = VMEM_ADDR_VIRTUAL_TO_PHYSICAL(&vmm_info->ept_info.PML4[0]) | (3 << 3) /* Page walk length - 1 */;
  cpu_vmwrite(EPT_POINTER, eptp & 0xFFFFFFFF);
  cpu_vmwrite(EPT_POINTER_HIGH, (eptp & 0xFFFFFFFF) >> 32);

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
  uint32_t exit_controls = SAVE_IA32_EFER | LOAD_IA32_EFER | HOST_ADDR_SPACE_SIZE /* x86_64 host */;
  cpu_vmwrite(VM_EXIT_CONTROLS, cpu_adjust32(exit_controls, MSR_ADDRESS_IA32_VMX_EXIT_CTLS));

  // 24.7.2: VM-Exit Controls for MSRs
  cpu_vmwrite(VM_EXIT_MSR_STORE_COUNT, 0);
  // VM_EXIT_MSR_STORE_ADDR{,_HIGH} unused
  cpu_vmwrite(VM_EXIT_MSR_LOAD_COUNT, 0);
  // VM_EXIT_MSR_LOAD_ADDR{,_HIGH} unused
}

void vmm_vmcs_fill_vm_entry_control_fields(void) {
  /**
   * 24.8: VM-Entry Control fields.
   */
  // 24.8.1: VM-Entry Controls
  uint32_t entry_controls = ENTRY_LOAD_IA32_EFER;
  cpu_vmwrite(VM_ENTRY_CONTROLS, cpu_adjust32(entry_controls, MSR_ADDRESS_IA32_VMX_ENTRY_CTLS));

  // 24.8.2: VM-Entry Controls for MSRs.
  cpu_vmwrite(VM_ENTRY_MSR_LOAD_COUNT, 0);
  // VM_ENTRY_MSR_LOAD_ADDR{,_HIGH} unused

  // 24.8.3: VM-Entry Controls for Event Injection
  cpu_vmwrite(VM_ENTRY_INTR_INFO_FIELD, 0);
  // VM_ENTRY_EXCEPTION_ERROR_CODE and VM_ENTRY_INSTRUCTION_LEN unused
}
