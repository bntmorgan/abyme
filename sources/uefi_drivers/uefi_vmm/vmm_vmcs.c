#include "vmm.h"
#include "vmm_setup.h"
#include "vmem.h"
#include "mtrr.h"

#include "string.h"
#include "stdio.h"

#include "hardware/cpu.h"
#include "hardware/msr.h"

uint8_t io_bitmap_a[0x1000] __attribute__((aligned(0x1000)));
uint8_t io_bitmap_b[0x1000] __attribute__((aligned(0x1000)));

struct {
  uint8_t low_msrs_read_bitmap[0x400];
  uint8_t high_msrs_read_bitmap[0x400];
  uint8_t low_msrs_write_bitmap[0x400];
  uint8_t high_msrs_write_bitmap[0x400];
} __attribute__((packed)) msr_bitmaps __attribute__((aligned(0x1000)));
extern void *vm_entrypoint;

void vmm_vmcs_fill_guest_state_fields(void) {
  uint64_t cr0 = cpu_adjust64(cpu_read_cr0(), MSR_ADDRESS_VMX_CR0_FIXED0, MSR_ADDRESS_VMX_CR0_FIXED1);
  INFO("CR0 : %X\n", cr0);
  cpu_vmwrite(GUEST_CR0, cr0);

  uint64_t cr3 = cpu_read_cr3();
  INFO("CR3 : %X\n", cr3);
  cpu_vmwrite(GUEST_CR3, cr3);

  uint64_t cr4 = cpu_adjust64(cpu_read_cr4(), MSR_ADDRESS_VMX_CR4_FIXED0, MSR_ADDRESS_VMX_CR4_FIXED1);
  INFO("CR4 : %X\n", cr4);
  cpu_vmwrite(GUEST_CR4, cr4);

  uint64_t dr7 = cpu_read_dr7();
  INFO("DR7 : %X\n", dr7);
  cpu_vmwrite(GUEST_DR7, dr7);

  uint64_t rflags = cpu_read_flags();
  INFO("RFLAGS : %X\n", rflags);
  cpu_vmwrite(GUEST_RFLAGS, rflags);

  gdt_entry_t entry;
  uint64_t sel;
  sel = cpu_read_cs();
  VMEM_GET_SAVED_GDT_ENTRY(sel, &entry);
  INFO("CS Selector %x\n", sel);
  VMEM_PRINT_GDT_ENTRY(&entry);
  cpu_vmwrite(GUEST_CS_SELECTOR, sel);
  cpu_vmwrite(GUEST_CS_BASE, entry.base);
  cpu_vmwrite(GUEST_CS_LIMIT, entry.limit);
  cpu_vmwrite(GUEST_CS_AR_BYTES, (entry.granularity << 8) | (entry.access << 0));

  sel = cpu_read_ss();
  VMEM_GET_SAVED_GDT_ENTRY(sel, &entry);
  INFO("SS Selector %x\n", sel);
  VMEM_PRINT_GDT_ENTRY(&entry);
  cpu_vmwrite(GUEST_SS_SELECTOR, sel);
  cpu_vmwrite(GUEST_SS_BASE, entry.base);
  cpu_vmwrite(GUEST_SS_LIMIT, entry.limit);
  cpu_vmwrite(GUEST_SS_AR_BYTES, (entry.granularity << 8) | (entry.access << 0));

  sel = cpu_read_ds();
  VMEM_GET_SAVED_GDT_ENTRY(sel, &entry);
  INFO("DS Selector %x\n", sel);
  VMEM_PRINT_GDT_ENTRY(&entry);
  cpu_vmwrite(GUEST_DS_SELECTOR, sel);
  cpu_vmwrite(GUEST_DS_BASE, entry.base);
  cpu_vmwrite(GUEST_DS_LIMIT, entry.limit);
  cpu_vmwrite(GUEST_DS_AR_BYTES, (entry.granularity << 8) | (entry.access << 0));

  sel = cpu_read_es();
  VMEM_GET_SAVED_GDT_ENTRY(sel, &entry);
  INFO("ES Selector %x\n", sel);
  VMEM_PRINT_GDT_ENTRY(&entry);
  cpu_vmwrite(GUEST_ES_SELECTOR, sel);
  cpu_vmwrite(GUEST_ES_BASE, entry.base);
  cpu_vmwrite(GUEST_ES_LIMIT, entry.limit);
  cpu_vmwrite(GUEST_ES_AR_BYTES, (entry.granularity << 8) | (entry.access << 0));

  sel = cpu_read_fs();
  VMEM_GET_SAVED_GDT_ENTRY(sel, &entry);
  INFO("FS Selector %x\n", sel);
  VMEM_PRINT_GDT_ENTRY(&entry);
  cpu_vmwrite(GUEST_FS_SELECTOR, sel);
  cpu_vmwrite(GUEST_FS_BASE, entry.base);
  cpu_vmwrite(GUEST_FS_LIMIT, entry.limit);
  cpu_vmwrite(GUEST_FS_AR_BYTES, (entry.granularity << 8) | (entry.access << 0));

  sel = cpu_read_gs();
  VMEM_GET_SAVED_GDT_ENTRY(sel, &entry);
  INFO("GS Selector %x\n", sel);
  VMEM_PRINT_GDT_ENTRY(&entry);
  cpu_vmwrite(GUEST_GS_SELECTOR, sel);
  cpu_vmwrite(GUEST_GS_BASE, entry.base);
  cpu_vmwrite(GUEST_GS_LIMIT, entry.limit);
  cpu_vmwrite(GUEST_GS_AR_BYTES, (entry.granularity << 8) | (entry.access << 0));

  sel = cpu_read_ldtr();
  VMEM_GET_SAVED_GDT_ENTRY(sel, &entry);
  INFO("LDTR Selector %x\n", sel);
  VMEM_PRINT_GDT_ENTRY(&entry);
  cpu_vmwrite(GUEST_LDTR_SELECTOR, 0);
  cpu_vmwrite(GUEST_LDTR_BASE, 0);
  cpu_vmwrite(GUEST_LDTR_LIMIT, 0xFFFF);
  cpu_vmwrite(GUEST_LDTR_AR_BYTES, 0x82 /* Present, R/W */);

  sel = cpu_read_tr();
  VMEM_GET_SAVED_GDT_ENTRY(sel, &entry);
  INFO("TR Selector %x\n", sel);
  VMEM_PRINT_GDT_ENTRY(&entry);
  cpu_vmwrite(GUEST_TR_SELECTOR, 0);
  cpu_vmwrite(GUEST_TR_BASE, 0);
  cpu_vmwrite(GUEST_TR_LIMIT, 0xFFFF);
  cpu_vmwrite(GUEST_TR_AR_BYTES, 0x8b /* Present, 16-bit busy TSS */);

  INFO("GDTR base %x\n", gdt_ptr.base);
  cpu_vmwrite(GUEST_GDTR_BASE, gdt_ptr.base);
  cpu_vmwrite(GUEST_GDTR_LIMIT, gdt_ptr.limit);

  INFO("IDTR base %x\n", idt_ptr.base);
  cpu_vmwrite(GUEST_IDTR_BASE, idt_ptr.base);
  cpu_vmwrite(GUEST_IDTR_LIMIT, idt_ptr.limit);

  cpu_vmwrite(GUEST_IA32_DEBUGCTL, msr_read(MSR_ADDRESS_IA32_DEBUGCTL));
  cpu_vmwrite(GUEST_IA32_DEBUGCTL_HIGH, msr_read(MSR_ADDRESS_IA32_DEBUGCTL) >> 32);
  cpu_vmwrite(GUEST_SYSENTER_CS, msr_read(MSR_ADDRESS_IA32_SYSENTER_CS));
  uint32_t msr;
  msr = msr_read(MSR_ADDRESS_IA32_SYSENTER_ESP);
  cpu_vmwrite(GUEST_SYSENTER_ESP, msr);
  INFO("GUEST_SYSENTER_ESP : %x\n", msr);
  msr = msr_read(MSR_ADDRESS_IA32_SYSENTER_ESP);
  cpu_vmwrite(GUEST_SYSENTER_EIP, msr);
  INFO("GUEST_SYSENTER_EIP : %x\n", msr);
  uint64_t msr64;

  msr64 = msr_read(MSR_ADDRESS_IA32_EFER);
  cpu_vmwrite(GUEST_IA32_EFER, msr64 & 0xFFFFFFFF),
  cpu_vmwrite(GUEST_IA32_EFER_HIGH, (msr64 >> 32) & 0xFFFFFFFF);
  INFO("IA32_EFER %X\n", msr64);

  cpu_vmwrite(GUEST_ACTIVITY_STATE, 0);
  cpu_vmwrite(GUEST_INTERRUPTIBILITY_INFO, 0);
  cpu_vmwrite(GUEST_PENDING_DBG_EXCEPTIONS, 0);
  cpu_vmwrite(VMCS_LINK_POINTER, 0xFFFFFFFF);
  cpu_vmwrite(VMCS_LINK_POINTER_HIGH, 0xFFFFFFFF);
}

void vmm_vmcs_fill_host_state_fields(void) {
  uint64_t sel;
  uint64_t cr0 = cpu_adjust64(cpu_read_cr0(), MSR_ADDRESS_VMX_CR0_FIXED0, MSR_ADDRESS_VMX_CR0_FIXED1);
  uint64_t cr4 = cpu_adjust64(cpu_read_cr4(), MSR_ADDRESS_VMX_CR4_FIXED0, MSR_ADDRESS_VMX_CR4_FIXED1);
  cpu_vmwrite(HOST_CR0, cr0);
  cpu_vmwrite(HOST_CR3, cpu_read_cr3());
  cpu_vmwrite(HOST_CR4, cr4);
  cpu_vmwrite(HOST_RSP, (uint64_t) vmm_stack + VMM_STACK_SIZE);
  cpu_vmwrite(HOST_RIP, (uint64_t) vmm_vm_exit_handler);

  sel = cpu_read_cs();
  INFO("Selector %x\n", sel);
  cpu_vmwrite(HOST_CS_SELECTOR, sel & 0xf8);
  sel = cpu_read_ss();
  INFO("Selector %x\n", sel);
  cpu_vmwrite(HOST_SS_SELECTOR, sel & 0xf8);
  sel = cpu_read_ds();
  INFO("Selector %x\n", sel);
  cpu_vmwrite(HOST_DS_SELECTOR, sel & 0xf8);
  sel = cpu_read_es();
  INFO("Selector %x\n", sel);
  cpu_vmwrite(HOST_ES_SELECTOR, sel & 0xf8);
  sel = cpu_read_fs();
  INFO("Selector %x\n", sel);
  cpu_vmwrite(HOST_FS_SELECTOR, sel & 0xf8);
  sel = cpu_read_gs();
  INFO("Selector %x\n", sel);
  cpu_vmwrite(HOST_GS_SELECTOR, sel & 0xf8);
  sel = 0x8;
  INFO("Selector %x\n", sel);
  cpu_vmwrite(HOST_TR_SELECTOR, sel & 0xf8);

  gdt_entry_t entry;
  sel = cpu_read_fs();
  VMEM_GET_SAVED_GDT_ENTRY(sel, &entry);
  VMEM_PRINT_GDT_ENTRY(&entry);
  cpu_vmwrite(HOST_FS_BASE, entry.base);
  sel = cpu_read_gs();
  VMEM_GET_SAVED_GDT_ENTRY(sel, &entry);
  VMEM_PRINT_GDT_ENTRY(&entry);
  cpu_vmwrite(HOST_GS_BASE, entry.base);
  sel = 0x8;
  VMEM_GET_SAVED_GDT_ENTRY(sel, &entry);
  VMEM_PRINT_GDT_ENTRY(&entry);
  cpu_vmwrite(HOST_TR_BASE, entry.base);

  cpu_vmwrite(HOST_GDTR_BASE, saved_gdt_ptr.base);
  cpu_vmwrite(HOST_IDTR_BASE, idt_ptr.base);
  cpu_vmwrite(HOST_IA32_SYSENTER_CS, msr_read(MSR_ADDRESS_IA32_SYSENTER_CS));
  uint32_t msr;
  msr = msr_read(MSR_ADDRESS_IA32_SYSENTER_ESP);
  cpu_vmwrite(HOST_IA32_SYSENTER_ESP, msr);
  INFO("HOST_IA32_SYSENTER_ESP : %x\n", msr);
  msr = msr_read(MSR_ADDRESS_IA32_SYSENTER_ESP);
  cpu_vmwrite(HOST_IA32_SYSENTER_EIP, msr);
  INFO("HOST_IA32_SYSENTER_EIP : %x\n", msr);
  cpu_vmwrite(HOST_IA32_EFER, msr_read(MSR_ADDRESS_IA32_EFER)),
  cpu_vmwrite(HOST_IA32_EFER_HIGH, msr_read(MSR_ADDRESS_IA32_EFER) >> 32);
}

void vmm_vmcs_fill_vm_exec_control_fields(void) {
  uint32_t pinbased_ctls = 0 /* ACT_VMX_PREEMPT_TIMER */;
  cpu_vmwrite(PIN_BASED_VM_EXEC_CONTROL, cpu_adjust32(pinbased_ctls, MSR_ADDRESS_IA32_VMX_PINBASED_CTLS));
  uint32_t procbased_ctls = ACT_SECONDARY_CONTROLS | USE_MSR_BITMAPS /*| USE_IO_BITMAPS | MONITOR_TRAP_FLAG*/; // Monitor trap flag : Debug porpose
  procbased_ctls = cpu_adjust32(procbased_ctls, MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS);
  procbased_ctls &= ~(CR3_LOAD_EXITING | CR3_STORE_EXITING);
  INFO("Bit 55 of IA32_VMX_BASIC allowing bits default1 bits procbase exec control 1, 4-6, 8, 13-16, and 26 to be zero if 1 : %d\n", !((msr_read(MSR_ADDRESS_IA32_VMX_BASIC) & ((uint64_t)0x1 << 55)) == 0));
  cpu_vmwrite(CPU_BASED_VM_EXEC_CONTROL, procbased_ctls);

  uint32_t procbased_ctls_2 = ENABLE_EPT | ENABLE_VPID | UNRESTRICTED_GUEST;
  cpu_vmwrite(SECONDARY_VM_EXEC_CONTROL, cpu_adjust32(procbased_ctls_2, MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS2));

  cpu_vmwrite(EXCEPTION_BITMAP, 0);
  cpu_vmwrite(PAGE_FAULT_ERROR_CODE_MASK, 0);
  cpu_vmwrite(PAGE_FAULT_ERROR_CODE_MATCH, 0);

  memset(&io_bitmap_a[0], 0, 0x1000);
  memset(&io_bitmap_b[0], 0, 0x1000);
  //io_bitmap_a[0x70 / 8] = io_bitmap_a[0x70 / 8] | (1 << (0x70 % 8));
  //io_bitmap_a[0x71 / 8] = io_bitmap_a[0x71 / 8] | (1 << (0x71 % 8));
  /*cpu_vmwrite(IO_BITMAP_A, (uint32_t) (((uint64_t) &io_bitmap_a[0]) & 0xffffffff));
  cpu_vmwrite(IO_BITMAP_A_HIGH, (uint32_t) ((((uint64_t) &io_bitmap_a[0]) >> 32) & 0xffffffff));
  cpu_vmwrite(IO_BITMAP_B, (uint32_t) (((uint64_t) &io_bitmap_b[0]) & 0xffffffff));
  cpu_vmwrite(IO_BITMAP_B_HIGH, (uint32_t) ((((uint64_t) &io_bitmap_b[0]) >> 32) & 0xffffffff));*/

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

  memset(&msr_bitmaps, 0, sizeof(msr_bitmaps));
  uint64_t msr_base = MSR_ADDRESS_IA32_MTRR_PHYBASE0;
  uint64_t i, count = mtrr_get_nb_variable_mtrr();
  for (i = 0; i < count; i++) {
    msr_bitmaps.low_msrs_read_bitmap[msr_base / 8] |= (1 << (msr_base % 8));
    msr_bitmaps.low_msrs_write_bitmap[msr_base / 8] |= (1 << (msr_base % 8));
    msr_bitmaps.low_msrs_read_bitmap[(msr_base + 1) / 8] |= (1 << ((msr_base + 1) % 8));
    msr_bitmaps.low_msrs_write_bitmap[(msr_base + 1) / 8] |= (1 << ((msr_base + 1) % 8));
    msr_base += 2;
  }
  
  msr_bitmaps.low_msrs_read_bitmap[MSR_ADDRESS_IA32_MTRRCAP / 8] |= (1 << (                    0x0fe %8));
  msr_bitmaps.low_msrs_read_bitmap[MSR_ADDRESS_A32_MTRR_DEF_TYPE / 8] |= (1 << (               0x2ff %8));
  msr_bitmaps.low_msrs_read_bitmap[MSR_ADDRESS_IA32_MTRR_FIX64K_00000 / 8] |= (1 << (          0x250 %8));
  msr_bitmaps.low_msrs_read_bitmap[MSR_ADDRESS_IA32_MTRR_FIX16K_80000 / 8] |= (1 << (          0x258 %8));
  msr_bitmaps.low_msrs_read_bitmap[MSR_ADDRESS_IA32_MTRR_FIX16K_A0000  / 8] |= (1 << (         0x259 %8));
  msr_bitmaps.low_msrs_read_bitmap[MSR_ADDRESS_IA32_MTRR_FIX4K_C0000 / 8] |= (1 << (           0x268 %8));
  msr_bitmaps.low_msrs_read_bitmap[MSR_ADDRESS_IA32_MTRR_FIX4K_C8000 / 8] |= (1 << (           0x269 %8));
  msr_bitmaps.low_msrs_read_bitmap[MSR_ADDRESS_IA32_MTRR_FIX4K_D0000 / 8] |= (1 << (           0x26a %8));
  msr_bitmaps.low_msrs_read_bitmap[MSR_ADDRESS_IA32_MTRR_FIX4K_D8000  / 8] |= (1 << (          0x26b %8));
  msr_bitmaps.low_msrs_read_bitmap[MSR_ADDRESS_IA32_MTRR_FIX4K_E0000 / 8] |= (1 << (           0x26c %8));
  msr_bitmaps.low_msrs_read_bitmap[MSR_ADDRESS_IA32_MTRR_FIX4K_E8000 / 8] |= (1 << (           0x26d %8));
  msr_bitmaps.low_msrs_read_bitmap[MSR_ADDRESS_IA32_MTRR_FIX4K_F0000 / 8] |= (1 << (           0x26e %8));
  msr_bitmaps.low_msrs_read_bitmap[MSR_ADDRESS_IA32_MTRR_FIX4K_F8000 / 8] |= (1 << (           0x26f %8));
    
  msr_bitmaps.low_msrs_write_bitmap[MSR_ADDRESS_IA32_MTRRCAP / 8] |= (1 << (                    0x0fe %8));
  msr_bitmaps.low_msrs_write_bitmap[MSR_ADDRESS_A32_MTRR_DEF_TYPE / 8] |= (1 << (               0x2ff %8));
  msr_bitmaps.low_msrs_write_bitmap[MSR_ADDRESS_IA32_MTRR_FIX64K_00000 / 8] |= (1 << (          0x250 %8));
  msr_bitmaps.low_msrs_write_bitmap[MSR_ADDRESS_IA32_MTRR_FIX16K_80000 / 8] |= (1 << (          0x258 %8));
  msr_bitmaps.low_msrs_write_bitmap[MSR_ADDRESS_IA32_MTRR_FIX16K_A0000  / 8] |= (1 << (         0x259 %8));
  msr_bitmaps.low_msrs_write_bitmap[MSR_ADDRESS_IA32_MTRR_FIX4K_C0000 / 8] |= (1 << (           0x268 %8));
  msr_bitmaps.low_msrs_write_bitmap[MSR_ADDRESS_IA32_MTRR_FIX4K_C8000 / 8] |= (1 << (           0x269 %8));
  msr_bitmaps.low_msrs_write_bitmap[MSR_ADDRESS_IA32_MTRR_FIX4K_D0000 / 8] |= (1 << (           0x26a %8));
  msr_bitmaps.low_msrs_write_bitmap[MSR_ADDRESS_IA32_MTRR_FIX4K_D8000  / 8] |= (1 << (          0x26b %8));
  msr_bitmaps.low_msrs_write_bitmap[MSR_ADDRESS_IA32_MTRR_FIX4K_E0000 / 8] |= (1 << (           0x26c %8));
  msr_bitmaps.low_msrs_write_bitmap[MSR_ADDRESS_IA32_MTRR_FIX4K_E8000 / 8] |= (1 << (           0x26d %8));
  msr_bitmaps.low_msrs_write_bitmap[MSR_ADDRESS_IA32_MTRR_FIX4K_F0000 / 8] |= (1 << (           0x26e %8));
  msr_bitmaps.low_msrs_write_bitmap[MSR_ADDRESS_IA32_MTRR_FIX4K_F8000 / 8] |= (1 << (           0x26f %8));

  cpu_vmwrite(MSR_BITMAP, (uint64_t) &msr_bitmaps & 0xFFFFFFFF);
  cpu_vmwrite(MSR_BITMAP_HIGH, ((uint64_t) &msr_bitmaps >> 32) & 0xFFFFFFFF);

  uint64_t eptp = VMEM_ADDR_VIRTUAL_TO_PHYSICAL(&ept_info.PML4[0]);
  eptp |= (3 << 3) /* Page walk length - 1 */;
  eptp |= (0x6 << 0) /* Page walk length - 1 */;
  eptp &= ~(1 << 6) /* Desactivate accessed and dirty flag  */;
  eptp &= ~(0x1f << 7) /* bit 11:7 must be set to zero */;
  cpu_vmwrite(EPT_POINTER, eptp & 0xFFFFFFFF);
  cpu_vmwrite(EPT_POINTER_HIGH, (eptp >> 32) & 0xFFFFFFFF);
  INFO("Eptp : high : %x, low : %x\n", (eptp >> 32) & 0xFFFFFFFF, eptp & 0xFFFFFFFF);
  cpu_vmwrite(VIRTUAL_PROCESSOR_ID, 0xff);
}

void vmm_vmcs_fill_vm_exit_control_fields(void) {
  uint32_t exit_controls = SAVE_IA32_EFER | LOAD_IA32_EFER | HOST_ADDR_SPACE_SIZE /* x86_64 host */;
  cpu_vmwrite(VM_EXIT_CONTROLS, cpu_adjust32(exit_controls, MSR_ADDRESS_IA32_VMX_EXIT_CTLS));
  cpu_vmwrite(VM_EXIT_MSR_STORE_COUNT, 0);
  cpu_vmwrite(VM_EXIT_MSR_LOAD_COUNT, 0);
}

void vmm_vmcs_fill_vm_entry_control_fields(void) {
  uint32_t entry_controls = ENTRY_LOAD_IA32_EFER | IA32E_MODE_GUEST;
  cpu_vmwrite(VM_ENTRY_CONTROLS, cpu_adjust32(entry_controls, MSR_ADDRESS_IA32_VMX_ENTRY_CTLS));
  cpu_vmwrite(VM_ENTRY_MSR_LOAD_COUNT, 0);
  cpu_vmwrite(VM_ENTRY_INTR_INFO_FIELD, 0);
}
