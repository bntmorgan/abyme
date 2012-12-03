#include "vmem_int.h"

#include "arch/cpu_int.h"
#include "common/string_int.h"

gdt_entry_t gdt_entries[] = {
    /* NULL */
  {0x00000000, 0x00000000, 0x00, 0x00},
    /* Code 32 */
  {0x00000000, 0xffffffff, 0x9a, 0xcf},
    /* Code 64 - """To enter long mode, the D/B bit (bit 22, 2nd dword) of the
     * GDT code segment must be clear (as it would be for a 16-bit code
     * segment), and the L bit (bit 21, 2nd dword) of the GDT code segment must
     * be set. """ (OSDEV)
     */
  {0x00000000, 0xffffffff, 0x9a, 0xaf},
    /* Data 32 */
  {0x00000000, 0xffffffff, 0x92, 0xcf},
    /* Data 64 */
  {0x00000000, 0xffffffff, 0x92, 0xaf},
};

void vmem_set_gdt_desc(uint8_t *gdt_desc, gdt_entry_t *entry) {
  *((uint16_t *) (gdt_desc + 2)) = ((entry->base  >>  0) & 0xffff);
  *((uint8_t  *) (gdt_desc + 4)) = ((entry->base  >> 16) & 0x00ff);
  *((uint8_t  *) (gdt_desc + 7)) = ((entry->base  >> 24) & 0x00ff);
  *((uint16_t *) (gdt_desc + 0)) = ((entry->limit >>  0) & 0xffff);
  *((uint8_t  *) (gdt_desc + 6)) = ((entry->limit >> 16) & 0x000f) |
                                    (entry->granularity  & 0x00f0);
  *((uint8_t  *) (gdt_desc + 5)) =   entry->access;
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

uint64_t vmem_addr_logical_to_linear(uint64_t addr, uint8_t desc) {
  gdt_pm_ptr_t gdt_ptr;
  gdt_entry_t entry;
  cpu_read_gdt((uint32_t *) &gdt_ptr);
  uint8_t *gdt_desc = (uint8_t *) (gdt_ptr.base + desc);
  vmem_get_gdt_desc(gdt_desc, &entry);
  return addr + entry.base;
}

uint64_t vmem_addr_logical_to_linear_ds(uint64_t addr) {
  uint32_t ds;
  cpu_read_ds(&ds);
  return vmem_addr_logical_to_linear(addr, ds);
}

uint64_t vmem_addr_linear_to_logical(uint64_t addr, uint8_t desc) {
  gdt_pm_ptr_t gdt_ptr;
  gdt_entry_t entry;
  cpu_read_gdt((uint32_t *) &gdt_ptr);
  uint8_t *gdt_desc = (uint8_t *) (gdt_ptr.base + desc);
  vmem_get_gdt_desc(gdt_desc, &entry);
  return addr - entry.base;
}

uint64_t vmem_addr_linear_to_logical_ds(uint64_t addr) {
  uint32_t ds;
  cpu_read_ds(&ds);
  return vmem_addr_linear_to_logical(addr, ds);
}

void vmem_setup_gdt(vmem_info_t *vmem_info) {
  ACTION("setup gdt\n");
  for (uint8_t i = 0; i < sizeof(gdt_entries) / sizeof(gdt_entries[0]); i++) {
    vmem_set_gdt_desc(&vmem_info->gdt_desc[8 * i], &gdt_entries[i]);
  }
  gdt_pm_ptr_t gdt_ptr;
  gdt_ptr.limit = sizeof(vmem_info->gdt_desc) - 1;
  gdt_ptr.base = (uint32_t) &vmem_info->gdt_desc;
  cpu_write_gdt((uint32_t) &gdt_ptr, 0x8, 0x18);
}

/*
 * - All the memory but the vmm memory is mapped with 1 GB pages using identity mapping.
 *   See "Intel 64 and IA-32 Architectures Software Developer's Manual", figure 4.10.
 * - The Page-Directory associated to the virtual memory of the vmm is configured using
 *   identity mapping with 2 MB pages for all memory but the vmm memory.
 *   See "Intel 64 and IA-32 Architectures Software Developer's Manual", figure 4.9.
 * - 2 MB pages of the vmm memory are mapped to vmm physical memory.
 */
void vmem_setup_paging2(vmem_info_t *vmem_info, uint32_t physical_mod_dest,
    uint32_t virtual_mod_dest __attribute__((unused)),
    uint32_t mod_dest_nb_pages_2MB __attribute__((unused))) {
  /*
   * We use identity mapping for the vmm in order to use the physical addresses for gdt and others.
   * So, the vmm is mapped twice (except if the physical address is 0x2000000).
   */
  /*
   * Everything stand into the first 4GB, so we only need the first entry of PML4.
   * In other words, it is useless to shift an uint32_t number 39 times to the right.
   */
  for (uint32_t i = 0; i < sizeof(vmem_info->PML4) / sizeof(vmem_info->PML4[0]); i++) {
    vmem_info->PML4[i] = 0;
  }
  /*
   * Automatically map all memory accessed with PDPT_PML40 in 1GB pages.
   */
  for (uint32_t i = 0; i < sizeof(vmem_info->PDPT_PML40) / sizeof(vmem_info->PDPT_PML40[0]); i++) {
    vmem_info->PDPT_PML40[i] = (((uint64_t) i) << 30) | VMEM_PDPT_PS_1G | 0x7;
  }
  /*
   * Automatically map all memory accessed with PD_PDPT0_PML40 in 2MB pages.
   */
  for (uint32_t i = 0; i < sizeof(vmem_info->PD_PDPT0_PML40) / sizeof(vmem_info->PD_PDPT0_PML40[0]); i++) {
    vmem_info->PD_PDPT0_PML40[i] = (((uint64_t) i) << 21) | 0x83;
  }
  /*
   * vmem_info->PML4[0] = ((uint64_t) (&vmem_info->PDPT_PML40[0])) | 0x07;
   */
  vmem_info->PML4[0] = (uint64_t) ((uint32_t) &vmem_info->PDPT_PML40[0]) | 0x03;
  /*
   * vmem_info->PDPT_PML40[0] = ((uint64_t) (&vmem_info->PD_PDPT0_PML40[0])) | 0x07;
   */
  vmem_info->PDPT_PML40[0] = (uint64_t) ((uint32_t) &vmem_info->PD_PDPT0_PML40[0]) | 0x03;
  /*
   * vmem_info->PD_PDPT0_PML40[0] = ((uint64_t) 0) | 0x83;
   */
  vmem_info->PD_PDPT0_PML40[0] = ((uint32_t) 0) | 0x83;
  /*
   * vmem_info->PD_PDPT0_PML40[1] = ((uint64_t) physical_mod_dest) | 0x83;
   */
  vmem_info->PD_PDPT0_PML40[1] = ((uint32_t) physical_mod_dest) | 0x83;
  /*
   * We use identity mapping for the vmm in order to use the physical addresses for gdt and others.
   * So, the vmm is mapped twice (except if the physical address is 0x2000000).
   * vmem_info->PD_PDPT0_PML40[physical_mod_dest >> 21] = ((uint64_t) physical_mod_dest) | 0x83;
   */
  vmem_info->PD_PDPT0_PML40[physical_mod_dest >> 21] = ((uint32_t) physical_mod_dest) | 0x83;

  INFO("eip before modifying cr3: %x\n", CPU_READ_EIP());
  cpu_write_cr3((uint32_t) &vmem_info->PML4);
}
void vmem_setup_paging(vmem_info_t *vmem_info, uint32_t physical_mod_dest, uint32_t virtual_mod_dest, uint32_t mod_dest_nb_pages_2MB) {
  /*
   * We use identity mapping for the vmm in order to use the physical addresses for gdt and others.
   * So, the vmm is mapped twice (except if the physical address is 0x2000000).
   */
  /*
   * Everything stand into the first 4GB, so we only need the first entry of PML4.
   * In other words, it is useless to shift an uint32_t number 39 times to the right.
   */
  for (uint32_t i = 0; i < sizeof(vmem_info->PML4) / sizeof(vmem_info->PML4[0]); i++) {
    vmem_info->PML4[i] = 0;
  }
  vmem_info->PML4[0] = ((uint64_t) ((uint32_t) &vmem_info->PDPT_PML40[0])) | 0x03;
  /*
   * Automatically map all memory accessed with PDPT_PML40 in 1GB pages.
   */
  for (uint32_t i = 0; i < sizeof(vmem_info->PDPT_PML40) / sizeof(vmem_info->PDPT_PML40[0]); i++) {
    vmem_info->PDPT_PML40[i] = (((uint64_t) i) << 30) | VMEM_PDPT_PS_1G | 0x7;
  }
  vmem_info->PDPT_PML40[virtual_mod_dest >> 30] = \
      ((uint64_t) ((uint32_t) &vmem_info->PD_PDPT0_PML40[0])) | 0x07;
  /*
   * Automatically map all memory accessed with PD_PDPT0_PML40 in 2MB pages.
   */
  for (uint32_t i = 0; i < sizeof(vmem_info->PD_PDPT0_PML40) / sizeof(vmem_info->PD_PDPT0_PML40[0]); i++) {
    vmem_info->PD_PDPT0_PML40[i] = (((uint64_t) i) << 21) | 0x83;
  }
  for (uint32_t i = 0; i < mod_dest_nb_pages_2MB; i++) {
    if ((virtual_mod_dest >> 30) != (((virtual_mod_dest >> 21) + i) >> 9)) {
      ERROR("kernel pages don't belong to the same PDPT entry");
    }
    vmem_info->PD_PDPT0_PML40[((virtual_mod_dest >> 21) + i) & 0x1ff] = \
        ((uint64_t) (physical_mod_dest + (i << 21))) | 0x83;
  }
  INFO("eip before modifying cr3: %x\n", CPU_READ_EIP());
  cpu_write_cr3((uint32_t) &vmem_info->PML4);
}


void vmem_setup(vmem_info_t *vmem_info, uint32_t physical_mod_dest, uint32_t virtual_mod_dest, uint32_t mod_dest_nb_pages_2MB) {
  vmem_setup_gdt(vmem_info);
  vmem_setup_paging(vmem_info, physical_mod_dest, virtual_mod_dest, mod_dest_nb_pages_2MB);
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
  INFO("  PML4 at %08x (from cr3)\n", cr3);
  uint64_t *PML4 = (uint64_t *) cr3;
  for (uint32_t i = 0; i < 512; i++) {
    if (PML4[i] != 0) {
      INFO("  %03d: %08x%08x\n", i, (uint32_t) (PML4[i] >> 32), (uint32_t) PML4[i]);
      INFO("    PDPT at %08x\n", PML4[i] & 0xffffff00);
      uint64_t *PDPT = (uint64_t *) ((uint32_t) (PML4[i] & 0xffffff00));
      for (uint32_t j = 0; j < 512; j++) {
        if (PDPT[j] != 0) {
          if ((PDPT[j] & VMEM_PDPT_PS_1G) != 0) {
            /*
             * Will print too many informations:
             * INFO("    %03d: %08x%08x (1G page)\n", j, (uint32_t) (PDPT[j] >> 32), (uint32_t) PDPT[j]);
             */
          } else {
            INFO("    %03d: %08x%08x\n", j, (uint32_t) (PDPT[j] >> 32), (uint32_t) PDPT[j]);
            INFO("      PD at %08x\n", PDPT[j] & 0xffffff00);
            uint64_t *PD = (uint64_t *) ((uint32_t) (PDPT[j] & 0xffffff00));
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
}
