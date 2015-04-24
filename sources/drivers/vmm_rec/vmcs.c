#include "vmm.h"
#include "vmx.h"
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

uint8_t vmxon[4096] __attribute((aligned(0x1000)));
uint8_t vmcs0[4096] __attribute((aligned(0x1000)));

static uint16_t tsc_freq_MHz;
static uint8_t tsc_divider;

void vmcs_fill_guest_state_fields(void) {
  struct gdt_entry gdt_entry;
  struct idt_ptr idt_ptr;
  uint64_t msr;
  uint64_t sel;

  cpu_vmwrite(GUEST_CR0, cpu_adjust64(cpu_read_cr0(), MSR_ADDRESS_VMX_CR0_FIXED0, MSR_ADDRESS_VMX_CR0_FIXED1));
  cpu_vmwrite(GUEST_CR3, cpu_read_cr3());
  cpu_vmwrite(GUEST_CR4, cpu_adjust64(cpu_read_cr4(), MSR_ADDRESS_VMX_CR4_FIXED0, MSR_ADDRESS_VMX_CR4_FIXED1));
  cpu_vmwrite(GUEST_DR7, cpu_read_dr7());
  cpu_vmwrite(GUEST_RFLAGS, (1 << 1));

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

  cpu_vmwrite(GUEST_IA32_PERF_GLOBAL_CTRL, msr_read(MSR_ADDRESS_IA32_PERF_GLOBAL_CTRL) & 0xffffffff);
  cpu_vmwrite(GUEST_IA32_PERF_GLOBAL_CTRL_HIGH, msr_read(MSR_ADDRESS_IA32_PERF_GLOBAL_CTRL) >> 32);

  // Init and compute vmx_preemption_timer_value
  tsc_freq_MHz = ((msr_read(MSR_ADDRESS_MSR_PLATFORM_INFO) >> 8) & 0xff) * 100;
  tsc_divider = msr_read(MSR_ADDRESS_IA32_VMX_MISC) & 0x7;
  vmcs_set_vmx_preemption_timer_value(VMCS_DEFAULT_PREEMPTION_TIMER_MICROSEC);
}

void vmcs_fill_host_state_fields(void) {
  uint64_t sel;
  struct gdt_entry gdt_entry;
  struct idt_ptr idt_ptr;

  cpu_vmwrite(HOST_CR0, cpu_adjust64(cpu_read_cr0(), MSR_ADDRESS_VMX_CR0_FIXED0, MSR_ADDRESS_VMX_CR0_FIXED1));
  // cpu_vmwrite(HOST_CR3, cpu_read_cr3());
  cpu_vmwrite(HOST_CR3, paging_get_host_cr3());
  // TODO Bit 18 OSXSAVE Activation du XCR0 -> proxifier le XSETBV
  cpu_vmwrite(HOST_CR4, cpu_adjust64(cpu_read_cr4() | (1 << 18), MSR_ADDRESS_VMX_CR4_FIXED0, MSR_ADDRESS_VMX_CR4_FIXED1));
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

  idt_get_idt_ptr(&idt_ptr);
  cpu_vmwrite(HOST_IDTR_BASE, idt_ptr.base);

  cpu_vmwrite(HOST_IA32_SYSENTER_CS, msr_read(MSR_ADDRESS_IA32_SYSENTER_CS));
  cpu_vmwrite(HOST_IA32_SYSENTER_ESP, msr_read(MSR_ADDRESS_IA32_SYSENTER_ESP));
  cpu_vmwrite(HOST_IA32_SYSENTER_EIP, msr_read(MSR_ADDRESS_IA32_SYSENTER_EIP));
  cpu_vmwrite(HOST_IA32_EFER, msr_read(MSR_ADDRESS_IA32_EFER) & 0xffffffff),
  cpu_vmwrite(HOST_IA32_EFER_HIGH, (msr_read(MSR_ADDRESS_IA32_EFER) >> 32) & 0xffffffff);

  cpu_vmwrite(HOST_IA32_PERF_GLOBAL_CTRL, 0);
  cpu_vmwrite(HOST_IA32_PERF_GLOBAL_CTRL_HIGH, 0);
}

void vmcs_fill_vm_exec_control_fields(void) {
  uint32_t procbased_ctls = ACT_SECONDARY_CONTROLS | USE_MSR_BITMAPS | USE_IO_BITMAPS | CR3_LOAD_EXITING | USE_TSC_OFFSETTING;
  uint32_t procbased_ctls_2 = ENABLE_EPT | ENABLE_VPID | UNRESTRICTED_GUEST | ENABLE_RDTSCP/* | VIRT_INTR_DELIVERY | APIC_REGISTER_VIRT*/;
                             
  uint64_t msr_bitmap_ptr;
  uint64_t eptp;
  uint64_t io_bitmap_ptr;
  uint32_t pinbased_ctls = ACT_VMX_PREEMPT_TIMER;

  cpu_vmwrite(PIN_BASED_VM_EXEC_CONTROL, cpu_adjust32(pinbased_ctls, MSR_ADDRESS_IA32_VMX_PINBASED_CTLS));

  procbased_ctls = cpu_adjust32(procbased_ctls, MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS);
  if (((msr_read(MSR_ADDRESS_IA32_VMX_BASIC) >> 55) & 1)                       // extra capabilities enabled
  && (((msr_read(MSR_ADDRESS_IA32_VMX_TRUE_PROCBASED_CTLS) >> 15) & 3)) == 0){ // CR3_LOAD_EXITING & CR3_STORE_EXITING can be disabled
    procbased_ctls &= ~(CR3_LOAD_EXITING | CR3_STORE_EXITING);
  } else {
    panic("#!PROCBASED_CTLS CR3_LOAD_EXITING or CR3_STORE_EXITING couldn't be disabled\n");
  }
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

  // As we are using UNRESTRICTED_GUEST procbased_ctrl, the guest can itself modify CR0.PE and CR0.PG, see doc INTEL vol 3C chap 23.8
  cpu_vmwrite(CR0_GUEST_HOST_MASK, (msr_read(MSR_ADDRESS_VMX_CR0_FIXED0) | (~msr_read(MSR_ADDRESS_VMX_CR0_FIXED1))) & ~(0x80000001));
  cpu_vmwrite(CR0_READ_SHADOW, cpu_read_cr0());
  cpu_vmwrite(CR4_GUEST_HOST_MASK, msr_read(MSR_ADDRESS_VMX_CR4_FIXED0) | ~msr_read(MSR_ADDRESS_VMX_CR4_FIXED1));
  cpu_vmwrite(CR4_READ_SHADOW, cpu_read_cr4() & ~(uint64_t)(1<<13)); // We hide CR4.VMXE

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
  cpu_vmwrite(VIRTUAL_PROCESSOR_ID, 0x1); // vmcs0 is 1

  // TEST virtual APIC
  cpu_vmwrite(VIRTUAL_APIC_PAGE_ADDR, (((uintptr_t)&vapic[0]) >> 0) &
      0xffffffff);
  cpu_vmwrite(VIRTUAL_APIC_PAGE_ADDR_HIGH, (((uintptr_t)&vapic[0]) >> 32) &
      0xffffffff);
}

void vmcs_fill_vm_exit_control_fields(void) {
  uint32_t exit_controls = EXIT_SAVE_IA32_EFER | EXIT_LOAD_IA32_EFER |
    EXIT_LOAD_IA32_PERF_GLOBAL_CTRL | HOST_ADDR_SPACE_SIZE |
    SAVE_VMX_PREEMPT_TIMER_VAL;
  cpu_vmwrite(VM_EXIT_CONTROLS, cpu_adjust32(exit_controls, MSR_ADDRESS_IA32_VMX_EXIT_CTLS));
  cpu_vmwrite(VM_EXIT_MSR_STORE_COUNT, 0);
  cpu_vmwrite(VM_EXIT_MSR_LOAD_COUNT, 0);
}

void vmcs_fill_vm_entry_control_fields(void) {
  uint32_t entry_controls = ENTRY_LOAD_IA32_EFER | ENTRY_LOAD_IA32_PERF_GLOBAL_CTRL | IA32E_MODE_GUEST;
  cpu_vmwrite(VM_ENTRY_CONTROLS, cpu_adjust32(entry_controls, MSR_ADDRESS_IA32_VMX_ENTRY_CTLS));
  cpu_vmwrite(VM_ENTRY_MSR_LOAD_COUNT, 0);
  cpu_vmwrite(VM_ENTRY_INTR_INFO_FIELD, 0);
}

/* Dump a section of VMCS */
void print_section(char *header, uint32_t start, uint32_t end,
    int incr) {
  uint32_t addr, j;
  unsigned long val;
  int code, rc;
  char *fmt[4] = {"0x%04x ", "0x%016x ", "0x%08x ", "0x%016x "};
  char *err[4] = {"------ ", "------------------ ",
                  "---------- ", "------------------ "};
  /* Find width of the field (encoded in bits 14:13 of address) */
  code = (start>>13)&3;
  if (header)
    printk("\t %s", header);
  for (addr=start, j=0; addr<=end; addr+=incr, j++) {
    if (!(j&3))
      printk("\n\t\t0x%08x: ", addr);
    val = cpu_vmread_safe(addr, (long unsigned int *)&rc);
    if (val != 0)
      printk(fmt[code], rc);
    else
      printk("%s", err[code]);
  }
  printk("\n");
}

void vmcs_dump_vcpu(void)
{
  print_section("16-bit Control Fields", 0x000, 0x004, 2);
  print_section("16-bit Guest-State Fields", 0x800, 0x810, 2);
  print_section("16-bit Host-State Fields", 0xc00, 0xc0c, 2);
  print_section("64-bit Control Fields", 0x2000, 0x202d, 1);
  print_section("64-bit RO Data Fields", 0x2400, 0x2401, 1);
  print_section("64-bit Guest-State Fields", 0x2800, 0x2811, 1);
  print_section("64-bit Host-State Fields", 0x2c00, 0x2c05, 1);
  print_section("32-bit Control Fields", 0x4000, 0x4022, 2);
  print_section("32-bit RO Data Fields", 0x4400, 0x440e, 2);
  print_section("32-bit Guest-State Fields", 0x4800, 0x482e, 2);
  print_section("32-bit Host-State Fields", 0x4c00, 0x4c00, 2);
  print_section("Natural 64-bit Control Fields", 0x6000, 0x600e, 2);
  print_section("Natural RO Data Fields", 0x6400, 0x640a, 2);
  print_section("Natural 64-bit Guest-State Fields", 0x6800, 0x6826, 2);
  print_section("Natural 64-bit Host-State Fields", 0x6c00, 0x6c16, 2);
}

void vmcs_set_vmx_preemption_timer_value(uint64_t time_microsec) {
  cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE, (tsc_freq_MHz * time_microsec) >> tsc_divider);
}
