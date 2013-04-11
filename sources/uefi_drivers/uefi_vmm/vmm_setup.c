#include "vmm.h"
#include "vmm_setup.h"
#include "vmm_vmcs.h"
#include "vmm_info.h"
#include "vmem.h"
#include "mtrr.h"
#include "debug.h"

#include "hardware/msr.h"
#include "hardware/cpu.h"
#include "hardware/cpuid.h"
#include "stdio.h"

uint32_t vmcs_revision_identifier;
uint32_t number_bytes_regions;

// Declare ept tables
ept_info_t ept_info;

// Declare the vmm stack
uint8_t vmm_stack[VMM_STACK_SIZE];

void vmm_main() {
  // Print the current virtual memory
  // configuration
  vmem_print_info();
  // Dump the core state
  struct core_gpr gpr;
  struct core_cr cr;
  read_core_state(&gpr, &cr);
  dump_core_state(&gpr, &cr);
  // Save the current GDT
  INFO("Current GDT save\n");
  vmem_save_gdt();
  vmm_setup();
  vmm_ept_setup(&ept_info);
  vmm_vm_setup_and_launch();
}

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

void vmm_create_vmxon_and_vmcs_regions(void) {
  uint32_t eax;
  uint32_t edx;

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
  msr_read(MSR_ADDRESS_IA32_VMX_BASIC, &eax, &edx);
  vmcs_revision_identifier = eax;
  number_bytes_regions = edx & 0x1fff;
  INFO("MSR_ADDRESS_IA32_VMX_BASIC: eax : %08x edx : %08x\n", eax, edx);
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

/*
 * - All the physical memory is mapped using identity mapping.
 */
void vmm_ept_setup(ept_info_t *ept_info) {
  /*
   * Everything stands into the first 4GB, so we only need the first entry of PML4.
   */
  ept_info->PML4[0] = VMEM_ADDR_VIRTUAL_TO_PHYSICAL((uint8_t*) ept_info->PDPT_PML40) | 0x07 /* R, W, X */;
  uint32_t i, j, k;
  for (i = 1; i < sizeof(ept_info->PML4) / sizeof(ept_info->PML4[0]); i++) {
    ept_info->PML4[i] = 0;
  }

  /*
   * Read the mtrr to configure ept default cache
   */

  uint64_t mtrr_cap = msr_read64(MSR_ADDRESS_IA32_MTRRCAP);
  uint64_t mtrr_def_type = msr_read64(MSR_ADDRESS_A32_MTRR_DEF_TYPE);
  uint8_t mtrr_default_type = 0;
  uint8_t mtrr_active = 0;
  printk("-> %X <-\n", mtrr_cap);
  printk("-> %X <-\n", mtrr_def_type);
  mtrr_active = (mtrr_def_type & (1 << 11)) && mtrr_support();
  printk("ioezjiozef %d\n", mtrr_active);
  if (mtrr_active) {
    mtrr_default_type = mtrr_def_type & 0xff;
    printk("%x\n", mtrr_default_type);
    if (!MTRR_VALID_TYPE(mtrr_default_type)) {
      //TODO: must be an error???
      ERROR("bad value for mtrr_def_type...\n");
      while (1);
    }
  }

  /*
   * Automatically map all memory accessed with PDPT_PML40 in 2MB pages.
   * <<
   */
  for (i = 0; i < sizeof(ept_info->PDPT_PML40) / sizeof(ept_info->PDPT_PML40[0]); i++) {
    ept_info->PDPT_PML40[i] = ((uint64_t) VMEM_ADDR_VIRTUAL_TO_PHYSICAL(&ept_info->PD_PDPT_PML40[i][0])) | 0x7 /* R, W, X */;
    uint32_t nb_pde = sizeof(ept_info->PD_PDPT_PML40[i]) / sizeof(ept_info->PD_PDPT_PML40[i][0]);
    for (j = 0; j < nb_pde; j++) {
      /*
       * Map the 2 first megabytes in 4ko pages
       */
      if (i == 0 && j == 0) {
        ept_info->PD_PDPT_PML40[i][j] = ((uint64_t) VMEM_ADDR_VIRTUAL_TO_PHYSICAL(&ept_info->PT_PD0[0])) | 0x7 /* R, W, X */;
        for (k = 0; k < sizeof(ept_info->PT_PD0); k++) {
          ept_info->PT_PD0[k] = ((uint64_t) k << 12) | 0x7 /* R, W, X */ | (mtrr_default_type << 3);
        }
      } else {
        ept_info->PD_PDPT_PML40[i][j] = (((uint64_t) (i * nb_pde + j)) << 21) | (1 << 7) /* 2MB page */ | 0x7 /* R, W, X */ | (mtrr_default_type << 3);
      }
    }
  }

  if (mtrr_active) {
    mtrr_fixed_read();
    mtrr_print();

    uint8_t fixed_step;
    uint64_t index;
    uint64_t types;

    index = 0;
    fixed_step = 16;
    types = mtrr_fixed.fix64K_00000.q;
#define TODO \
    for (i = 0; i < 8; i++) {\
      for (j = 0; j < fixed_step; j++) {\
        uint8_t type = (types >> (8 * i)) & 0xff;\
        ept_info->PT_PD0[index] |= type << 3;\
        index++;\
      }\
    }
    TODO
      printk("index MTRR fixed: %d\n", index);
    fixed_step = 4;
    types = mtrr_fixed.fix16K_80000.q;
    TODO
    types = mtrr_fixed.fix16K_A0000.q;
    TODO
    fixed_step = 1;
    types = mtrr_fixed.fix4K_C0000.q;
    TODO
    types = mtrr_fixed.fix4K_C8000.q;
    TODO
    types = mtrr_fixed.fix4K_D0000.q;
    TODO
    types = mtrr_fixed.fix4K_D8000.q;
    TODO
    types = mtrr_fixed.fix4K_E0000.q;
    TODO
    types = mtrr_fixed.fix4K_E8000.q;
    TODO
    types = mtrr_fixed.fix4K_F0000.q;
    TODO
    types = mtrr_fixed.fix4K_F8000.q;
    TODO
    for (j = 0; j < 256; j++) {
      ept_info->PT_PD0[index] |= 6 << 3;
      index++;
    }

    printk("index MTRR fixed: %d\n", index);
    printk("ioezjiozef %d\n", mtrr_active);

    /* Variable MTRR: TODO */
    // TODO: check if these mttr are always closed together.
    uint64_t msr_base = MSR_ADDRESS_IA32_MTRR_PHYBASE0;
    uint64_t msr_mask = MSR_ADDRESS_IA32_MTRR_PHYBASE0 + 1;
    //CPUID.80000008H:EAX
    uint8_t maxphyaddr = cpuid_get_maxphyaddr();
    printk("%X\n", maxphyaddr);
    for (i = 0; i < (mtrr_cap & 0xff); i++) {
      uint64_t value_base = msr_read64(msr_base);
      uint64_t value_mask = msr_read64(msr_mask);
      if ((value_mask & (((uint64_t) 1) << 11)) != 0) {
        uint8_t type = value_base & 0x7;
        if (!MTRR_VALID_TYPE(type)) {
          INFO("Bad value ???");
          while(1);
        }
        uint64_t addr_base = value_base & 0xfffffffffffff000;
        //uint64_t addr_base_save = value_base & 0xfffffffffffff000;
        uint64_t addr_mask = (~(value_mask & 0xfffffffffffff000)) & (((uint64_t) 1 << maxphyaddr) - 1);
        //printk("      %016X       %016X\n", addr_base, addr_mask);
        uint64_t addr_limit = addr_base + addr_mask;
        printk("B: %010X M: %010X L: %010X E: %010X\n", value_base, value_mask, addr_mask, addr_limit);
        //uint64_t pdpt = addr_base >> 30;
        //uint64_t pd = addr_base >> 21;
        //printk("- %04x %04x\n", pdpt, pd);
          // TODO verifier que limit est multiple de 2mb
        uint64_t pdpt = addr_base >> 30;
        uint64_t pd = (addr_base >> 21) - (pdpt << (30 - 21));
        while (addr_base < addr_limit) {
          pdpt = addr_base >> 30;
          pd = (addr_base >> 21) - (pdpt << (30 - 21));
          if (addr_base > 0) {
            ept_info->PD_PDPT_PML40[pdpt][pd] &= ~(((uint64_t) 0x7) << 3);
            ept_info->PD_PDPT_PML40[pdpt][pd] |= type << 3;
          }
          //printk("- %04x %04x\n", pdpt, pd);
          addr_base += 1024 * 1024 * 2;
        }
        //pdpt = addr_base >> 30;
        //pd = addr_base >> 21;
        //printk("- %04x %04x\n", pdpt, pd);
      }
      msr_base += 2;
      msr_mask += 2;
    }
  }
  uint64_t vv = (uint64_t) &(ept_info->PD_PDPT_PML40[0]);
  printk("%X\n", vv);
  uint64_t pdpt = vv >> 30;
  uint64_t pd = (vv >> 21) - (pdpt << (30 - 21));
  vv = ept_info->PD_PDPT_PML40[pdpt][pd];
  printk("%X %x %x!!!!\n", vv, pdpt, pd);
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
  INFO("EPIC WIN!\n");
}
