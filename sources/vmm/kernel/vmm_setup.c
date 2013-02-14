#include "vmm.h"
#include "vmm_setup.h"
#include "vmm_vmcs.h"

#include "hardware/msr.h"
#include "vmem.h"
#include "stdio.h"

uint32_t vmcs_revision_identifier;
uint32_t number_bytes_regions;

/*
 * We need a VMXON region and at least one VMCS region in non-pageable memory,
 * of a size specified by IA32_VMX_BASIC MSR and aligned to a 4-KByte boundary.
 * See [Intel_August_2012], volume 3, section 31.5.
 * See [Intel_August_2012], volume 3, section 31.6.
 * See [Intel_August_2012], volume 3, section A.1.
 * See [Intel_August_2012], volume 3, section 11.11.
 */
/*
 * TODO: adapt memory type using MTRR.
 * See [Intel_August_2012], volume 3, section 11.11.
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
  cpu_vmxon((uint8_t*) VMEM_ADDR_VIRTUAL_TO_PHYSICAL(vmxon));
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
  cpu_vmclear((uint8_t*) VMEM_ADDR_VIRTUAL_TO_PHYSICAL(vmcs0));

  /*
   * 4) Execute the VMPTRLD instruction by supplying the guest-VMCS address.
   */
  cpu_vmptrld((uint8_t*) VMEM_ADDR_VIRTUAL_TO_PHYSICAL(vmcs0));

  /*
   * 5) Issue a sequence of VMWRITEs to initialize various host-state area
   *    fields in the working VMCS.
   */
  vmm_vmcs_fill_host_state_fields();

  /*
   * 6) Use VMWRITEs to set up the various VM-exit control fields, VM-entry
   *    control fields, and VM-execution control fields in the VMCS.
   */
  vmm_vmcs_fill_vm_exit_control_fields();
  vmm_vmcs_fill_vm_entry_control_fields();
  vmm_vmcs_fill_vm_exec_control_fields();

  /*
   * 7) Use VMWRITE to initialize various guest-state area fields in the
   *    working VMCS.
   */
  vmm_vmcs_fill_guest_state_fields();

  /*
   * 8) Execute VMLAUNCH to launch the guest VM.
   */
  INFO("READY TO GO!\n");
  cpu_vmlaunch();
}

/*
 * - All the physical memory is mapped using identity mapping.
 * - All the physical memory but the vmm memory is mapped using 1GB pages.
 * - The Page-Directory associated to the physical memory of the vmm is
 *   configured using 2MB pages.
 * - 2MB pages of the vmm memory are marked as non-readable, non-writable
 *   and non-executable in order to protect the vmm memory.
 *
 * See [Intel_August_2012], volume 3, section 28.2.
 */
void vmm_ept_setup(ept_info_t *ept_info, uintptr_t vmm_physical_start, uintptr_t vmm_size) {
  /*
   * Everything stands into the first 4GB, so we only need the first entry of PML4.
   */
  ept_info->PML4[0] = VMEM_ADDR_VIRTUAL_TO_PHYSICAL((uint8_t*) ept_info->PDPT_PML40) | 0x07 /* R, W, X */;
  for (uint32_t i = 1; i < sizeof(ept_info->PML4) / sizeof(ept_info->PML4[0]); i++) {
    ept_info->PML4[i] = 0;
  }

  /*
   * Automatically map all memory accessed with PDPT_PML40 in 2MB pages.
   */
  for (uint32_t i = 0; i < sizeof(ept_info->PDPT_PML40) / sizeof(ept_info->PDPT_PML40[0]); i++) {
    ept_info->PDPT_PML40[i] = ((uint64_t) VMEM_ADDR_VIRTUAL_TO_PHYSICAL(&ept_info->PD_PDPT_PML40[i][0])) | 0x7 /* R, W, X */;

    uint32_t nb_pde = sizeof(ept_info->PD_PDPT_PML40[i]) / sizeof(ept_info->PD_PDPT_PML40[i][0]);
    for (uint32_t j = 0; j < nb_pde; j++) {
      ept_info->PD_PDPT_PML40[i][j] = (((uint64_t) (i * nb_pde + j)) << 21) | (1 << 7) /* 2MB page */ | 0x7 /* R, W, X */;
    }
  }

  /*
   * Mark vmm memory as non-readable, non-writable and non-executable
   */
  /*
   * 1. Get the size of the vmm, including the padding between the start of the vmm
   *    and the beginning of its first 2 MB page.
   * 2. Get the address of the first 2 MB page of the vmm.
   * 3. Count the number of 2 MB pages used for the vmm and adjust if the last one is
   *    partially used.
   */
  vmm_size = vmm_size + (vmm_physical_start % 0x200000);
  vmm_physical_start = vmm_physical_start - (vmm_physical_start % 0x200000);

  uint64_t vmm_nb_pages_2MB = vmm_size / 0x200000;
  if (vmm_size % 0x200000 > 0) {
    vmm_nb_pages_2MB = vmm_nb_pages_2MB + 1;
  }

  for (uint64_t i = 0; i < vmm_nb_pages_2MB; i++) {
    if ((vmm_physical_start >> 30) != (((vmm_physical_start >> 21) + i) >> 9)) {
      ERROR("vmm pages don't belong to the same PDPT entry");
    }

    /* TODO: create separate readable pages for VMM data that can be read by the guest */
    ept_info->PD_PDPT_PML40[vmm_physical_start >> 30][((vmm_physical_start >> 21) + i) & 0x1ff] =
        ((uint64_t) (vmm_physical_start + (i << 21))) | (1 << 7) /* 2MB page */;
  }
}

void vmm_create_vmxon_and_vmcs_regions(void) {
  uint32_t eax;
  uint32_t edx;
  INFO("read IA32_VMX_BASIC\n");

  /*
   * We ignore bit 48 b because everything must stand into the first giga bytes
   * of memory.
   * See [Intel_August_2012], volume 3, section A.1.
   */
  msr_read(MSR_ADDRESS_IA32_VMX_BASIC, &eax, &edx);
  vmcs_revision_identifier = eax;
  number_bytes_regions = edx & 0x1fff;
  INFO("MSR_ADDRESS_IA32_VMX_BASIC:\n");
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
