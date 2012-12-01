#include <stdint.h>

#include "include/vmm.h"
#include "include/kernel.h"

#include "common/screen_int.h"
#include "common/string_int.h"
#include "arch/cpu_int.h"
#include "arch/cpuid_int.h"

#include "multiboot_int.h"
#include "pmem_int.h"
#include "vmem_int.h"
#include "mod_int.h"
#include "kernel_int.h"

/*
 * The header for the multiboot must be close to the beginning.
 */
uint32_t __mb_magic = MB_MAGIC;
uint32_t __mb_flags = MB_FLAGS;
uint32_t __mb_checksum = MB_CHECKSUM;

uint32_t mod_dest;
uint32_t reallocation_size;

vmm_info_t *vmm_info;

uint32_t vmm_info_offset;
uint64_t kernel_physical_start;

void kernel_memory_allocation(void) {
  /*
   * Identification of the memory slot used for the reallocation, based
   * on the memory map provided by the multiboot header.
   */
  /*
   * First, get the size in memory needed.
   */
  uint32_t padding = 4096 - (mod_get_size() % 4096);
  vmm_info_offset = mod_get_size() + padding;
  reallocation_size = vmm_info_offset + sizeof(vmm_info_t);
  kernel_physical_start = pmem_get_stealth_area(reallocation_size, 22);
  if (kernel_physical_start == 0) {
    ERROR("Null address for reallocation\n");
  }

  vmm_info = (vmm_info_t *) (uint32_t) vmem_addr_linear_to_logical_ds(kernel_physical_start + padding + mod_get_size());
  mod_dest = (uint32_t) vmem_addr_linear_to_logical_ds(kernel_physical_start);
  INFO("kernel at %08x\n", (uint32_t) mod_dest);
  INFO("vmm_info at %08x\n", (uint32_t) vmm_info);
}

void kernel_check(void) {
  if (cpu_is_protected_mode_enabled() == 0) {
    ERROR("before executing the kernel, the processor must be in protected mode\n");
  }
  if (cpu_is_paging_enabled() == 1) {
    ERROR("before executing the kernel, paging must be disabled\n");
  }
  if (cpuid_is_long_mode_supported() == 0) {
    ERROR("long mode not supported\n");
  }
  if (cpuid_is_pae_supported() == 0) {
    ERROR("pae not supported\n");
  }
  if (cpuid_is_vmx_supported() == 0) {
    ERROR("vmx not supported\n");
  }
  /*
   * Needed to configure smp.
   */
  if (cpuid_has_local_apic() == 0) {
    ERROR("no local apic\n");
  }
  /*
   * TODO: BIOS can deactivate VMX instruction.
   * Maybe we need to test using MSR IA32_FEATURE_CONTROL 0x3A
   * See: MODEL-SPECIFIC REGISTERS (MSRS), Chapter 35, Table 35-2.
   */
}

void kernel_copy_info(kernel_info_t *kernel_info) {
  kernel_info->kernel_physical_start = kernel_physical_start;
}

void kernel_main(uint32_t magic, uint32_t *address) {
  scr_clear();

  /*
   * cpuid information are needed to check hardware capabiblities.
   */
  cpuid_setup();

  kernel_check();
  mb_check(magic, address);

  mod_setup(mb_get_info());
  pmem_setup(mb_get_info());

  kernel_memory_allocation();
  mod_reallocation((uint8_t *) mod_dest);

  /*
   * TODO: make 0x200000 a macro!
   * TODO: instead of 2 pages, use reallocation_size!
   */
  vmem_setup(&vmm_info->vmem_info, mod_dest, 0x200000, 2);

  kernel_copy_info(&vmm_info->kernel_info);
  mod_copy_info(&vmm_info->mod_info);
  pmem_copy_info(&vmm_info->pmem_mmap);

  cpu_print_info();
  pmem_print_info(mb_get_info());
  vmem_print_info();
  mod_print_info();

  ACTION("switching to long mode (vmm_info at %08x)\n", (uint32_t) vmm_info);

  cpu_enable_pae();
  cpu_enable_long_mode();
  cpu_enable_paging();

  __asm__ __volatile__("mov %0, %%edi" : : "d"(vmm_info_offset + 0x200000));
  __asm__ __volatile__("ljmp $0x10,$0x200000");
}
