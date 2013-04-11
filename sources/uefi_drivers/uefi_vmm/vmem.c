#include "vmem.h"

#include "vmm_info.h"

#include "hardware/cpu.h"
#include "stdio.h"
#include "string.h"

// Copy of the UEFI GDT for the host
uint8_t vmem_gdt[VMEM_GDT_SIZE] __attribute((aligned(0x4)));

// Copy of the UEFI GDT ptr
gdt_lm_ptr_t gdt_ptr, idt_ptr, saved_gdt_ptr;

void vmem_get_gdt_desc(uint8_t *gdt_desc, gdt_entry_t *entry) {
  entry->base = 0;
  entry->base |= (*((uint16_t *) (gdt_desc + 2)) <<  0) & 0xffff;
  entry->base |= (*((uint8_t *)  (gdt_desc + 4)) << 16) & 0xff0000;
  entry->base |= (*((uint8_t *)  (gdt_desc + 7)) << 24) & 0xff000000;
  entry->limit = 0;
  entry->limit |= (*((uint16_t *) (gdt_desc + 0)) <<  0) & 0xffff;
  entry->limit |= (*((uint8_t *)  (gdt_desc + 6)) << 16) & 0xf0000;
  entry->granularity = (*((uint8_t *)  (gdt_desc + 6)) <<  0) & 0x00f0;
  entry->access = (*((uint8_t *)  (gdt_desc + 5)) <<  0) & 0x00ff;
}

void vmem_save_gdt(void) {
  cpu_read_gdt((uint8_t *) &gdt_ptr);
  cpu_read_idt((uint8_t *) &idt_ptr);
  // Set the saved gdt ptr pointer
  saved_gdt_ptr.base = (uint32_t)((uint64_t)vmem_gdt);
  saved_gdt_ptr.limit = gdt_ptr.limit;
  // We have a limited size of GDT to save
  if (gdt_ptr.limit > VMEM_GDT_SIZE) {
    ERROR("Current GDT is too big\n");
  }
  // Test the size of the entries : for the moment
  // we don't want to manage the 16 bytes entries
  // See [Intel_August_2012], volume 3, section 5.2.
  gdt_entry_t entry;
  uint8_t *gdt_desc = (uint8_t *) gdt_ptr.base;
  while (gdt_desc < (uint8_t *) gdt_ptr.base + gdt_ptr.limit) {
    vmem_get_gdt_desc(gdt_desc, &entry);
    // Check NULL Segment descriptor
    if (!*((uint64_t *) gdt_desc)) {
      INFO("NULL segment descriptor\n");
    } else {
      INFO("S bit : %x\n", ((entry.access >> 4) & 0x1));
      // In 64 bit mode system descriptors are coded on 16 bytes
      if (!((entry.access >> 4) & 0x1)) {
        ERROR("Current GDT has 16 bytes system entries\n");
      }
      // Check if we have a flat segmentation
      if (entry.base != 0x0) {
        ERROR("Current GDT has non flat segment descriptors\n");
      }
    }
    gdt_desc = gdt_desc + 8;
  }
  // Finally perform the copy
  memcpy(vmem_gdt, (void *)gdt_ptr.base, gdt_ptr.limit);
}

void vmem_print_gdt(gdt_lm_ptr_t *gdt_ptr) {
  gdt_entry_t entry;
  INFO("gdt: base=%08X limit=%8x\n", (uint64_t) gdt_ptr->base, gdt_ptr->limit);
  uint8_t *gdt_desc = (uint8_t *) gdt_ptr->base;
  while (gdt_desc < (uint8_t *) gdt_ptr->base + gdt_ptr->limit) {
    vmem_get_gdt_desc(gdt_desc, &entry);
    VMEM_PRINT_GDT_ENTRY(&entry)
    gdt_desc = gdt_desc + 8;
  }
}

/*
 * TODO: find a way to make this function common 
 * (cr3 size is 32 bits for loader and 64 bits for vmm).
 */
void vmem_print_info(void) {
  /*
   * Print informations on GDT.
   */
  gdt_lm_ptr_t gdt_ptr;
  cpu_read_gdt((uint8_t *) &gdt_ptr);
  vmem_print_gdt(&gdt_ptr);
  /*
   * Print informations on CR3 (note that paging is configured for long mode).
   */
  INFO("paging from cr3=%08x\n", cpu_read_cr3());
}
