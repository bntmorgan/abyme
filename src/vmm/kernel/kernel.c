#include "types.h"
#include "screen.h"
#include "stdio.h"

#include "hardware/msr.h"
#include "pmem.h"
#include "vmem.h"
#include "vmm_info.h"
#include "vmm_setup.h"
#include "smp.h"

/*
 * Used to identify the size used of vmm (see the linker script).
 */
extern uint8_t kernel_end;
extern uint8_t kernel_start;

uint64_t vmm_stack;
uint64_t ept_pml4_addr;

void kernel_print_info(void) {
  INFO("kernel_start: %08X\n", (uint64_t) &kernel_start);
  INFO("kernel_end:   %08X\n", ((uint64_t) &kernel_end) & 0xffffffff);
}

void kernel_main(vmm_info_t *vmm_info) {
  screen_clear();
  kernel_print_info();
  //vmem_print_info();
  //pmem_print_info(&vmm_info->pmem_mmap);

  // XXX: Laid.
  vmm_stack = (uint64_t) vmm_info + sizeof(vmm_info_t);
  ept_pml4_addr = (uint64_t) VMEM_ADDR_VIRTUAL_TO_PHYSICAL(&vmm_info->ept_info.PML4[0]);

  /*
   * Enables core/cpu.
   */
  //smp_setup();

  vmm_read_cmos();
  vmm_setup();
  vmm_ept_setup(&vmm_info->ept_info, vmm_info->vmm_physical_start, vmm_info->vmm_physical_end - vmm_info->vmm_physical_start + 1);
  vmm_vm_setup_and_launch();
}
