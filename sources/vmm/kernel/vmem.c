#include "vmem.h"

#include "vmm_info.h"

#include "hardware/cpu.h"
#include "stdio.h"

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
  INFO("paging from cr3=%08x\n", cpu_read_cr3());
}
