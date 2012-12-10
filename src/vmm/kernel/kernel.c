#include <stdint.h>

#include "common/string_int.h"
#include "common/screen_int.h"

#include "arch/msr_int.h"

#include "vmem_int.h"
#include "vmm_int.h"
#include "pmem_int.h"
#include "smp_int.h"

#include "include/vmm.h"

extern uint8_t kernel_end;
extern uint8_t start;

uint64_t vmm_stack;
uint64_t ept_pml4_addr;

void kernel_check(vmm_info_t *vmm_info) {
  /*
   * We make sur the kernel is completly loaded.  It could be incompletly
   * loaded if, for example, the bss segment is the last one of the binary. In
   * this situation, the binary file is truncated (bss is always zero and it is
   * the operating system which must create and initialize this segment). So,
   * the module start and end in the multiboot header reflect only a subpart of
   * the overall binary. As a consequence, the loader may create the vmm_info
   * structure directly in the bss section (this structure will be erased by
   * the first data uninitialized of the first object conaining data, for
   * instance cursor_x of screen)!. That is why the rodata (data always
   * initialized) is always the last one in the linker script (hence, the
   * binary is not truncated).
   */
  if ((vmm_info->mod_info.mod_end + 1 - vmm_info->mod_info.mod_start) < (((uint64_t) &kernel_end) - ((uint64_t) &start))) {
    ERROR("kernel incompletely loaded\n");
  }
}

void kernel_print_info(vmm_info_t *vmm_info) {
  INFO("kernel_start: %08x\n", (uint32_t) ((uint64_t) &start));
  INFO("kernel_end:   %08x\n", (uint32_t) (((uint64_t) &kernel_end) & 0xffffffff));
  INFO("mod_start: %08x\n", (uint32_t) vmm_info->mod_info.mod_start);
  INFO("mod_end  : %08x\n", (uint32_t) vmm_info->mod_info.mod_end);
}

void kernel_main(vmm_info_t *vmm_info) {
  scr_clear();
  kernel_print_info(vmm_info);
  kernel_check(vmm_info);
  vmem_setup(&vmm_info->vmem_info, vmm_info->kernel_info.kernel_physical_start, (uint64_t) &start);
  //vmem_print_info();
  //pmem_print_info(&vmm_info->pmem_mmap);

  // XXX: Laid.
  vmm_stack = (uint64_t) vmm_info + sizeof(vmm_info_t);
  ept_pml4_addr = (uint64_t) vmem_virtual_address_to_physical_address((uint8_t*) vmm_info->ept_info.PML4);

  /*
   * Enables core/cpu.
   */
  smp_setup();

  vmm_setup();
  /*
   * TODO: replace the hardcoded size.
   */
  vmm_ept_setup(&vmm_info->ept_info, vmm_info->kernel_info.kernel_physical_start, 2);
  vmm_vm_setup_and_launch();
}
