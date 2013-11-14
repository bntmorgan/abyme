#include "paging.h"
#include "stdio.h"

struct paging_ia32e {
  uint64_t PML4[512]      __attribute__((aligned(0x1000)));
  uint64_t PDPT[512][512] __attribute__((aligned(0x1000)));
} __attribute__((aligned(8)));

struct paging_ia32e paging_ia32e;

uint64_t paging_error;

void paging_setup_host_paging(void) {
  /* TODO: pcide dans le registre cr4 */
  INFO("SETTING HOST UP PAGING\n");
  uint64_t i;
  uint64_t j;
  for (i = 0; i < 512; i++) {
    paging_ia32e.PML4[i] = ((uint64_t) &paging_ia32e.PDPT[i][0]) | 0x7;
    for (j = 0; j < 512; j++) {
      paging_ia32e.PDPT[i][j] = ((i * 512 + j) << 30) | (1 << 7) | 0x7;
//printk("%08X \n", paging_ia32e.PDPT[i][j]);
//if (j == 4) {
//while (1);
//}
    }
  }
//while (1);
}

uint64_t paging_get_host_cr3(void) {
  return (uint64_t) &paging_ia32e.PML4[0];
}

/**
 * Return the entry
 */
uint64_t paging_get_pml4e(uint64_t e, uintptr_t linear) {
  INFO("Entry ADDR 0x%016X linear offset 0x%016X\n", e & PAGING_CR3_PLM4_ADDR,
      PAGING_LINEAR_PML4E(linear));
  return *((uint64_t *)((e & PAGING_CR3_PLM4_ADDR) | PAGING_LINEAR_PML4E(linear)));
}

inline uint64_t paging_get_pdpte(uint64_t e, uintptr_t linear) {
  INFO("Entry ADDR 0x%016X linear offset 0x%016X\n", e & PAGING_PML4E_PDPT_ADDR,
      PAGING_LINEAR_PDPTE(linear));
  return *((uint64_t *)((e & PAGING_PML4E_PDPT_ADDR) | PAGING_LINEAR_PDPTE(linear)));
}

inline uint64_t paging_get_pde(uint64_t e, uintptr_t linear) {
  INFO("Entry ADDR 0x%016X linear offset 0x%016X\n", e & PAGING_PDPTE_PD_ADDR,
      PAGING_LINEAR_PDE(linear));
  return *((uint64_t *)((e & PAGING_PDPTE_PD_ADDR) | PAGING_LINEAR_PDE(linear)));
}


inline uint64_t paging_get_pte(uint64_t e, uintptr_t linear) {
  INFO("Entry ADDR 0x%016X linear offset 0x%016X\n", e & PAGING_PDE_PT_ADDR,
      PAGING_LINEAR_PTE(linear));
  return *((uint64_t *)((e & PAGING_PDE_PT_ADDR) | PAGING_LINEAR_PTE(linear)));
}

int paging_walk(uint64_t cr3, uintptr_t linear, uint64_t *e, uintptr_t *a) {
  *e = cr3;
  INFO("Entry : 0x%016X\n", *e);
  INFO("Cr3 -> PML4E\n");
  *e = paging_get_pml4e(*e, linear);
  INFO("Entry : 0x%016X\n", *e);
  if (!(*e & PAGING_PML4E_P)) {
    paging_error = PAGING_WALK_NOT_PRESENT;
    return -1;
  }
  INFO("PML4E -> PDPTE\n");
  *e = paging_get_pdpte(*e, linear);
  INFO("Entry : 0x%016X\n", *e);
  if (!(*e & PAGING_PDPTE_P)) {
    paging_error = PAGING_WALK_NOT_PRESENT;
    return -1;
  }
  if (*e & PAGING_PDPTE_PAGE) {
    INFO("1 Go Frame\n");
    *a = (*e & PAGING_PDPTE_FRAME_ADDR) | PAGING_LINEAR_PDPTE_OFFSET(linear);
    return 0;
  }
  INFO("PDPTE -> PDE\n");
  *e = paging_get_pde(*e, linear);
  INFO("Entry : 0x%016X\n", *e);
  if (!(*e & PAGING_PDE_P)) {
    paging_error = PAGING_WALK_NOT_PRESENT;
    return -1;
  }
  if (*e & PAGING_PDE_PAGE) {
    INFO("2 Mo Frame\n");
    *a = (*e & PAGING_PDE_FRAME_ADDR) | PAGING_LINEAR_PDE_OFFSET(linear);
    return 0;
  }
  INFO("PDE -> PTE\n");
  *e = paging_get_pte(*e, linear);
  INFO("Entry : 0x%016X\n", *e);
  if (!(*e & PAGING_PTE_P)) {
    paging_error = PAGING_WALK_NOT_PRESENT;
    return -1;
  }
  INFO("4 Ko Frame\n");
  *a = (*e & PAGING_PTE_FRAME_ADDR) | PAGING_LINEAR_PTE_OFFSET(linear);
  return 0;
}
