#include "kernel.h"

#include "hardware/cpu.h"
#include "hardware/cpuid.h"

#include "multiboot.h"
#include "pmem.h"
#include "vmem.h"
#include "elf64.h"

#include "vmm_info.h"
#include "screen.h"
#include "stdio.h"

/*
 * The header for the multiboot must be close to the beginning.
 * The bit field __mb_flags lists the information needed by this
 * kernel.
 * See Multiboot Specification version 0.6.96, section 3.1.
 */
uint32_t __mb_magic = MB_MAGIC;
uint32_t __mb_flags = MB_FLAGS;
uint32_t __mb_checksum = MB_CHECKSUM;

vmm_info_t *vmm_info;
uint32_t vmm_stack;
uint32_t vmm_physical_start;
uint32_t vmm_entry;

void kernel_vmm_allocation(void) {
  void *vmm_header = (void *) multiboot_get_module_start();
  uint32_t vmm_size = (uint32_t) elf64_get_size(vmm_header);
  uint32_t vmm_algn = (uint32_t) elf64_get_alignment(vmm_header);
  uint32_t padding = 4096 - (vmm_size % 4096);
  // TODO: VMM stack allocation is ugly.
  uint32_t reallocation_size = vmm_size + padding + sizeof(vmm_info_t) + VMM_STACK_SIZE;
  vmm_physical_start = pmem_get_stealth_area(reallocation_size, vmm_algn);
  vmm_entry = vmm_physical_start + elf64_get_entry(vmm_header);
  vmm_info = (vmm_info_t *) (vmm_physical_start + padding + vmm_size);
  vmm_stack = vmm_physical_start + vmm_size + padding + sizeof(vmm_info_t);
  elf64_load_relocatable_segment(vmm_header, (void *) vmm_physical_start);
  INFO("vmm_info at %08x\n", (uint32_t) vmm_info);
  INFO("vmm_stack at %08x\n", (uint32_t) vmm_stack);
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
  if (cpuid_is_page1g_supported() == 0) {
    ERROR("1 GB pages not supported\n");
  }
  if (cpuid_has_local_apic() == 0) {
    ERROR("no local apic\n");
  }
}

void kernel_main(uint32_t magic, uint32_t *address) {
  screen_clear();
  cpuid_setup();
  kernel_check();
  multiboot_setup(magic, address);
  pmem_setup(multiboot_get_info());
  kernel_vmm_allocation();
  vmem_setup(&vmm_info->vmem_info);
  pmem_copy_info(&vmm_info->pmem_mmap);
<<<<<<< HEAD
  cpu_print_info();
  pmem_print_info(multiboot_get_info());
  vmem_print_info();
=======

  /*cpu_print_info();
  pmem_print_info(mb_get_info());
  vmem_print_info();
  mod_print_info();*/

>>>>>>> 6695b0f0befe2eff1dbbff885d4cb318d8e99c9a
  ACTION("switching to long mode (vmm_info at %08x)\n", (uint32_t) vmm_info);
  ACTION("(vmm_entry at %08x)\n", (uint32_t) vmm_entry);
  ACTION("(vmm_physical_start at %08x)\n", (uint32_t) vmm_physical_start);
  cpu_enable_pae();
  cpu_enable_long_mode();
  cpu_enable_paging();

  __asm__ __volatile__(
      "mov %0, %%edi ;"
      "pushl $0x10   ;"
      "pushl %1      ;"
      "lret          ;"
    : : "d"(vmm_info), "m"(vmm_entry));
}
