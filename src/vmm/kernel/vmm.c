#include "vmm_int.h"

#include "vmem_int.h"

#include "arch/msr_int.h"
#include "common/string_int.h"

uint32_t vmcs_revision_identifier;
uint32_t number_bytes_regions;

/*
 * We need at least two vmcs structure in order to use
 * unrestricted guest (for real mode virtual machine).
 * See Volume 3, Section 25.6 of intel documentation.
 * See Volume 3, Section 31.2.1 of intel documentation.
 *
 * These structure must be aligned on 4Kb memory.
 * See Volume 3, Section 24.1 of intel documentation.
 */
uint8_t vmcs0[4096] __attribute((aligned(0x1000)));
uint8_t vmcs1[4096] __attribute((aligned(0x1000)));

#include "include/vmm.h"

void vmm_create_vmcs(void) {
  uint32_t eax;
  uint32_t edx;
  INFO("read IA32_VMX_BASIC\n");

  /*
   * We ignore bit 48 b because everything must stand into the first giga bytes
   * of memory.
   * See Volume 3, Section A.1 of intel documentation.
   */
  msr_read(MSR_ADDRESS_IA32_VMX_BASIC, &eax, &edx);
  vmcs_revision_identifier = eax;
  number_bytes_regions = edx & 0x1fff;
  INFO("MSR_ADDRESS_IA32_VMX_BASIC:\n");
  INFO("  vmcs0 at %08x\n", (uint32_t) (uint64_t) &vmcs0[0]);
  INFO("  vmcs1 at %08x\n", (uint32_t) (uint64_t) &vmcs1[0]);
  INFO("  vmcs_revision_identifier=%08x, number_bytes_regions=%04x\n", vmcs_revision_identifier, number_bytes_regions);
  if (sizeof(vmcs0) < number_bytes_regions ||
      sizeof(vmcs1) < number_bytes_regions) {
    ERROR("no enough pre-allocated bytes for vmcs structure\n");
  }
  /*
   * Write revision identifier into vmcs structures.
   */
  *((uint32_t *) &vmcs0[0]) = vmcs_revision_identifier;;
  *((uint32_t *) &vmcs1[0]) = vmcs_revision_identifier;;
}

void vmm_vmx_cr4_fixed(void) {
  uint32_t eax;
  uint32_t edx;
  uint64_t msr;
  uint64_t cr4;
  cpu_read_cr4(&cr4);
  msr_read(MSR_ADDRESS_VMX_CR4_FIXED1, &eax, &edx);
  msr = (((uint64_t) edx) << 32) | ((uint64_t) eax);
  cr4 &= msr;
  msr_read(MSR_ADDRESS_VMX_CR4_FIXED0, &eax, &edx);
  msr = (((uint64_t) edx) << 32) | ((uint64_t) eax);
  cr4 |= msr;
  cpu_write_cr4(cr4);
}

void vmm_vmx_cr0_fixed(void) {
  uint32_t eax;
  uint32_t edx;
  uint64_t msr;
  uint64_t cr0;
  cpu_read_cr0(&cr0);
  msr_read(MSR_ADDRESS_VMX_CR0_FIXED1, &eax, &edx);
  msr = (((uint64_t) edx) << 32) | ((uint64_t) eax);
  cr0 &= msr;
  msr_read(MSR_ADDRESS_VMX_CR0_FIXED0, &eax, &edx);
  msr = (((uint64_t) edx) << 32) | ((uint64_t) eax);
  cr0 |= msr;
  cpu_write_cr0(cr0);
}

void vmm_vmxon(void) {
  /*
   * vmxon need physical address.
   */
  cpu_vmxon((uint8_t *) vmem_virtual_address_to_physical_address((uint8_t *) &vmcs0[0]));
}
