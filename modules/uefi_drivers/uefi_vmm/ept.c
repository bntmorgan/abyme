#include "ept.h"

// TODO: pour inclure stdio dans ept? => trouver une autre solution?
#include "stdio.h"

#include "mtrr.h"
#include "debug_server/debug_server.h"
#include "pci.h"

struct ept_tables {
  uint64_t PML4[512]               __attribute__((aligned(0x1000)));
  uint64_t PDPT_PML40[512]         __attribute__((aligned(0x1000)));
  uint64_t PD_PDPT_PML40[512][512] __attribute__((aligned(0x1000)));
  uint64_t PT_PD0[512]             __attribute__((aligned(0x1000)));
  uint64_t PT_PDX[512]             __attribute__((aligned(0x1000)));
  uint64_t PT_PDX2[512]             __attribute__((aligned(0x1000)));
} __attribute__((aligned(8)));

struct ept_tables ept_tables;

extern uint8_t _padding_begin_a;
extern uint8_t _padding_begin_b;
extern uint8_t _padding_end_a;
extern uint8_t _padding_end_b;

uint8_t trap_pci[0x1000] __attribute__((aligned(0x1000)));
uint8_t trap_bar[0x1000] __attribute__((aligned(0x1000)));

/* All the physical memory is mapped using identity mapping. */
void ept_create_tables(void) {
  uint64_t i;
  uint64_t j;

  for (i = 0; i < sizeof(trap_pci); i++) {
    trap_pci[i] = 0xff;
    trap_bar[i] = 0xff;
  }

  /* Everything stands into the first 4GB, so we only need the first entry of PML4. */
  ept_tables.PML4[0] = ((uint64_t) &ept_tables.PDPT_PML40[0]) | 0x07;
  for (i = 1; i < 512; i++) {
    ept_tables.PML4[i] = 0;
  }

  /* VMM pages are between bound_a and bound_b */
  uint64_t bound_a = ((uint64_t) &_padding_begin_b) & ~((uint64_t) 0x200000);
  uint64_t bound_b = ((uint64_t) &_padding_end_b) & ~((uint64_t) 0x200000);
  uint64_t Tmp;

  /* Map all memory with 2mb except for the first entry. This first entry is mapped using 4ko pages. */
  uint64_t address = 0;
  const struct memory_range *memory_range = NULL;
  for (i = 0; i < 512; i++) {
    ept_tables.PDPT_PML40[i] = ((uint64_t) &ept_tables.PD_PDPT_PML40[i][0]) | 0x7;
    for (j = 0; j < 512; j++) {
      if (i > 0  || j > 0) {
        if (memory_range == NULL || address < memory_range->range_address_begin || memory_range->range_address_end < address) {
          memory_range = mtrr_get_memory_range(address);
        }
        //if (memory_range->range_address_end < address + 0x200000) {
        if (memory_range == NULL) {
          panic("!#EPT MR2MB [NULL]");
        }
        if (memory_range->range_address_end < address + 0x1fffff) {
          panic("!#EPT MR2MB [?%X<%X<%X:%d]", memory_range->range_address_begin, address, memory_range->range_address_end, memory_range->type);
        }
        ept_tables.PD_PDPT_PML40[i][j] = (((uint64_t) (i * 512 + j)) << 21) | (1 << 7) | 0x7 | (memory_range->type << 3);

        /* FIXME : VMM address range need to be unprotected
         * because his code need to be executed after vmxlaunch */
        Tmp = (((uint64_t) (i * 512 + j)) << 21);
        if (Tmp >= bound_a && Tmp <= bound_b) {
          ept_tables.PD_PDPT_PML40[i][j] = (((uint64_t) (i * 512 + j)) << 21) | (1 << 7) | 0x7 | (memory_range->type << 3);
        }
      }
      address += ((uint64_t) 1 << 21);
    }
  }
  /* Map the first 2mb with 4ko pages. */
  ept_tables.PD_PDPT_PML40[0][0] = ((uint64_t) &ept_tables.PT_PD0[0]) | 0x7;
  address = 0;
  memory_range = NULL;
  for (i = 0; i < 512; i++) {
    if (memory_range == NULL || address < memory_range->range_address_begin || memory_range->range_address_end < address) {
      memory_range = mtrr_get_memory_range(address);
    }
    if (memory_range == NULL) {
      panic("!#EPT MR4KB [NULL]");
    }
    if (memory_range->range_address_end < address + 0xfff) {
      panic("!#EPT MR4KB [?%x<%X<0x%X:%d]", memory_range->range_address_begin, address, memory_range->range_address_end, memory_range->type);
    }
    ept_tables.PT_PD0[i] = ((uint64_t) i << 12) | 0x7 | (memory_range->type << 3);
    address = (((uint64_t) i) << 12);
  }
  // 
  // Network card
  // MMIO pci space protection
  //
#ifdef _DEBUG_SERVER
  uint8_t MMCONFIG_length = (pci_readb(0,0x60) & 0x6) >> 1; // bit 1:2 of PCIEXBAR (offset 60)
  uint16_t MMCONFIG_mask;

  /* MMCONFIG corresponds to bits 38 to 28 of the pci base address
     the length decrease to 27 or 26 the lsb of MMCONFIG */
  switch (MMCONFIG_length) {
    case 0:
      MMCONFIG_mask = 0xF07F;
      break;
    case 1:
      MMCONFIG_mask = 0xF87F;
      break;
    case 2:
      MMCONFIG_mask = 0xFC7F;
      break;
    default:
      panic("Bad MMCONFIG Length\n");
  }

  // bit 38:28-26 of PCIEXBAR (offset 60) -> 14:4-2 of PCIEXBAR + 3
  uint16_t MMCONFIG = pci_readw(0,0x63) & MMCONFIG_mask;

  uint64_t base_addr =  ((uint64_t)MMCONFIG << 16)
                        + PCI_MAKE_MMCONFIG(eth->pci_addr.bus,
                                      eth->pci_addr.device,
                                      eth->pci_addr.function);

  uint32_t PML40_X = base_addr >> 30;
  uint32_t PDPT_PML40_X = (base_addr - (PML40_X << 30)) >> 21;

  /* Map memory associated to eth MMIO configuration with 2mb except for the first entry.
   * This first entry is mapped using 4ko pages. */
  ept_tables.PD_PDPT_PML40[PML40_X][PDPT_PML40_X] = ((uint64_t) &ept_tables.PT_PDX[0]) | 0x7;


  address = base_addr & ~(0x200000 - 1);    // align addr on 2Mo
  memory_range = NULL;
  for (i = 0; i < 512; i++) {
    if (memory_range == NULL || address < memory_range->range_address_begin || memory_range->range_address_end < address) {
      memory_range = mtrr_get_memory_range(address);
    }
    if (memory_range == NULL) {
      panic("!#EPT MR4KB [NULL]");
    }
    if (memory_range->range_address_end < address + 0xfff) {
      panic("!#EPT MR4KB [?%x<%X<0x%X:%d]", memory_range->range_address_begin, address, memory_range->range_address_end, memory_range->type);
    }

    // protect PCI MMIO Access
    if (address == base_addr) {
      ept_tables.PT_PDX[i] = ((uint64_t) &trap_pci[0]) | 0x7 | (memory_range->type << 3);
    } else {
      ept_tables.PT_PDX[i] = (((uint64_t) i << 12) | ((PDPT_PML40_X << 21) + (PML40_X << 30))) | 0x7 | (memory_range->type << 3);
    }
    address += 0x1000;
  }
  // 
  // Network card
  // bar0 space protection
  //
  base_addr = eth->bar0;
  PML40_X = base_addr >> 30;
  PDPT_PML40_X = (base_addr - (PML40_X << 30)) >> 21;
  ept_tables.PD_PDPT_PML40[PML40_X][PDPT_PML40_X] = ((uint64_t) &ept_tables.PT_PDX2[0]) | 0x7;

  address = base_addr & ~(0x200000 - 1);    // align addr on 2Mo
  memory_range = NULL;
  for (i = 0; i < 512; i++) {
    if (memory_range == NULL || address < memory_range->range_address_begin || memory_range->range_address_end < address) {
      memory_range = mtrr_get_memory_range(address);
    }
    if (memory_range == NULL) {
      panic("!#EPT MR4KB [NULL]");
    }
    if (memory_range->range_address_end < address + 0xfff) {
      panic("!#EPT MR4KB [?%x<%X<0x%X:%d]", memory_range->range_address_begin, address, memory_range->range_address_end, memory_range->type);
    }

    if (address == base_addr) {
      ept_tables.PT_PDX2[i] = ((uint64_t) &trap_bar[0]) | 0x7 | (memory_range->type << 3);
    } else {
      ept_tables.PT_PDX2[i] = (((uint64_t) i << 12) | ((PDPT_PML40_X << 21) + (PML40_X << 30))) | 0x7 | (memory_range->type << 3);
    }
    address += 0x1000;
  }
#endif
}

uint64_t ept_get_eptp(void) {
  return ((uint64_t) &ept_tables.PML4[0]) | (3 << 3) | (0x6 << 0);
}
