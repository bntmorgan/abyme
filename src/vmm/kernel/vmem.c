#include "vmem_int.h"

#include "include/vmm.h"

#include "arch/cpu_int.h"
#include "common/string_int.h"

uint64_t vmm_physical_start;
uint64_t vmm_virtual_start;

uint64_t vmem_virtual_address_to_physical_address(uint8_t *addr) {
  /*
   * In theory, segmentation and pagination are used to translate a virtual
   * address in physical address. In order to get this physical address, we
   * must walk through the gdt structure and the PML4/PDPT/PD structures.
   *
   * On the other hand, our memory managment is very simple: 1) we use flat
   * segmentation and 2) all our vmm stands into an adjacent block of physical
   * memory and 3) all our vmm stands into an adjacent block of virtual memory.
   * So, we only need to substract the starting address of virtual memory and
   * add the starting address of physical memory.
   */
  return ((uint64_t) addr) - vmm_virtual_start + vmm_physical_start;
}

void vmem_setup(vmem_info_t *vmem_info, uint64_t physical_start, uint64_t virtual_start) {
  vmm_physical_start = physical_start;
  vmm_virtual_start = virtual_start;
}

void vmem_get_gdt_desc(uint8_t *gdt_desc, gdt_entry_t *entry) {
  entry->base = 0;
  entry->base |= (*((uint16_t *) (gdt_desc + 2)) <<  0) & 0xffff;
  entry->base |= (*((uint8_t *)  (gdt_desc + 4)) << 16) & 0x00ff;
  entry->base |= (*((uint8_t *)  (gdt_desc + 7)) << 24) & 0x00ff;
  entry->limit = 0;
  entry->limit |= (*((uint16_t *) (gdt_desc + 0)) <<  0) & 0xffff;
  entry->limit |= (*((uint8_t *)  (gdt_desc + 6)) << 16) & 0x000f;
  entry->granularity = (*((uint8_t *)  (gdt_desc + 6)) <<  0) & 0x00f0;
  entry->access = (*((uint8_t *)  (gdt_desc + 5)) <<  0) & 0x00ff;
}

void vmem_print_info(void) {
  /*
   * Print informations on GDT.
   */
  gdt_lm_ptr_t gdt_ptr;
  gdt_entry_t entry;
  cpu_read_gdt((uint8_t *) &gdt_ptr);
  INFO("gdt: base=%08x%08x limit=%8x\n", (uint32_t) ((gdt_ptr.base >> 32) & 0xffffffff), (uint32_t) (gdt_ptr.base & 0xffffffff), gdt_ptr.limit);
  uint8_t *gdt_desc = (uint8_t *) gdt_ptr.base;
  while (gdt_desc < (uint8_t *) gdt_ptr.base + gdt_ptr.limit) {
    vmem_get_gdt_desc(gdt_desc, &entry);
    INFO("  base=%08x limit=%08x access=%02x granularity=%02x\n",
      entry.base, entry.limit, entry.access, entry.granularity);
    gdt_desc = gdt_desc + 8;
  }
  /*
   * Print informations on CR3 (note that paging is configured for long mode).
   */
  uint64_t cr3;
  cpu_read_cr3(&cr3);

  INFO("paging from cr3=%08x\n", cr3);
  INFO("  PML4 at %08x (from cr3)\n", cr3);
  uint64_t *PML4 = (uint64_t *) cr3;
  for (uint32_t i = 0; i < 512; i++) {
    if (PML4[i] != 0) {
      INFO("  %03d: %08x%08x\n", i, (uint32_t) (PML4[i] >> 32), (uint32_t) PML4[i]);
      INFO("    PDPT at %08x\n", PML4[i] & 0xffffff00);
      uint64_t *PDPT = (uint64_t *) ((uint64_t) (PML4[i] & 0xffffff00));
      for (uint32_t j = 0; j < 512; j++) {
        if (PDPT[j] != 0) {
          INFO("    %03d: %08x%08x\n", j, (uint32_t) (PDPT[j] >> 32), (uint32_t) PDPT[j]);
          INFO("      PD at %08x\n", PDPT[j] & 0xffffff00);
          uint64_t *PD = (uint64_t *) ((uint64_t) (PDPT[j] & 0xffffff00));
          for (uint32_t k = 0; k < 512; k++) {
            if (PD[k] != 0) {
              INFO("      %03d: %08x%08x\n", k, (uint32_t) (PD[k] >> 32), (uint32_t) PD[k]);
            }
          }
        }
      }
    }
  }
}
