#include "types.h"
#include "screen.h"
#include "stdio.h"

#include "hardware/msr.h"
#include "pmem.h"
#include "vmem.h"
#include "vmm.h"
#include "vmm_info.h"
#include "vmm_setup.h"
#include "smp.h"

/*
 * Used to identify the size used of vmm (see the linker script).
 */
extern uint8_t kernel_end;
extern uint8_t kernel_start;

/* Vector of addresses: segment (first word) : offset (second word) */
uint32_t bios_ivt[256];

vmm_info_t *vmm_info;

void kernel_print_info(void) {
  INFO("kernel_start: %08X\n", (uint64_t) &kernel_start);
  INFO("kernel_end:   %08X\n", ((uint64_t) &kernel_end) & 0xffffffff);
}

void dump_bios_ivt() {
  for (int i = 0; i < 256; i++) {
    bios_ivt[i] = *((uint32_t*) (uint64_t) (4 * i));
  }

  /*for (int i = 0; i < 256; i++) {
    INFO("vector for int 0x%02x: segment = %04x, offset = %04x\n", i, bios_ivt[i] >> 16, bios_ivt[i] & 0xFFFF);
  }*/
}

void kernel_main(vmm_info_t *_vmm_info) {
  vmm_info = _vmm_info;
  screen_clear();
  dump_bios_ivt();
  kernel_print_info();
  //vmem_print_info();
  pmem_print_info(&vmm_info->pmem_mmap);
  pmem_fix_info(&vmm_info->pmem_mmap, VMEM_ADDR_PHYSICAL_TO_VIRTUAL(vmm_info->vmm_physical_start));
  pmem_print_info(&vmm_info->pmem_mmap);

  /*
   * Enables core/cpu.
   */
  //smp_setup();

  vmm_read_cmos();
  vmm_setup();
  vmm_ept_setup(&vmm_info->ept_info, vmm_info->vmm_physical_start, vmm_info->vmm_physical_end - vmm_info->vmm_physical_start + 1);
  vmm_vm_setup_and_launch();
}
