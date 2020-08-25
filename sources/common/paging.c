#include "paging.h"
#include "stdio.h"
#include "cpuid.h"
#include "efiw.h"

struct paging_ia32e *paging_ia32e;

uint64_t paging_error;

uint8_t max_phyaddr;

// Parameter is in memory allocated pages
void paging_setup_host_paging(struct paging_ia32e *pages) {

  paging_ia32e = pages;

  /* TODO: pcide dans le registre cr4 */
  INFO("SETTING HOST UP PAGING\n");
  uint64_t i;
  uint64_t j;
  uint64_t k;
  uint64_t l;
  for (i = 0; i < 512; i++) {
    pages->PML4[i] = ((uint64_t) &pages->PDPT[i][0]) | 0x7;
    for (j = 0; j < 512; j++) {
      if (i == 0 && j < MAX_ADDRESS_WIDTH_PDPT_1) {
        // 4ko frame : physical memory
        pages->PDPT[i][j] = ((uint64_t) &pages->PD[j][0]) | 0x7;
        for (k = 0; k < 512; k++) {
          pages->PD[j][k] = ((uint64_t) &pages->PT[j][k][0]) | 0x7;
          for (l = 0; l < 512; l++) {
            pages->PT[j][k][l] = ((i * 512 + j) << 30) | (k << 21) | (l << 12) | 0x7;
            /*if (k == 0 && l == 0) {
              INFO("Entry [%d][%d][%d][%d] : 0x%016X\n", i, j, k, l,
                 pages->PT[j][k][l]); 
            }
            if (k == 0 && l == 1) {
              INFO("Entry [%d][%d][%d][%d] : 0x%016X\n", i, j, k, l,
                 pages->PT[j][k][l]); 
            }
            if (k == 1 && l == 0) {
              INFO("Entry [%d][%d][%d][%d] : 0x%016X\n", i, j, k, l,
                 pages->PT[j][k][l]); 
            }*/
          }
        }
      } else {
        // 1GB frame
        pages->PDPT[i][j] = ((i * 512 + j) << 30) | (1 << 7) | 0x7;
        if (j == 0) {
          // INFO("Entry [%d][%d] : 0x%016X\n", i, j, pages->PDPT[i][j]); 
        }
      }
//printk("%08X \n", pages->PDPT[i][j]);
//if (j == 4) {
//while (1);
//}
    }
  }
//while (1);
}

// Parameter is in memory allocated pages
void paging_setup_host_paging_1gb(struct paging_ia32e_1gb *pages) {

  /* TODO: pcide dans le registre cr4 */
  INFO("SETTING HOST UP PAGING\n");
  uint64_t i;
  uint64_t j;
  for (i = 0; i < 512; i++) {
    pages->PML4[i] = ((uint64_t) &pages->PDPT[i][0]) | 0x7;
    for (j = 0; j < 512; j++) {
      // 1GB frame
      pages->PDPT[i][j] = ((i * 512 + j) << 30) | (1 << 7) | 0x7;
    }
  }
}

uint64_t paging_get_host_cr3(void) {
  return (uint64_t) &paging_ia32e->PML4[0];
}

uint8_t paging_get_frame_address(uint64_t entry, uint64_t type, uint64_t *address) {
  switch (type) {
    case PAGING_ENTRY_PDPTE:
      *address = entry & PAGING_PDPTE_FRAME_ADDR;
      return 0;
    case PAGING_ENTRY_PDE:
      *address = entry & PAGING_PDE_FRAME_ADDR;
      return 0;
    case PAGING_ENTRY_PTE:
      *address = entry & PAGING_PTE_FRAME_ADDR;
      return 0;
  }
  return -1;
}

int paging_iterate(uint64_t cr3, int (*cb)(uint64_t *, uint64_t, uint8_t)) {
  uint64_t i; // PML4
  uint64_t j; // PDPT
  uint64_t k; // PD
  uint64_t l; // PT
  uint64_t *e; // entry
  uint64_t a; // address
  uint8_t s; // page size
  uint64_t *pml4;
  uint64_t *pdpt;
  uint64_t *pd;
  uint64_t *pt;

  max_phyaddr = cpuid_get_maxphyaddr();

  INFO("cr3 0x%016X\n", cr3);

  // Initiate the algorithm
  pml4 = (uint64_t *)(cr3 & PAGING_CR3_PLM4_ADDR);
  INFO("@PML4 0x%016X\n", pml4);

  // PML4
  for (i = 0; i < 512; i++) {
    e = &pml4[i];
    // INFO("@PML4E 0x%016X\n", *e);
    if (!(*e & PAGING_PML4E_P)) {
      paging_error = PAGING_WALK_NOT_PRESENT;
      return -1;
    }
    pdpt = (uint64_t *)(*e & PAGING_PML4E_PDPT_ADDR);
    // INFO("@PDPT 0x%016X\n", pdpt);
    // PDPT
    for (j = 0; j < 512; j++) {
      e = &pdpt[j];
      // INFO("@PDPTE 0x%016X\n", *e);
      if (!(*e & PAGING_PDPTE_P)) {
        paging_error = PAGING_WALK_NOT_PRESENT;
        return -1;
      }
      // 1 GB Frame
      if (*e & PAGING_PDPTE_PAGE) {
        a = (*e & PAGING_PDPTE_FRAME_ADDR);
        s = PAGING_ENTRY_PDPTE;
        if (!cb(e, a, s)) {
          return 0;
        }
        continue;
      }
      pd = (uint64_t *)(*e & PAGING_PDPTE_PD_ADDR);
      // INFO("@PD 0x%016X\n", pd);
      // PD
      for (k = 0; k < 512; k++) {
        e = &pd[k];
        // INFO("@PDE 0x%016X\n", *e);
        if (!(*e & PAGING_PDE_P)) {
          paging_error = PAGING_WALK_NOT_PRESENT;
          return -1;
        }
        // 2 MB Frame
        if (*e & PAGING_PDE_PAGE) {
          a = (*e & PAGING_PDE_FRAME_ADDR);
          s = PAGING_ENTRY_PDE;
          if (!cb(e, a, s)) {
            return 0;
          }
          continue;
        }
        pt = (uint64_t *)(*e & PAGING_PDE_PT_ADDR);
        // INFO("@PT 0x%016X\n", pt);
        // PT
        for (l = 0; l < 512; l++) {
          e = &pt[l];
          // INFO("@PTE 0x%016X\n", *e);
          if (!(*e & PAGING_PTE_P)) {
            paging_error = PAGING_WALK_NOT_PRESENT;
            return -1;
          }
          a = (*e & PAGING_PTE_FRAME_ADDR);
          s = PAGING_ENTRY_PTE;
          if (!cb(e, a, s)) {
            return 0;
          }
        }
      }
    }
  }
  return 0;
}

uint8_t walk_is_in_page(uint64_t l, uint64_t p, uint8_t size) {
  uint64_t mask = (1 << size) - 1;
  return (p & ~mask) == (l & ~mask);
}

// DEPRECATED... See next one
int paging_walk(uint64_t cr3, uint64_t linear, uint64_t **e, uint64_t *a, uint8_t *s) {
  max_phyaddr = cpuid_get_maxphyaddr();
  // INFO("Max phy 0x%016x, 0x%016x\n", max_phyaddr, PAGING_MAXPHYADDR(max_phyaddr));
  *e = &cr3;
  // INFO("// Cr3 -> PML4E\n");
  *e = paging_get_pml4e(**e, linear);
  if (!(**e & PAGING_PML4E_P)) {
    paging_error = PAGING_WALK_NOT_PRESENT;
    return -1;
  }
  // INFO("// PML4E -> PDPTE\n");
  *e = paging_get_pdpte(**e, linear);
  if (!(**e & PAGING_PDPTE_P)) {
    paging_error = PAGING_WALK_NOT_PRESENT;
    return -1;
  }
  if (**e & PAGING_PDPTE_PAGE) {
    // INFO("// 1 Go Frame\n");
    *a = (**e & PAGING_PDPTE_FRAME_ADDR) | PAGING_LINEAR_PDPTE_OFFSET(linear);
    *s = PAGING_ENTRY_PDPTE;
    return 0;
  }
  // INFO("// PDPTE -> PDE\n");
  *e = paging_get_pde(**e, linear);
  if (!(**e & PAGING_PDE_P)) {
    paging_error = PAGING_WALK_NOT_PRESENT;
    return -1;
  }
  if (**e & PAGING_PDE_PAGE) {
    // INFO("// 2 Mo Frame\n");
    *a = (**e & PAGING_PDE_FRAME_ADDR) | PAGING_LINEAR_PDE_OFFSET(linear);
    *s = PAGING_ENTRY_PDE;
    return 0;
  }
  // INFO("// PDE -> PTE\n");
  *e = paging_get_pte(**e, linear);
  if (!(**e & PAGING_PTE_P)) {
    paging_error = PAGING_WALK_NOT_PRESENT;
    return -1;
  }
  // INFO("// 4 Ko Frame\n");
  *a = (**e & PAGING_PTE_FRAME_ADDR) | PAGING_LINEAR_PTE_OFFSET(linear);
  *s = PAGING_ENTRY_PTE;
  return 0;
}

int page_walk_long_mode(uint64_t cr0, uint64_t cr3, uint64_t cr4, uint64_t
    ia32_efer, uint64_t vaddr, uint64_t *paddr) {
  uint64_t page;

  INFO("Walking @0x%016X\n", vaddr);

  INFO("IA32_EFER 0x%08x\n", ia32_efer);
  INFO("CR0 0x%08x\n", cr0);
  INFO("CR4 0x%08x\n", cr4);

  // LMA LME
  INFO("IA32_EFER.LMA %d\n", (ia32_efer >> 10) & 1);
  INFO("IA32_EFER.LME %d\n", (ia32_efer >> 8) & 1);

  // PE PG
  INFO("CR0.PG %d\n", (cr0 >> 31) & 1);
  INFO("CR0.PE %d\n", (cr0 >> 0) & 1);

  // PAE LA57
  INFO("CR4.PAE %d\n", (cr4 >> 5) & 1);
  INFO("CR4.LA57 %d\n", (cr4 >> 12) & 1);

  // IF ia32e paging && no 5 level paging (cr4.LA57 == 0)
  if (((ia32_efer >> 10) & 1) && ((cr4 >> 12) & 1) == 0) {
    page = cr3 & ~((long int)0xfff);
    INFO("CR3 0x%016X\n", page);

    __asm__ __volatile__("mov (%%eax, %%ebx, 8), %%rax": "=a"(page) : "a"(page),
        "b"((vaddr >> (12 + 3 * 9) ) & 0x1ff));
    INFO("PML4E 0x%016X\n", page);
    if (!((page >> 0) & 1)) {
      INFO("PML4E not present\n");
      goto fail;
    }
    page = page & ~((long int)0xfff);

    __asm__ __volatile__("mov (%%eax, %%ebx, 8), %%rax": "=a"(page) : "a"(page),
        "b"((vaddr >> (12 + 2 * 9) ) & 0x1ff));
    INFO("PDPTE 0x%016X\n", page);
    if (!((page >> 0) & 1)) {
      INFO("PDPTE not present\n");
      goto fail;
    }
    if ((page >> 7) & 1) {
      *paddr = ((page & ~((long int)0x3fffffff | ((long int)0xfff << 52))) |
          (vaddr & 0x3fffffff));
      INFO("PDPTE 1 GB mapped @0x%016X\n", *paddr);
      goto ok;
    }
    page = page & ~((long int)0xfff);

    __asm__ __volatile__("mov (%%eax, %%ebx, 8), %%rax": "=a"(page) : "a"(page),
        "b"((vaddr >> (12 + 1 * 9) ) & 0x1ff));
    INFO("PDE 0x%016X\n", page);
    if (!((page >> 0) & 1)) {
      INFO("PDE not present\n");
      goto fail;
    }
    if ((page >> 7) & 1) {
      *paddr = ((page & ~((long int)0x1fffff | ((long int)0xfff << 52))) |
          (vaddr & 0x1fffff));
      INFO("PDE 2 MB mapped @0x%016X\n", *paddr);
      goto ok;
    }
    page = page & ~((long int)0xfff);

    __asm__ __volatile__("mov (%%eax, %%ebx, 8), %%rax": "=a"(page) : "a"(page),
        "b"((vaddr >> (12 + 0 * 9) ) & 0x1ff));
    INFO("PTE 0x%016X\n", page);
    if (!((page >> 0) & 1)) {
      INFO("PTE not present\n");
      goto fail;
    }
    *paddr = ((page & ~((long int)0xfff | ((long int)0xfff << 52))) |
        (vaddr & 0xfff));
    INFO("PTE 4kB mapped @0x%016X\n", *paddr);
    goto ok;
  }

fail:
  return 0;

ok:
  return 1;
}
