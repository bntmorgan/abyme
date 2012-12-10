#include "vmm_int.h"
#include "vmm_vmcs_int.h"
#include "vmem_int.h"

#include "arch/msr_int.h"
#include "common/string_int.h"
#include "include/vmm.h"

uint32_t vmcs_revision_identifier;
uint32_t number_bytes_regions;

/*
 * We need a VMXON region and at least one VMCS region in non-pageable memory,
 * of a size specified by IA32_VMX_BASIC MSR and aligned to a 4-KByte boundary.
 * See Volume 3, Section 31.5 of intel documentation.
 * See Volume 3, Section 31.6 of intel documentation.
 * See Volume 3, Section A.1 of intel documentation.
 * See Volume 3, Section 11.11 of intel documentation.
 */
/*
 * TODO: adapt memory type using MTRR.
 * See Volume 3, Section 11.11 of intel documentation.
 */
uint8_t vmxon[4096] __attribute((aligned(0x1000)));
uint8_t vmcs0[4096] __attribute((aligned(0x1000)));

/**
 * Sets up the VMM and enters VMX root operation.
 * See Volume 3, Section 23.7 of intel documentation.
 * See Volume 3, Section 31.5 of intel documentation.
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
   * See Volume 3, Section 11.11 of intel documentation.
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
  vmm_vmx_cr0_fixed();

  /*
   * 6) Enable VMX operation by setting CR4.VMXE = 1.
   *    Ensure the resultant CR4 value supports all the CR4 fixed bits reported
   *    in the IA32_VMX_CR4_FIXED0 and IA32_VMX_CR4_FIXED1 MSRs.
   */
  cpu_enable_vmxe();
  vmm_vmx_cr4_fixed();

  /*
   * 7) Ensure that the IA32_FEATURE_CONTROL MSR (MSR index 3AH) has been
   *    properly programmed and that its lock bit is set (Bit 0 = 1).
   */
  msr_check_feature_control_msr_lock();

  /*
   * 8) Execute VMXON with the physical address of the VMXON region as the operand.
   */
  vmm_vmxon();
}

/**
 * Sets up and launches a guest VM.
 * See Volume 3, Section 31.6 of intel documentation.
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
   * See Volume 3, Section 11.11 of intel documentation.
   */

  /*
   * 3) Execute the VMCLEAR instruction by supplying the guest-VMCS address.
   */
  vmm_vmclear();

  /*
   * 4) Execute the VMPTRLD instruction by supplying the guest-VMCS address.
   */
  vmm_vmptrld();

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
  vmm_vmlaunch();
}

/*
 * - All the physical memory is mapped using identity mapping.
 * - All the physical memory but the vmm memory is mapped using 1GB pages.
 * - The Page-Directory associated to the physical memory of the vmm is
 *   configured using 2MB pages.
 * - 2MB pages of the vmm memory are marked as non-readable, non-writable
     and non-executable in order to protect the vmm memory.
 *
 * See Volume 3, Section 28.2 of intel documentation.
 */
void vmm_ept_setup(ept_info_t *ept_info, uint32_t physical_mod_dest, uint32_t mod_dest_nb_pages_2MB) {
  /*
   * Everything stands into the first 4GB, so we only need the first entry of PML4.
   */
  ept_info->PML4[0] = vmem_virtual_address_to_physical_address((uint8_t*) ept_info->PDPT_PML40) | 0x07 /* R, W, X */;
  for (uint32_t i = 1; i < sizeof(ept_info->PML4) / sizeof(ept_info->PML4[0]); i++) {
    ept_info->PML4[i] = 0;
  }

  /*
   * Automatically map all memory accessed with PDPT_PML40 in 1GB pages,
   * except for the vmm memory for which we need 2MB granularity.
   */
  for (uint32_t i = 0; i < sizeof(ept_info->PDPT_PML40) / sizeof(ept_info->PDPT_PML40[0]); i++) {
    ept_info->PDPT_PML40[i] = (((uint64_t) i) << 30) | (1 << 7) /* 1GB page */ | 0x7 /* R, W, X */;
  }
  ept_info->PDPT_PML40[physical_mod_dest >> 30] = vmem_virtual_address_to_physical_address((uint8_t*) ept_info->PD_PDPT0_PML40) | 0x07 /* R, W, X */;

  /*
   * Automatically map all memory accessed with PD_PDPT0_PML40 in 2MB pages.
   */
  for (uint32_t i = 0; i < sizeof(ept_info->PD_PDPT0_PML40) / sizeof(ept_info->PD_PDPT0_PML40[0]); i++) {
    ept_info->PD_PDPT0_PML40[i] = (((uint64_t) i) << 21) | (1 << 7) /* 2MB page */ | 0x7 /* R, W, X */;
  }

  /*
   * Mark vmm memory as non-readable, non-writable and non-executable
   */
  for (uint32_t i = 0; i < mod_dest_nb_pages_2MB; i++) {
    if ((physical_mod_dest >> 30) != (((physical_mod_dest >> 21) + i) >> 9)) {
      ERROR("vmm pages don't belong to the same PDPT entry");
    }
    ept_info->PD_PDPT0_PML40[((physical_mod_dest >> 21) + i) & 0x1ff] = ((uint64_t) (physical_mod_dest + (i << 21))) | (1 << 7) /* 2MB page */;
  }
}

void vmm_vm_exit_handler(void) {
  __asm__ __volatile__(
    "push %rax    ;"
    "push %rcx    ;"
    "push %rdx    ;"
    "push %rbx    ;"
    "sub $8, %rsp ;" // push %rsp
    "push %rbp    ;"
    "push %rsi    ;"
    "push %rdi    ;"
    "push %r8     ;"
    "push %r9     ;"
    "push %r10    ;"
    "push %r11    ;"
    "push %r12    ;"
    "push %r13    ;"
    "push %r14    ;"
    "push %r15    ;"

    "call vmm_handle_vm_exit ;"

    "pop %r15     ;"
    "pop %r14     ;"
    "pop %r13     ;"
    "pop %r12     ;"
    "pop %r11     ;"
    "pop %r10     ;"
    "pop %r9      ;"
    "pop %r8      ;"
    "pop %rdi     ;"
    "pop %rsi     ;"
    "pop %rbp     ;"
    "add $8, %rsp ;" // pop %rsp
    "pop %rbx     ;"
    "pop %rdx     ;"
    "pop %rcx     ;"
    "pop %rax     ;");

  vmm_vmresume();
}

void vmm_handle_vm_exit(gpr64_t *guest_gpr) {
  INFO("VM-EXIT\n");

  guest_gpr->rsp = vmm_vmcs_read(GUEST_RSP);
  uint32_t guest_rip = vmm_vmcs_read(GUEST_RIP);
  uint32_t exit_reason = vmm_vmcs_read(VM_EXIT_REASON);
  uint32_t exit_instruction_length = vmm_vmcs_read(VM_EXIT_INSTRUCTION_LEN);

  vmm_vmcs_write(GUEST_RIP, guest_rip + exit_instruction_length);

  INFO("exit_reason = %d\n", exit_reason);
  INFO("----------\n");
  INFO("rip = 0x%x\n", guest_rip);
  INFO("rsp = 0x%x    rbp = 0x%x\n", guest_gpr->rsp, guest_gpr->rbp);
  INFO("rax = 0x%x    rbx = 0x%x\n", guest_gpr->rax, guest_gpr->rbx);
  INFO("rcx = 0x%x    rdx = 0x%x\n", guest_gpr->rcx, guest_gpr->rdx);
  INFO("rsi = 0x%x    rdi = 0x%x\n", guest_gpr->rsi, guest_gpr->rdi);
  INFO("r8  = 0x%x    r9  = 0x%x\n", guest_gpr->r8,  guest_gpr->r9);
  INFO("r10 = 0x%x    r11 = 0x%x\n", guest_gpr->r10, guest_gpr->r11);
  INFO("r12 = 0x%x    r13 = 0x%x\n", guest_gpr->r12, guest_gpr->r13);
  INFO("r14 = 0x%x    r15 = 0x%x\n", guest_gpr->r14, guest_gpr->r15);

  switch (exit_reason) {
    case EXIT_REASON_CPUID:
      INFO("handling CPUID (rax = %d)\n", guest_gpr->rax);
      __asm__ __volatile__("cpuid" : "=a" (guest_gpr->rax),
          "=b" (guest_gpr->rbx), "=c"(guest_gpr->rcx),
          "=d"(guest_gpr->rdx) : "a"(guest_gpr->rax));
      break;
    case EXIT_REASON_TASK_SWITCH: {
      // Checks here a gneral protection VM_EXIT
      // 25.4.2 Treatment of Task Switches
      // If CALL, INT n, or JMP accesses a task gate in IA-32e mode, a general-protection exception occurs
      // BIOS Call INT 15h (rax a e820)
      // Get the vm exit interrupt information
      uint32_t int_info = vmm_vmcs_read(VM_EXIT_INTR_INFO);
      // interruption 0x15 Miscellaneous system services
      if ((int_info & 0xff) == 0x15 && ((int_info & 0x700) >> 8) == 0x6) {
        // Query System Address Map gate e820
        if ((guest_gpr->rax & 0xff) == 0xe820) {
          INFO("BIOS interrupt call 0xe820\n");
          while(1);
        }
      }
    }
    default:
      INFO("unhandled reason: %d\n");
  }

}

void vmm_create_vmxon_and_vmcs_regions(void) {
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

void vmm_vmxon(void) {
  /*
   * vmxon needs physical address.
   */
  cpu_vmxon((uint8_t *) vmem_virtual_address_to_physical_address(vmxon));
}

void vmm_vmclear(void) {
  /*
   * vmclear needs physical address.
   */
  cpu_vmclear((uint8_t *) vmem_virtual_address_to_physical_address(vmcs0));
}

void vmm_vmptrld(void) {
  /*
   * vmclear needs physical address.
   */
  cpu_vmptrld((uint8_t *) vmem_virtual_address_to_physical_address(vmcs0));
}

void vmm_vmlaunch(void) {
  cpu_vmlaunch();
}

void vmm_vmresume(void) {
  cpu_vmresume();
}

void vmm_vmcs_write(uint32_t field, uint32_t value) {
  cpu_vmwrite(field, value);
}

uint32_t vmm_vmcs_read(uint32_t field) {
  return cpu_vmread(field);
}
