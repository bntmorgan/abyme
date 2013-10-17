#include "vmm.h"
#include "setup.h"
#include "vmcs.h"
#include "gdt.h"
#include "mtrr.h"
#include "ept.h"
#include "pci.h"
#include "debug.h"
#include "paging.h"
#include "msr.h"
#include "cpu.h"
#include "cpuid.h"
#include "stdio.h"
#include "string.h"
#include "msr_bitmap.h"
#include "io_bitmap.h"
#include "debug_server/debug_server.h"
#include "smp.h"

//extern char __text;
extern uint8_t _padding_begin_a;
extern uint8_t _padding_begin_b;
extern uint8_t _padding_end_a;
extern uint8_t _padding_end_b;

uint32_t vmcs_revision_identifier;
uint32_t number_bytes_regions;
uint8_t vmm_stack[VMM_STACK_SIZE];

uint8_t vmxon[4096] __attribute((aligned(0x1000)));
uint8_t vmcs0[4096] __attribute((aligned(0x1000)));

void bsp_main() {
#ifdef _DEBUG_SERVER
  debug_server_init();
#endif
  cpuid_setup();

  INFO("VMCS addresses %X %X\n", vmxon, vmcs0);
  INFO("_padding_begin_a %X\n", &_padding_begin_a);
  INFO("_padding_begin_b %X\n", &_padding_begin_b);
  INFO("_padding_end_a   %X\n", &_padding_end_a);
  INFO("_padding_end_b   %X\n", &_padding_end_b);

  gdt_setup_guest_gdt();
  gdt_setup_host_gdt();
  paging_setup_host_paging();
  mtrr_create_ranges();
  mtrr_print_ranges();

  // Test smp
  smp_setup();

  // Virtualization
  msr_bitmap_setup();
  msr_bitmap_set_for_mtrr();
  io_bitmap_setup();
  // Ethernet card protection
  io_bitmap_set_for_port(PCI_CONFIG_ADDR);
  io_bitmap_set_for_port(PCI_CONFIG_DATA);
  ept_create_tables();

  // Launch APs initialization chain
  // TODO implement
  
  // Wait for the end of APs initialization chain
  // TODO implement

  vmm_setup(/* TODO #core */);
  vmm_vm_setup_and_launch(/* TODO #core */);
}

void vmm_setup() {
  vmm_create_vmxon_and_vmcs_regions();
  cpu_enable_ne();
  cpu_write_cr0(cpu_adjust64(cpu_read_cr0(), MSR_ADDRESS_VMX_CR0_FIXED0, MSR_ADDRESS_VMX_CR0_FIXED1));
  cpu_enable_vmxe();
  cpu_write_cr4(cpu_adjust64(cpu_read_cr4(), MSR_ADDRESS_VMX_CR4_FIXED0, MSR_ADDRESS_VMX_CR4_FIXED1));
  msr_check_feature_control_msr_lock();
  cpu_vmxon((uint8_t *) vmxon);
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
  cpu_vmlaunch();
  INFO("EPIC WIN!\n");
}
