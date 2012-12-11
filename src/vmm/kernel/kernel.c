#include "types.h"

#include "screen.h"
#include "stdio.h"

#include "hardware/msr.h"

#include "vmem.h"
#include "vmm.h"
#include "pmem.h"
#include "smp.h"

#include "vmm_info.h"

extern uint8_t kernel_end;
extern uint8_t start;

uint64_t vmm_stack;
uint64_t ept_pml4_addr;

void kernel_print_info(void) {
  INFO("kernel_start: %08x\n", (uint32_t) ((uint64_t) &start));
  INFO("kernel_end:   %08x\n", (uint32_t) (((uint64_t) &kernel_end) & 0xffffffff));
}

void kernel_main(vmm_info_t *vmm_info) {
  screen_clear();
  kernel_print_info();
  vmem_setup(&vmm_info->vmem_info, vmm_info->kernel_info.kernel_physical_start, (uint64_t) &start);
  //vmem_print_info();
  //pmem_print_info(&vmm_info->pmem_mmap);

  // XXX: Laid.
  vmm_stack = (uint64_t) vmm_info + sizeof(vmm_info_t);
  ept_pml4_addr = vmem_virtual_address_to_physical_address((uint8_t *) &vmm_info->ept_info.PML4);

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
