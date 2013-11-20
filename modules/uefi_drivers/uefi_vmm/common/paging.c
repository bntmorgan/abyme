#include "paging.h"
#include "stdio.h"
#include "cpuid.h"

struct paging_ia32e *paging_ia32e;

uint64_t paging_error;

uint8_t max_phyaddr;

void paging_setup_host_paging(void) {
  /* TODO: pcide dans le registre cr4 */
  INFO("SETTING HOST UP PAGING\n");
  uint64_t i;
  uint64_t j;
  uint64_t k;
  uint64_t l;
  for (i = 0; i < 512; i++) {
    paging_ia32e->PML4[i] = ((uint64_t) &paging_ia32e->PDPT[i][0]) | 0x7;
    for (j = 0; j < 512; j++) {
      if (j > MAX_ADDRESS_WIDTH_PDPT_1) {
        // 1GB frame
        paging_ia32e->PDPT[i][j] = ((i * 512 + j) << 30) | (1 << 7) | 0x7;
      } else {
        paging_ia32e->PDPT[i][j] = ((uint64_t) &paging_ia32e->PD[j][0]) | 0x7;
        for (k = 0; i < 512; i++) {
          paging_ia32e->PD[j][k] = ((uint64_t) &paging_ia32e->PT[j][k][0]) | 0x7;
          for (l = 0; i < 512; i++) {
            // 4ko frame : physical memory
            paging_ia32e->PT[j][k][l] = ((i * 512 + j) << 30) | (k << 21) | (l << 10) | 0x7;
          }
        }
      }
//printk("%08X \n", paging_ia32e->PDPT[i][j]);
//if (j == 4) {
//while (1);
//}
    }
  }
//while (1);
}

uint64_t paging_get_host_cr3(void) {
  return (uint64_t) &paging_ia32e->PML4[0];
}

/**
 * Return the entry
 */
uint64_t *paging_get_pml4e(uint64_t e, uint64_t linear) {
  return ((uint64_t *)(e & PAGING_CR3_PLM4_ADDR)) + PAGING_LINEAR_PML4E(linear);
}

inline uint64_t *paging_get_pdpte(uint64_t e, uint64_t linear) {
  return ((uint64_t *)(e & PAGING_PML4E_PDPT_ADDR)) + PAGING_LINEAR_PDPTE(linear);
}

inline uint64_t *paging_get_pde(uint64_t e, uint64_t linear) {
  return ((uint64_t *)(e & PAGING_PDPTE_PD_ADDR)) + PAGING_LINEAR_PDE(linear);
}


inline uint64_t *paging_get_pte(uint64_t e, uint64_t linear) {
  return ((uint64_t *)(e & PAGING_PDE_PT_ADDR)) + PAGING_LINEAR_PTE(linear);
}

int paging_walk(uint64_t cr3, uint64_t linear, uint64_t **e, uint64_t *a, uint64_t *s) {
  max_phyaddr = cpuid_get_maxphyaddr();
  INFO("Max phy 0x%016x, 0x%016x\n", max_phyaddr, PAGING_MAXPHYADDR(max_phyaddr));
  *e = &cr3;
  INFO("// Cr3 -> PML4E\n");
  *e = paging_get_pml4e(**e, linear);
  if (!(**e & PAGING_PML4E_P)) {
    paging_error = PAGING_WALK_NOT_PRESENT;
    return -1;
  }
  INFO("// PML4E -> PDPTE\n");
  *e = paging_get_pdpte(**e, linear);
  if (!(**e & PAGING_PDPTE_P)) {
    paging_error = PAGING_WALK_NOT_PRESENT;
    return -1;
  }
  if (**e & PAGING_PDPTE_PAGE) {
    INFO("// 1 Go Frame\n");
    *a = (**e & PAGING_PDPTE_FRAME_ADDR) | PAGING_LINEAR_PDPTE_OFFSET(linear);
    *s = PAGING_ENTRY_PDPTE;
    return 0;
  }
  INFO("// PDPTE -> PDE\n");
  *e = paging_get_pde(**e, linear);
  if (!(**e & PAGING_PDE_P)) {
    paging_error = PAGING_WALK_NOT_PRESENT;
    return -1;
  }
  if (**e & PAGING_PDE_PAGE) {
    INFO("// 2 Mo Frame\n");
    *a = (**e & PAGING_PDE_FRAME_ADDR) | PAGING_LINEAR_PDE_OFFSET(linear);
    *s = PAGING_ENTRY_PDE;
    return 0;
  }
  INFO("// PDE -> PTE\n");
  *e = paging_get_pte(**e, linear);
  if (!(**e & PAGING_PTE_P)) {
    paging_error = PAGING_WALK_NOT_PRESENT;
    return -1;
  }
  INFO("// 4 Ko Frame\n");
  *a = (**e & PAGING_PTE_FRAME_ADDR) | PAGING_LINEAR_PTE_OFFSET(linear);
  *s = PAGING_ENTRY_PTE;
  return 0;
}
