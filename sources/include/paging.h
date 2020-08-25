#ifndef __PAGING_H__
#define __PAGING_H__

#include <efi.h>
#include "types.h"

#define MAX_ADDRESS_WIDTH_PDPT_1 (((uint64_t)1 << 36) >> 30)

struct paging_ia32e {
  uint64_t PML4[512]      __attribute__((aligned(0x1000)));
  uint64_t PDPT[512][512] __attribute__((aligned(0x1000)));
  uint64_t PD[MAX_ADDRESS_WIDTH_PDPT_1][512] __attribute__((aligned(0x1000)));
  uint64_t PT[MAX_ADDRESS_WIDTH_PDPT_1][512][512]__attribute__((aligned(0x1000)));
} __attribute__((aligned(8)));

struct paging_ia32e_1gb {
  uint64_t PML4[512]      __attribute__((aligned(0x1000)));
  uint64_t PDPT[512][512] __attribute__((aligned(0x1000)));
} __attribute__((aligned(8)));

extern struct paging_ia32e *paging_ia32e;

void paging_setup_host_paging(struct paging_ia32e *pages);

void paging_setup_host_paging_1gb(struct paging_ia32e_1gb *pages);

uint64_t paging_get_host_cr3(void);

extern uint64_t paging_error;

extern uint8_t max_phyaddr;

int paging_iterate(uint64_t cr3, int (*cb)(uint64_t *, uint64_t, uint8_t));

/**
 * This function is computing physical address from a linear one
 * It is returning the location pointer to the entry referecing the frame,
 * the type of the frame and of course, the physical address.
 */
int paging_walk(uint64_t cr3, uint64_t linear, uint64_t **entry, uint64_t *address, uint8_t *type);
// Other safe implementation of long mode paging walk
int page_walk_long_mode(uint64_t cr0, uint64_t cr3, uint64_t cr4, uint64_t
    ia32_efer, uint64_t vaddr, uint64_t *paddr);

/**
 * Return the frame address according to the entry type
 */
uint8_t paging_get_frame_address(uint64_t entry, uint64_t type, uint64_t *address);

#define PAGING_MAXPHYADDR(x)      (((uint64_t)1 << x) - 1) // 36 bit max phy addr

//
// Errors
//

// Pagewalk
#define PAGING_WALK_NOT_PRESENT   -1

//
// Paging masks
// And Macros
//

// Page sizes
#define PAGING_ENTRY_PDPTE       30
#define PAGING_ENTRY_PDE         21
#define PAGING_ENTRY_PTE         12

// Linear address
#define PAGING_LINEAR_PML4E(x)          ((x >> 39) & 0x1ff)
#define PAGING_LINEAR_PDPTE(x)          ((x >> 30) & 0x1ff)
#define PAGING_LINEAR_PDE(x)            ((x >> 21) & 0x1ff)
#define PAGING_LINEAR_PTE(x)            ((x >> 12) & 0x1ff)
#define PAGING_LINEAR_PDPTE_OFFSET(x)   ((x >>  0) & 0x3fffffff)
#define PAGING_LINEAR_PDE_OFFSET(x)     ((x >>  0) & 0x1fffff)
#define PAGING_LINEAR_PTE_OFFSET(x)     ((x >>  0) & 0xfff)

// CR3
#define PAGING_CR3_PWT            (1 <<  3)
#define PAGING_CR3_PCD            (1 <<  4)
#define PAGING_CR3_PLM4_ADDR      (PAGING_MAXPHYADDR(max_phyaddr) & ~0xfff)

// PML4E
#define PAGING_PML4E_P            (1 <<  0)
#define PAGING_PML4E_RW           (1 <<  1)
#define PAGING_PML4E_US           (1 <<  2)
#define PAGING_PML4E_PWT          (1 <<  3)
#define PAGING_PML4E_PCD          (1 <<  4)
#define PAGING_PML4E_A            (1 <<  5)
#define PAGING_PML4E_PDPT_ADDR    (PAGING_MAXPHYADDR(max_phyaddr) & ~0xfff)
#define PAGING_PML4E_XD           (1 << 63)

// PDPTE
#define PAGING_PDPTE_P            (1 <<  0)
#define PAGING_PDPTE_RW           (1 <<  1)
#define PAGING_PDPTE_US           (1 <<  2)
#define PAGING_PDPTE_PWT          (1 <<  3)
#define PAGING_PDPTE_PCD          (1 <<  4)
#define PAGING_PDPTE_A            (1 <<  5)
#define PAGING_PDPTE_D            (1 <<  6)
#define PAGING_PDPTE_PAGE         (1 <<  7)
#define PAGING_PDPTE_G            (1 <<  8)
#define PAGING_PDPTE_PAT          (1 << 12)
#define PAGING_PDPTE_PD_ADDR      (PAGING_MAXPHYADDR(max_phyaddr) & ~0xfff)
#define PAGING_PDPTE_FRAME_ADDR   (PAGING_MAXPHYADDR(max_phyaddr) & ~0x3fffffff) // 1Go
#define PAGING_PDPTE_XD           (1 << 63)

// PDE
#define PAGING_PDE_P              (1 <<  0)
#define PAGING_PDE_RW             (1 <<  1)
#define PAGING_PDE_US             (1 <<  2)
#define PAGING_PDE_PWT            (1 <<  3)
#define PAGING_PDE_PCD            (1 <<  4)
#define PAGING_PDE_A              (1 <<  5)
#define PAGING_PDE_D              (1 <<  6)
#define PAGING_PDE_PAGE           (1 <<  7)
#define PAGING_PDE_G              (1 <<  8)
#define PAGING_PDE_PAT            (1 << 12)
#define PAGING_PDE_PT_ADDR        (PAGING_MAXPHYADDR(max_phyaddr) & ~0xfff)
#define PAGING_PDE_FRAME_ADDR     (PAGING_MAXPHYADDR(max_phyaddr) & ~0x1fffff) // 2Mo

// PTE
#define PAGING_PTE_P              (1 << 0)
#define PAGING_PTE_RW             (1 << 1)
#define PAGING_PTE_US             (1 << 2)
#define PAGING_PTE_PWT            (1 << 3)
#define PAGING_PTE_PCD            (1 << 4)
#define PAGING_PTE_A              (1 << 5)
#define PAGING_PTE_D              (1 << 6)
#define PAGING_PTE_PAT            (1 << 7)
#define PAGING_PTE_G              (1 << 8)
#define PAGING_PTE_FRAME_ADDR     (PAGING_MAXPHYADDR(max_phyaddr) & ~0xfff) // 4ko

/**
 * Return the entry
 */
static inline uint64_t *paging_get_pml4e(uint64_t e, uint64_t linear) {
  return ((uint64_t *)(e & PAGING_CR3_PLM4_ADDR)) + PAGING_LINEAR_PML4E(linear);
}

static inline uint64_t *paging_get_pdpte(uint64_t e, uint64_t linear) {
  return ((uint64_t *)(e & PAGING_PML4E_PDPT_ADDR)) + PAGING_LINEAR_PDPTE(linear);
}

static inline uint64_t *paging_get_pde(uint64_t e, uint64_t linear) {
  return ((uint64_t *)(e & PAGING_PDPTE_PD_ADDR)) + PAGING_LINEAR_PDE(linear);
}


static inline uint64_t *paging_get_pte(uint64_t e, uint64_t linear) {
  return ((uint64_t *)(e & PAGING_PDE_PT_ADDR)) + PAGING_LINEAR_PTE(linear);
}

uint8_t walk_is_in_page(uint64_t l, uint64_t p, uint8_t size);

#endif
