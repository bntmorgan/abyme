#include "vmm.h"
#include "vmm_setup.h"
#include "vmcs.h"
#include "gdt.h"
#include "mtrr.h"
#include "ept.h"
#include "debug.h"

#include "msr.h"
#include "cpu.h"
#include "cpuid.h"
#include "stdio.h"

uint32_t vmcs_revision_identifier;
uint32_t number_bytes_regions;

// Declare the vmm stack
uint8_t vmm_stack[VMM_STACK_SIZE];

void vmm_main() {
  // Print the current virtual memory
  // configuration
  gdt_print_guest();
  // TODO test
  mtrr_initialize();
  mtrr_compute_memory_ranges();
  mtrr_print_ranges();
  // Dump the core state
  struct core_gpr gpr;
  struct core_cr cr;
  read_core_state(&gpr, &cr);
  dump_core_state(&gpr, &cr);
  // Save the current GDT
  INFO("Current GDT save\n");
  INFO("TODO!!!! TEST SI ID MAPPING POUR LE HOST!!!");
  INFO("TODO!!!! CREER UN NOUVEAU CR3 !!!! ON NE FAIT PAS CONFIANCE A UEFI !!!!");
  gdt_create_guest_gdt();
  vmm_setup();
  ept_create_tables();
  vmm_vm_setup_and_launch();
}

/*
 * TODO: adapt memory type using MTRR.
 */
uint8_t vmxon[4096] __attribute((aligned(0x1000)));
uint8_t vmcs0[4096] __attribute((aligned(0x1000)));

/**
 * Sets up the VMM and enters VMX root operation.
 * See [Intel_August_2012], volume 3, section 23.7.
 * See [Intel_August_2012], volume 3, section 31.5.
 */
void vmm_setup() {

  /*
   * 1) Check VMX support in processor using CPUID. Done by the loader.
   * 2) TODO: Determine the VMX capabilities supported by the processor
   *    through the VMX capability MSRs.
   */

  /*
   * 3) Create a VMXON region in non-pageable memory of a size specified by
   *    IA32_VMX_BASIC MSR and aligned to a 4-KByte boundary.
   * 4) Initialize the version identifier in the VMXON region (the first 32
   *    bits) with the VMCS revision identifier reported by capability MSRs.
   */
  /*
   * TODO: adapt memory type using MTRR.
   * See [Intel_August_2012], volume 3, section 11.11.
   */
  vmm_create_vmxon_and_vmcs_regions();

  /*
   * 5) Ensure the current processor operating mode meets the required CR0
   *    fixed bits (CR0.PE = 1, CR0.PG = 1).
   *    Other required CR0 fixed bits can be detected through the IA32_VMX_CR0_FIXED0 and IA32_VMX_CR0_FIXED1 MSRs.
   */
  /*
   * Because we assume that we are in long mode, pe and pg are already set.
   *  cpu_enable_pe();
   *  cpu_enable_pg();
   */
  cpu_enable_ne(); // Native x87 FPU errors reporting
  cpu_write_cr0(cpu_adjust64(cpu_read_cr0(), MSR_ADDRESS_VMX_CR0_FIXED0, MSR_ADDRESS_VMX_CR0_FIXED1));

  /*
   * 6) Enable VMX operation by setting CR4.VMXE = 1.
   *    Ensure the resultant CR4 value supports all the CR4 fixed bits reported
   *    in the IA32_VMX_CR4_FIXED0 and IA32_VMX_CR4_FIXED1 MSRs.
   */
  cpu_enable_vmxe();
  cpu_write_cr4(cpu_adjust64(cpu_read_cr4(), MSR_ADDRESS_VMX_CR4_FIXED0, MSR_ADDRESS_VMX_CR4_FIXED1));

  /*
   * 7) Ensure that the IA32_FEATURE_CONTROL MSR (MSR index 3AH) has been
   *    properly programmed and that its lock bit is set (Bit 0 = 1).
   */
  msr_check_feature_control_msr_lock();

  /*
   * 8) Execute VMXON with the physical address of the VMXON region as the operand.
   */
  cpu_vmxon((uint8_t*) vmxon);
}

void vmm_create_vmxon_and_vmcs_regions(void) {
  uint32_t i;
  for (i = 0; i < 4096; i++) {
    vmxon[i] = 0;
    vmcs0[i] = 0;
  }
  /*
   * We ignore bit 48 b because everything must stand into the first giga bytes
   * of memory.
   * See [Intel_August_2012], volume 3, section A.1.
   */
  uint64_t msr_value = msr_read(MSR_ADDRESS_IA32_VMX_BASIC);
  vmcs_revision_identifier = msr_value & 0xffffffff;
  number_bytes_regions = (msr_value >> 32) & 0x1fff;
  INFO("MSR_ADDRESS_IA32_VMX_BASIC: %X\n", msr_value);
  INFO("  vmxon region at %08x\n", (uint32_t) (uint64_t) &vmxon[0]);
  INFO("  vmcs0 region at %08x\n", (uint32_t) (uint64_t) &vmcs0[0]);
  INFO("  vmcs_revision_identifier=%08x, number_bytes_regions=%04x\n", vmcs_revision_identifier, number_bytes_regions);
  if (sizeof(vmxon) < number_bytes_regions ||
      sizeof(vmcs0) < number_bytes_regions) {
    ERROR("no enough pre-allocated bytes for vmcs structure\n");
  }
  /*
   * Write revision identifier into vmcs structures.
   */
  *((uint32_t *) &vmxon[0]) = vmcs_revision_identifier;
  *((uint32_t *) &vmcs0[0]) = vmcs_revision_identifier;
}

/**
 * Sets up and launches a guest VM.
 * See [Intel_August_2012], volume 3, section 31.6.
 */
void vmm_vm_setup_and_launch() {
  /*
   * 1) Create a VMCS region in non-pageable memory of size specified by
   *    the VMX capability MSR IA32_VMX_BASIC and aligned to 4-KBytes.
   * 2) Initialize the version identifier in the VMCS (first 32 bits)
   *    with the VMCS revision identifier reported by the VMX capability
   *    MSR IA32_VMX_BASIC.
   * These steps have been already done by vmm_create_vmxon_and_vmcs_regions().
   */
  /*
   * TODO: adapt memory type using MTRR.
   * See [Intel_August_2012], volume 3, section 11.11.
   */

  /*
   * 3) Execute the VMCLEAR instruction by supplying the guest-VMCS address.
   */
  cpu_vmclear((uint8_t*) vmcs0);

  /*
   * 4) Execute the VMPTRLD instruction by supplying the guest-VMCS address.
   */
  cpu_vmptrld((uint8_t*) vmcs0);

  /*
   * 5) Issue a sequence of VMWRITEs to initialize various host-state area
   *    fields in the working VMCS.
   */
  vmcs_fill_host_state_fields();

  /*
   * 6) Use VMWRITEs to set up the various VM-exit control fields, VM-entry
   *    control fields, and VM-execution control fields in the VMCS.
   */
  vmcs_fill_vm_exit_control_fields();
  vmcs_fill_vm_entry_control_fields();
  vmcs_fill_vm_exec_control_fields();

  /*
   * 7) Use VMWRITE to initialize various guest-state area fields in the
   *    working VMCS.
   */
  vmcs_fill_guest_state_fields();

  /*
   * 8) Execute VMLAUNCH to launch the guest VM.
   */
  INFO("READY TO GO!\n");
  cpu_vmlaunch();
  INFO("EPIC WIN!\n");
}
