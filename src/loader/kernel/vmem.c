#include "vmem.h"

#include "stdio.h"

#include "hardware/cpu.h"

/*
 * We initialize the GDT for both protected mode and long mode.
 * See [Intel_August_2012], volume 3, section 3.5.1.
 * See [Intel_August_2012], volume 3, section 5.2.1.
 */
gdt_entry_t gdt_entries[] = {
    /* NULL */
  {0x00000000, 0x00000000, 0x00, 0x00},
    /* Code 32 */
  {0x00000000, 0xffffffff, 0x9a, 0xcf},
    /*
     * Code 64
     * See [Intel_August_2012], volume 3, section 5.2.1.
     */
  {0x00000000, 0xffffffff, 0x9a, 0xaf},
    /* Data 32 */
  {0x00000000, 0xffffffff, 0x92, 0xcf},
    /*
     * Data 64
     * See [Intel_August_2012], volume 3, section 5.2.1.
     */
  {0x00000000, 0xffffffff, 0x92, 0xaf},
};

void vmem_set_gdt_desc(uint8_t *gdt_desc, gdt_entry_t *entry) {
  /*
   * See [Intel_August_2012], volume 3, section 5.2.
   * See [Intel_August_2012], volume 3, section 5.2.1.
   */
  *((uint16_t *) (gdt_desc + 2)) = ((entry->base  >>  0) & 0xffff);
  *((uint8_t  *) (gdt_desc + 4)) = ((entry->base  >> 16) & 0x00ff);
  *((uint8_t  *) (gdt_desc + 7)) = ((entry->base  >> 24) & 0x00ff);
  *((uint16_t *) (gdt_desc + 0)) = ((entry->limit >>  0) & 0xffff);
  *((uint8_t  *) (gdt_desc + 6)) = ((entry->limit >> 16) & 0x000f) |
                                    (entry->granularity  & 0x00f0);
  *((uint8_t  *) (gdt_desc + 5)) =   entry->access;
}

void vmem_get_gdt_desc(uint8_t *gdt_desc, gdt_entry_t *entry) {
  /*
   * See [Intel_August_2012], volume 3, section 5.2.
   * See [Intel_August_2012], volume 3, section 5.2.1.
   */
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

void vmem_check_current_gdt_base(void) {
  uint32_t segment_selector;
  gdt_entry_t entry_cs;
  gdt_entry_t entry_ds;
  gdt_pm_ptr_t gdt_ptr;
  uint8_t *gdt_desc;
  /*
   * In order to access the GDT entry corresponding to a segment selector,
   * we don't care about the TI and RPL.
   * See [Intel_August_2012], volume 3, section 3.4.2.
   */
  gdt_desc = (uint8_t *) gdt_ptr.base;
  cpu_read_gdt((uint32_t *) &gdt_ptr);
  cpu_read_cs(&segment_selector);
  vmem_get_gdt_desc(gdt_desc + (8 * (segment_selector & ~0x7)), &entry_cs);
  cpu_read_ds(&segment_selector);
  vmem_get_gdt_desc(gdt_desc + (8 * (segment_selector & ~0x7)), &entry_ds);
  if (entry_cs.base != 0x0 || entry_ds.base != 0x0) {
    /*
     * Otherwise, we must take the base into account while switching
     * to the new GDT with cpu_write_gdt.
     */
    ERROR("the loader has not started its execution in flat model.\n");
  }
}

void vmem_setup_gdt(vmem_info_t *vmem_info) {
  gdt_pm_ptr_t gdt_ptr;
  ACTION("setup gdt\n");
  for (uint8_t i = 0; i < sizeof(gdt_entries) / sizeof(gdt_entries[0]); i++) {
    vmem_set_gdt_desc(&vmem_info->gdt_desc[8 * i], &gdt_entries[i]);
  }
  gdt_ptr.limit = sizeof(vmem_info->gdt_desc) - 1;
  gdt_ptr.base = (uint32_t) &vmem_info->gdt_desc;
  cpu_write_gdt((uint32_t) &gdt_ptr, 0x8, 0x18);
}

/*
 * All the memory is mapped with 1 GB pages using identity mapping.
 * See [Intel_August_2012], volume 3, section 4.5.1, figure 4.10.
 */
void vmem_setup_paging(vmem_info_t *vmem_info) {
  /*
   * Everything stand into the first 512 GB (more precisely, into the first 4 GB, due to
   * protected mode limitation), so we only need the first entry of PML4.
   */
  for (uint32_t i = 1; i < sizeof(vmem_info->PML4) / sizeof(vmem_info->PML4[0]); i++) {
    vmem_info->PML4[i] = 0;
  }
  vmem_info->PML4[0] = ((uint64_t) ((uint32_t) &vmem_info->PDPT_PML40[0])) | 0x03;
  /*
   * Automatically map all memory accessed with PDPT_PML40 in 1GB pages.
   */
  for (uint32_t i = 0; i < sizeof(vmem_info->PDPT_PML40) / sizeof(vmem_info->PDPT_PML40[0]); i++) {
    vmem_info->PDPT_PML40[i] = (((uint64_t) i) << 30) | VMEM_PDPT_PS_1G | 0x7;
  }
  INFO("eip before modifying cr3: %x\n", CPU_READ_EIP());
  cpu_write_cr3((uint32_t) &vmem_info->PML4);
}

/*
 * All the memory is mapped with 2 MB pages using identity mapping.
 * See [Intel_August_2012], volume 3, section 4.5.1, figure 4.9.
 */
void vmem_setup_paging_2MB(vmem_info_t *vmem_info) {
  /*
   * Everything stand into the first 512 GB (more precisely, into the first 4 GB, due to
   * protected mode limitation), so we only need the first entry of PML4.
   */
  for (uint32_t i = 1; i < sizeof(vmem_info->PML4) / sizeof(vmem_info->PML4[0]); i++) {
    vmem_info->PML4[i] = 0;
  }
  vmem_info->PML4[0] = ((uint64_t) (uint32_t) &vmem_info->PDPT_PML40[0]) | 0x03;
  /*
   * Automatically map all memory accessed with PDPT_PML40 in 2MB pages.
   */
  for (uint32_t i = 0; i < sizeof(vmem_info->PDPT_PML40) / sizeof(vmem_info->PDPT_PML40[0]); i++) {
    vmem_info->PDPT_PML40[i] = ((uint64_t) (uint32_t) &vmem_info->PD_PDPT_PML40[i][0]) | 0x7;
    for (uint32_t j = 0; j < sizeof(vmem_info->PD_PDPT_PML40[i]) / sizeof(vmem_info->PD_PDPT_PML40[i][0]); j++) {
      vmem_info->PD_PDPT_PML40[i][j] = (((uint64_t) (i * (sizeof(vmem_info->PD_PDPT_PML40[i]) / sizeof(vmem_info->PD_PDPT_PML40[i][0])) + j)) << 21) | VMEM_PDPT_PS_1G | 0x7;
    }
  }
  INFO("eip before modifying cr3: %x\n", CPU_READ_EIP());
  cpu_write_cr3((uint32_t) &vmem_info->PML4);
}

void vmem_setup(vmem_info_t *vmem_info) {
  vmem_check_current_gdt_base();
  vmem_setup_gdt(vmem_info);
  vmem_setup_paging_2MB(vmem_info);
}

void vmem_print_info(void) {
  /*
   * Print informations on GDT.
   */
  gdt_pm_ptr_t gdt_ptr;
  gdt_entry_t entry;
  cpu_read_gdt((uint32_t *) &gdt_ptr);
  INFO("gdt: base=%8x limit=%8x\n", gdt_ptr.base, gdt_ptr.limit);
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
  uint32_t cr3;
  cpu_read_cr3(&cr3);
  INFO("paging from cr3=%08x\n", cr3);
}
