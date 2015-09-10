#include "vmm.h"
#include "vmx.h"
#include "setup.h"
#include "vmcs.h"
#include "gdt.h"
#include "mtrr.h"
#include "ept.h"
#include "pci.h"
#include "debug.h"
#include "paging.h"
#include "pat.h"
#include "dmar.h"
#include "msr.h"
#include "cpu.h"
#include "cpuid.h"
#include "stdio.h"
#include "string.h"
#include "msr_bitmap.h"
#include "io_bitmap.h"
#include "idt.h"
#include "apic.h"
#include "hook.h"
#ifdef _DEBUG_SERVER
#include "debug_server/debug_server.h"
#endif
#ifdef _VMCS_SHADOWING
#include "nested_vmx.h"
#endif

struct setup_state *setup_state;

void bsp_main(struct setup_state *state) {

  setup_state = state;

#ifdef _DEBUG_SERVER
  debug_server_init();
#endif

#ifdef _DEBUG_SERVER
  if (debug_printk) {
    INFO("Debug infos by ethernet enabled\n");
    debug_server_enable_putc();
  } else {
    INFO("Debug infos totally disabled\n");
    debug_server_disable_putc();
  }
#endif

  INFO("CPUID SETUP\n");
  cpuid_setup();
  INFO("PAT SETUP\n");
  pat_setup();
  // dmar_init();

  INFO("IDT SETUP\n");
  idt_create();
  idt_debug_host();
  idt_debug_bios();

#ifdef _VMCS_SHADOWING
  nested_vmx_shadow_bitmap_init();
#endif

  INFO("protected_begin 0x%X\n", setup_state->protected_begin);
  INFO("protected_end 0x%X\n", setup_state->protected_end);

  gdt_init();
  gdt_setup_guest_gdt();
  INFO("GUEST GDT DONE\n");
  gdt_setup_host_gdt();
  gdt_print_host_gdt();
  INFO("HOST GDT DONE\n");
  paging_setup_host_paging();
  INFO("PAGING HOST DONE\n");
  mtrr_create_ranges();
  INFO("MTRR CREATE RANGES DONE\n");
  mtrr_print_ranges();
  INFO("MTRR PRINT RANGES DONE\n");
  ept_create_tables(setup_state->protected_begin, setup_state->protected_end);
  INFO("EPT CREATE TABLES DONE\n");
  // apic_setup();
  INFO("APIC SETUP DONE\n");

  // Virtualization
  msr_bitmap_setup();
  INFO("MSR BITMAP DONE\n");
  msr_bitmap_set_for_mtrr();
  INFO("MSR BITMAP FOR MTRR DONE\n");
  msr_bitmap_set_write(MSR_ADDRESS_IA32_APIC_BASE);
  INFO("MSR BITMAP FOR APIC PROTECTION DONE\n");
  msr_bitmap_set_read_write(MSR_ADDRESS_IA32_TSC_DEADLINE);
  INFO("MSR BITMAP FOR TSC DEADLINE PROTECTION DONE\n");
  io_bitmap_setup();
  INFO("IO BITMAP DONE\n");
  // Ethernet card protection
  io_bitmap_set_for_port(PCI_CONFIG_ADDR);
  INFO("IO BITMAP FOR PCI CONFIG DONE\n");
  io_bitmap_set_for_port(PCI_CONFIG_DATA);
  INFO("IO BITMAP FOR PCI DATA DONE\n");

  // Launch APs initialization chain
  // TODO implement

  // Wait for the end of APs initialization chain
  // TODO implement

  vmm_init();
  vmcs_init();

  vmm_setup(/* TODO #core */);
  INFO("SETUP DONE\n");
  vmm_vm_setup_and_launch();
}

void vmm_setup() {
  vmcs_create_vmcs_regions();
  cpu_write_cr0(cpu_adjust64(cpu_read_cr0(), MSR_ADDRESS_VMX_CR0_FIXED0, MSR_ADDRESS_VMX_CR0_FIXED1));
  cpu_write_cr4(cpu_adjust64(cpu_read_cr4(), MSR_ADDRESS_VMX_CR4_FIXED0, MSR_ADDRESS_VMX_CR4_FIXED1));
  msr_check_feature_control_msr_lock();
  INFO("VMXON\n");
  cpu_vmxon((uint8_t *) vmxon);
  INFO("VMXON DONE\n");
}

void vmm_vm_setup_and_launch(void) {
  // XXX notion de VM courante
  struct vm *v;

  // Allocate a VM
  INFO("Allocate the first VM\n");
  vm_alloc(&v);
  // Set current VM
  vm_set(v);
  // Root VM
  vm_set_root(v);
  INFO("vmclear(0x%016X)\n", (uint64_t)&(v->vmcs_region)[0]);
  cpu_vmclear((uint8_t *) (uint64_t)&(v->vmcs_region)[0]);
  INFO("vmptrld(0x%016X)\n", (uint64_t)&(v->vmcs_region)[0]);
  cpu_vmptrld((uint8_t *) (uint64_t)&(v->vmcs_region)[0]);
  INFO("Cloning configuration\n");
  vmcs_clone(v->vmcs);
  INFO("Setting executive I/O bitmaps\n");
  io_bitmap_clone_a_b(&io_bitmap_a_pool[v->index][0],
      &io_bitmap_b_pool[v->index][0]);
  VMW(ctrls.ex.io_bitmap_a, (uint64_t)&io_bitmap_a_pool[v->index][0]);
  VMW(ctrls.ex.io_bitmap_b, (uint64_t)&io_bitmap_b_pool[v->index][0]);
  INFO("Setting executive msr bitmap\n");
  msr_bitmap_clone((uint8_t *)&msr_bitmap_pool[v->index]);
  VMW(ctrls.ex.msr_bitmap, (uint64_t)&msr_bitmap_pool[v->index]);

  INFO("Committing the configuration\n");
  vmcs_commit();
  INFO("READY TO GO!\n");

  // Call hook main
  hook_main();

  INFO("vmlaunch\n");

  cpu_vmlaunch(setup_state->vm_RIP, setup_state->vm_RSP, setup_state->vm_RBP);
}
