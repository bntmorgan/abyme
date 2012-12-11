#include "vmem.h"

#include "vmm_info.h"

#include "hardware/cpu.h"
#include "stdio.h"

uint64_t vmem_virtual_address_to_physical_address(void *addr) {
  return (uint64_t) (uintptr_t) addr;
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

/*
 * TODO: find a way to make this function common (cr3 size is 32 bits for loader and 64 bits for vmm).
 */
void vmem_print_info(void) {
  /*
   * Print informations on GDT.
   */
  gdt_lm_ptr_t gdt_ptr;
  gdt_entry_t entry;
  cpu_read_gdt((uint8_t *) &gdt_ptr);
  INFO("gdt: base=%08X limit=%8x\n", (uint64_t) gdt_ptr.base, gdt_ptr.limit);
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
      INFO("  %03d: %08X\n", i, (uint64_t) PML4[i]);
      INFO("    PDPT at %08x\n", PML4[i] & 0xffffff00);
      uint64_t *PDPT = (uint64_t *) (PML4[i] & 0xffffffffffffff00);
      for (uint32_t j = 0; j < 512; j++) {
        if (PDPT[j] != 0) {
          if ((PDPT[j] & VMEM_PDPT_PS_1G) != 0) {
            /*
             * Will print too many informations:
             * INFO("    %03d: %08X (1G page)\n", j, (uint64_t) PDPT[j]);
             */
          } else {
            INFO("    %03d: %08X\n", j, (uint64_t) PDPT[j]);
            INFO("      PD at %08x\n", PDPT[j] & 0xffffff00);
            uint64_t *PD = (uint64_t *) (PDPT[j] & 0xffffffffffffff00);
            for (uint32_t k = 0; k < 512; k++) {
              if (PD[k] != 0) {
                INFO("      %03d: %08X\n", k, (uint64_t) PD[k]);
              }
            }
          }
        }
      }
    }
  }
}
