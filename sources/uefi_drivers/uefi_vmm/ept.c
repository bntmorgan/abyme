#include "ept.h"

// TODO: pour inclure stdio dans ept? => trouver une autre solution?
#include "stdio.h"

#include "mtrr.h"

struct ept_tables {
  uint64_t PML4[512]               __attribute__((aligned(0x1000)));
  uint64_t PDPT_PML40[512]         __attribute__((aligned(0x1000)));
  uint64_t PD_PDPT_PML40[512][512] __attribute__((aligned(0x1000)));
  uint64_t PT_PD0[512]             __attribute__((aligned(0x1000)));
} __attribute__((aligned(8)));

struct ept_tables ept_tables;

/* All the physical memory is mapped using identity mapping. */
void ept_create_tables(void) {
  uint64_t i;
  uint64_t j;
  /* Everything stands into the first 4GB, so we only need the first entry of PML4. */
  ept_tables.PML4[0] = ((uint64_t) &ept_tables.PDPT_PML40[0]) | 0x07;
  for (i = 1; i < 512; i++) {
    ept_tables.PML4[i] = 0;
  }
  /* Map all memory with 2mb pages but the first entry. This first entry is mapped using 4ko pages. */
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
      }
      address += ((uint64_t) 1 << 21);
    }
  }
//while (1);
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
}

uint64_t ept_get_eptp(void) {
  return ((uint64_t) &ept_tables.PML4[0]) | (3 << 3) | (0x6 << 0);
}
