#include "vmm.h"
#include "gdt.h"
#include "idt.h"
#include "mtrr.h"
#include "ept.h"
#include "vmcs.h"
#include "msr_bitmap.h"
#include "io_bitmap.h"
#include "paging.h"

#include "string.h"
#include "stdio.h"

#include "cpu.h"
#include "msr.h"

void vmcs_fill_guest_state_fields(void) {
  struct gdt_entry gdt_entry;
  struct idt_ptr idt_ptr;
  uint64_t msr;
  uint64_t sel;

  cpu_vmwrite(GUEST_CR0, cpu_adjust64(cpu_read_cr0(), MSR_ADDRESS_VMX_CR0_FIXED0, MSR_ADDRESS_VMX_CR0_FIXED1));
  cpu_vmwrite(GUEST_CR3, cpu_read_cr3());
  cpu_vmwrite(GUEST_CR4, cpu_adjust64(cpu_read_cr4(), MSR_ADDRESS_VMX_CR4_FIXED0, MSR_ADDRESS_VMX_CR4_FIXED1));
  cpu_vmwrite(GUEST_DR7, cpu_read_dr7());
  cpu_vmwrite(GUEST_RFLAGS, cpu_read_flags());

  sel = cpu_read_cs();
  gdt_get_guest_entry(sel, &gdt_entry);
  cpu_vmwrite(GUEST_CS_SELECTOR, sel);
  cpu_vmwrite(GUEST_CS_BASE, gdt_entry.base);
  cpu_vmwrite(GUEST_CS_LIMIT, gdt_entry.limit);
  cpu_vmwrite(GUEST_CS_AR_BYTES, (gdt_entry.granularity << 8) | (gdt_entry.access << 0));
  sel = cpu_read_ss();
  gdt_get_guest_entry(sel, &gdt_entry);
  cpu_vmwrite(GUEST_SS_SELECTOR, sel);
  cpu_vmwrite(GUEST_SS_BASE, gdt_entry.base);
  cpu_vmwrite(GUEST_SS_LIMIT, gdt_entry.limit);
  cpu_vmwrite(GUEST_SS_AR_BYTES, (gdt_entry.granularity << 8) | (gdt_entry.access << 0));
  sel = cpu_read_ds();
  gdt_get_guest_entry(sel, &gdt_entry);
  cpu_vmwrite(GUEST_DS_SELECTOR, sel);
  cpu_vmwrite(GUEST_DS_BASE, gdt_entry.base);
  cpu_vmwrite(GUEST_DS_LIMIT, gdt_entry.limit);
  cpu_vmwrite(GUEST_DS_AR_BYTES, (gdt_entry.granularity << 8) | (gdt_entry.access << 0));
  sel = cpu_read_es();
  gdt_get_guest_entry(sel, &gdt_entry);
  cpu_vmwrite(GUEST_ES_SELECTOR, sel);
  cpu_vmwrite(GUEST_ES_BASE, gdt_entry.base);
  cpu_vmwrite(GUEST_ES_LIMIT, gdt_entry.limit);
  cpu_vmwrite(GUEST_ES_AR_BYTES, (gdt_entry.granularity << 8) | (gdt_entry.access << 0));
  sel = cpu_read_fs();
  gdt_get_guest_entry(sel, &gdt_entry);
  cpu_vmwrite(GUEST_FS_SELECTOR, sel);
  cpu_vmwrite(GUEST_FS_BASE, gdt_entry.base);
  cpu_vmwrite(GUEST_FS_LIMIT, gdt_entry.limit);
  cpu_vmwrite(GUEST_FS_AR_BYTES, (gdt_entry.granularity << 8) | (gdt_entry.access << 0));
  sel = cpu_read_gs();
  gdt_get_guest_entry(sel, &gdt_entry);
  cpu_vmwrite(GUEST_GS_SELECTOR, sel);
  cpu_vmwrite(GUEST_GS_BASE, gdt_entry.base);
  cpu_vmwrite(GUEST_GS_LIMIT, gdt_entry.limit);
  cpu_vmwrite(GUEST_GS_AR_BYTES, (gdt_entry.granularity << 8) | (gdt_entry.access << 0));

  /* TODO Verify if these values are correct. */
  cpu_vmwrite(GUEST_LDTR_SELECTOR, 0);
  cpu_vmwrite(GUEST_LDTR_BASE, 0);
  cpu_vmwrite(GUEST_LDTR_LIMIT, 0xffff);
  cpu_vmwrite(GUEST_LDTR_AR_BYTES, 0x82);
  cpu_vmwrite(GUEST_TR_SELECTOR, 0);
  cpu_vmwrite(GUEST_TR_BASE, 0);
  cpu_vmwrite(GUEST_TR_LIMIT, 0xffff);
  cpu_vmwrite(GUEST_TR_AR_BYTES, 0x8b);

  cpu_vmwrite(GUEST_GDTR_BASE, gdt_get_guest_base());
  cpu_vmwrite(GUEST_GDTR_LIMIT, gdt_get_guest_limit());

  cpu_read_idt((uint8_t *) &idt_ptr);
  cpu_vmwrite(GUEST_IDTR_BASE, idt_ptr.base);
  cpu_vmwrite(GUEST_IDTR_LIMIT, idt_ptr.limit);

  cpu_vmwrite(GUEST_IA32_DEBUGCTL, msr_read(MSR_ADDRESS_IA32_DEBUGCTL));
  cpu_vmwrite(GUEST_IA32_DEBUGCTL_HIGH, msr_read(MSR_ADDRESS_IA32_DEBUGCTL) >> 32);
  cpu_vmwrite(GUEST_SYSENTER_CS, msr_read(MSR_ADDRESS_IA32_SYSENTER_CS));

  msr = msr_read(MSR_ADDRESS_IA32_EFER);
  cpu_vmwrite(GUEST_IA32_EFER, msr & 0xffffffff),
  cpu_vmwrite(GUEST_IA32_EFER_HIGH, (msr >> 32) & 0xffffffff);

  cpu_vmwrite(GUEST_ACTIVITY_STATE, 0);
  cpu_vmwrite(GUEST_INTERRUPTIBILITY_INFO, 0);
  cpu_vmwrite(GUEST_PENDING_DBG_EXCEPTIONS, 0);
  cpu_vmwrite(VMCS_LINK_POINTER, 0xffffffff);
  cpu_vmwrite(VMCS_LINK_POINTER_HIGH, 0xffffffff);
}

void vmcs_fill_host_state_fields(void) {
  uint64_t sel;
  struct gdt_entry gdt_entry;
  struct idt_ptr idt_ptr;

  cpu_vmwrite(HOST_CR0, cpu_adjust64(cpu_read_cr0(), MSR_ADDRESS_VMX_CR0_FIXED0, MSR_ADDRESS_VMX_CR0_FIXED1));
  cpu_vmwrite(HOST_CR3, cpu_read_cr3());
  /* TODO cpu_vmwrite(HOST_CR3, paging_get_host_cr3()); */
  cpu_vmwrite(HOST_CR4, cpu_adjust64(cpu_read_cr4(), MSR_ADDRESS_VMX_CR4_FIXED0, MSR_ADDRESS_VMX_CR4_FIXED1));
  cpu_vmwrite(HOST_RSP, (uint64_t) &vmm_stack[VMM_STACK_SIZE]);
  cpu_vmwrite(HOST_RIP, (uint64_t) vmm_vm_exit_handler);

  cpu_vmwrite(HOST_CS_SELECTOR, cpu_read_cs() & 0xf8);
  cpu_vmwrite(HOST_SS_SELECTOR, cpu_read_ss() & 0xf8);
  cpu_vmwrite(HOST_DS_SELECTOR, cpu_read_ds() & 0xf8);
  cpu_vmwrite(HOST_ES_SELECTOR, cpu_read_es() & 0xf8);
  cpu_vmwrite(HOST_FS_SELECTOR, cpu_read_fs() & 0xf8);
  cpu_vmwrite(HOST_GS_SELECTOR, cpu_read_gs() & 0xf8);
  cpu_vmwrite(HOST_TR_SELECTOR, 0x8 & 0xf8);

  sel = cpu_read_fs();
  gdt_get_host_entry(sel, &gdt_entry);
  cpu_vmwrite(HOST_FS_BASE, gdt_entry.base);
  sel = cpu_read_gs();
  gdt_get_host_entry(sel, &gdt_entry);
  cpu_vmwrite(HOST_GS_BASE, gdt_entry.base);
  sel = 0x8;
  gdt_get_host_entry(sel, &gdt_entry);
  cpu_vmwrite(HOST_TR_BASE, gdt_entry.base);

  cpu_vmwrite(HOST_GDTR_BASE, gdt_get_host_base());

  cpu_read_idt((uint8_t *) &idt_ptr);
  cpu_vmwrite(HOST_IDTR_BASE, idt_ptr.base);

  cpu_vmwrite(HOST_IA32_SYSENTER_CS, msr_read(MSR_ADDRESS_IA32_SYSENTER_CS));
  cpu_vmwrite(HOST_IA32_SYSENTER_ESP, msr_read(MSR_ADDRESS_IA32_SYSENTER_ESP));
  cpu_vmwrite(HOST_IA32_SYSENTER_EIP, msr_read(MSR_ADDRESS_IA32_SYSENTER_EIP));
  cpu_vmwrite(HOST_IA32_EFER, msr_read(MSR_ADDRESS_IA32_EFER) & 0xffffffff),
  cpu_vmwrite(HOST_IA32_EFER_HIGH, (msr_read(MSR_ADDRESS_IA32_EFER) >> 32) & 0xffffffff);
}

void vmcs_fill_vm_exec_control_fields(void) {
  uint32_t procbased_ctls = ACT_SECONDARY_CONTROLS | USE_MSR_BITMAPS | USE_IO_BITMAPS /*| MONITOR_TRAP_FLAG*/;
  uint32_t procbased_ctls_2 = ENABLE_EPT | ENABLE_VPID | UNRESTRICTED_GUEST;
  uint64_t msr_bitmap_ptr;
  uint64_t eptp;
  uint64_t io_bitmap_ptr;

  cpu_vmwrite(PIN_BASED_VM_EXEC_CONTROL, (1 << 3) | cpu_adjust32(0, MSR_ADDRESS_IA32_VMX_PINBASED_CTLS));
  procbased_ctls = cpu_adjust32(procbased_ctls, MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS);
  procbased_ctls &= ~(CR3_LOAD_EXITING | CR3_STORE_EXITING);
  cpu_vmwrite(CPU_BASED_VM_EXEC_CONTROL, procbased_ctls);

  cpu_vmwrite(SECONDARY_VM_EXEC_CONTROL, cpu_adjust32(procbased_ctls_2, MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS2));

  cpu_vmwrite(EXCEPTION_BITMAP, 0);
  cpu_vmwrite(PAGE_FAULT_ERROR_CODE_MASK, 0);
  cpu_vmwrite(PAGE_FAULT_ERROR_CODE_MATCH, 0);

  io_bitmap_ptr = io_bitmap_get_ptr_a();
  cpu_vmwrite(IO_BITMAP_A, io_bitmap_ptr & 0xffffffff);
  cpu_vmwrite(IO_BITMAP_A_HIGH, (io_bitmap_ptr >> 32) & 0xffffffff);
  io_bitmap_ptr = io_bitmap_get_ptr_b();
  cpu_vmwrite(IO_BITMAP_B, io_bitmap_ptr & 0xffffffff);
  cpu_vmwrite(IO_BITMAP_B_HIGH, (io_bitmap_ptr >> 32) & 0xffffffff);

  cpu_vmwrite(TSC_OFFSET, 0);
  cpu_vmwrite(TSC_OFFSET_HIGH, 0);

  cpu_vmwrite(CR0_GUEST_HOST_MASK, 0x20);
  cpu_vmwrite(CR0_READ_SHADOW, 0);
  cpu_vmwrite(CR4_GUEST_HOST_MASK, 0x2000);
  cpu_vmwrite(CR4_READ_SHADOW, 0);

  cpu_vmwrite(CR3_TARGET_COUNT, 0);
  cpu_vmwrite(CR3_TARGET_VALUE0, 0);
  cpu_vmwrite(CR3_TARGET_VALUE1, 0);
  cpu_vmwrite(CR3_TARGET_VALUE2, 0);
  cpu_vmwrite(CR3_TARGET_VALUE3, 0);

  msr_bitmap_ptr = msr_bitmap_get_ptr();
  cpu_vmwrite(MSR_BITMAP, msr_bitmap_ptr & 0xffffffff);
  cpu_vmwrite(MSR_BITMAP_HIGH, (msr_bitmap_ptr >> 32) & 0xffffffff);

  eptp = ept_get_eptp();
  cpu_vmwrite(EPT_POINTER, eptp & 0xffffffff);
  cpu_vmwrite(EPT_POINTER_HIGH, (eptp >> 32) & 0xffffffff);
  cpu_vmwrite(VIRTUAL_PROCESSOR_ID, 0xff);
}

void vmcs_fill_vm_exit_control_fields(void) {
  uint32_t exit_controls = EXIT_SAVE_IA32_EFER | EXIT_LOAD_IA32_EFER | HOST_ADDR_SPACE_SIZE;
  cpu_vmwrite(VM_EXIT_CONTROLS, cpu_adjust32(exit_controls, MSR_ADDRESS_IA32_VMX_EXIT_CTLS));
  cpu_vmwrite(VM_EXIT_MSR_STORE_COUNT, 0);
  cpu_vmwrite(VM_EXIT_MSR_LOAD_COUNT, 0);
}

void vmcs_fill_vm_entry_control_fields(void) {
  uint32_t entry_controls = ENTRY_LOAD_IA32_EFER | IA32E_MODE_GUEST;
  cpu_vmwrite(VM_ENTRY_CONTROLS, cpu_adjust32(entry_controls, MSR_ADDRESS_IA32_VMX_ENTRY_CTLS));
  cpu_vmwrite(VM_ENTRY_MSR_LOAD_COUNT, 0);
  cpu_vmwrite(VM_ENTRY_INTR_INFO_FIELD, 0);
}
