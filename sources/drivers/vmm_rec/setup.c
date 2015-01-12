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
#include "msr.h"
#include "cpu.h"
#include "cpuid.h"
#include "stdio.h"
#include "string.h"
#include "msr_bitmap.h"
#include "io_bitmap.h"
#ifdef _DEBUG_SERVER
#include "debug_server/debug_server.h"
#endif
#include "smp.h"

//extern char __text;
extern uint8_t _protected_begin;
extern uint8_t _protected_end;

uint32_t vmcs_revision_identifier;
uint32_t number_bytes_regions;

void bsp_main() {
#ifdef _DEBUG_SERVER
  debug_server_init();
#endif
  INFO("CPUID SETUP\n");
  cpuid_setup();
  INFO("PAT SETUP\n");
  pat_setup();

  INFO("VMCS addresses %X %X\n", vmxon, vmcs0);
  INFO("_protected_begin %X\n", &_protected_begin);
  INFO("_protected_end %X\n", &_protected_end);

  gdt_setup_guest_gdt();
  INFO("GUEST GDT DONE\n");
  gdt_setup_host_gdt();
  INFO("HOST GDT DONE\n");
  paging_setup_host_paging();
  INFO("PAGING HOST DONE\n");
  mtrr_create_ranges();
  INFO("MTRR CREATE RANGES DONE\n");
  mtrr_print_ranges();
  INFO("MTRR PRINT RANGES DONE\n");
  ept_create_tables();
  INFO("EPT CREATE TABLES DONE\n");

  // Test smp
  // smp_setup();

  // Virtualization
  msr_bitmap_setup();
  INFO("MSR BITMAP DONE\n");
  msr_bitmap_set_for_mtrr();
  INFO("MSR BITMAP FOR MTRR DONE\n");
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

  vmm_setup(/* TODO #core */);
  INFO("SETUP DONE\n");
  vmm_vm_setup_and_launch(/* TODO #core */);
}

void vmm_setup() {
  vmm_init();
  vmm_create_vmxon_and_vmcs_regions();
  cpu_write_cr0(cpu_adjust64(cpu_read_cr0(), MSR_ADDRESS_VMX_CR0_FIXED0, MSR_ADDRESS_VMX_CR0_FIXED1));
  cpu_write_cr4(cpu_adjust64(cpu_read_cr4(), MSR_ADDRESS_VMX_CR4_FIXED0, MSR_ADDRESS_VMX_CR4_FIXED1));
  msr_check_feature_control_msr_lock();
  INFO("VMXON\n");
  cpu_vmxon((uint8_t *) vmxon);
  INFO("VMXON DONE\n");
}

void vmm_create_vmxon_and_vmcs_regions(void) {
  memset((uint8_t *) &vmxon[0], 0, 0x1000);
  memset((uint8_t *) &vmcs0[0], 0, 0x1000);

  uint64_t msr_value = msr_read(MSR_ADDRESS_IA32_VMX_BASIC);
  vmcs_revision_identifier = msr_value & 0xffffffff;
  number_bytes_regions = (msr_value >> 32) & 0x1fff;
  if (sizeof(vmxon) < number_bytes_regions || sizeof(vmcs0) < number_bytes_regions) {
    panic("!#SETUP SZ");
  }
  *((uint32_t *) &vmxon[0]) = vmcs_revision_identifier;
  *((uint32_t *) &vmcs0[0]) = vmcs_revision_identifier;
}

void vmm_vm_setup_and_launch() {
  cpu_vmclear((uint8_t *) vmcs0);
  cpu_vmptrld((uint8_t *) vmcs0);
  vmcs_fill_host_state_fields();
  vmcs_fill_vm_exit_control_fields();
  vmcs_fill_vm_entry_control_fields();
  vmcs_fill_vm_exec_control_fields();
  vmcs_fill_guest_state_fields();
  INFO("READY TO GO!\n");

#ifdef _DEBUG_SERVER
  if (debug_server) {
    INFO("Debug infos by ethernet enabled\n");
    debug_server_enable_putc();
  } else {
    INFO("Debug infos totally disabled\n");
    debug_server_disable_putc();
  }
#endif

  cpu_vmlaunch();
}
