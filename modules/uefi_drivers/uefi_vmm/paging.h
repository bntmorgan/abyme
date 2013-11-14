#ifndef __PAGING_H__
#define __PAGING_H__

#include <efi.h>
#include "types.h"

void paging_setup_host_paging(void);

uint64_t paging_get_host_cr3(void);

#define PAGING_MAXPHYADDR 0xfffffffff // 36 bit max phy addr

//
// Paging masks
//

// CR3
#define PAGING_CR3_PWT            (1 << 3)
#define PAGING_CR3_PCD            (1 << 4)
#define PAGING_CR3_PLM4_ADDR      (PAGING_MAXPHYADDR & ~0xfff)

// PML4E
#define PAGING_PML4E_P            (1 << 0)
#define PAGING_PML4E_RW           (1 << 1)
#define PAGING_PML4E_US           (1 << 2)
#define PAGING_PML4E_PWT          (1 << 3)
#define PAGING_PML4E_PCD          (1 << 4)
#define PAGING_PML4E_A            (1 << 5)
#define PAGING_PML4E_PDPT_ADDR    (PAGING_MAXPHYADDR & ~0xfff)
#define PAGING_PML4E_XD           (1 << 63)

// PDPTE
#define PAGING_PDPTE_P            (1 << 0)
#define PAGING_PDPTE_RW           (1 << 1)
#define PAGING_PDPTE_US           (1 << 2)
#define PAGING_PDPTE_PWT          (1 << 3)
#define PAGING_PDPTE_PCD          (1 << 4)
#define PAGING_PDPTE_A            (1 << 5)
#define PAGING_PDPTE_D            (1 << 6)
#define PAGING_PDPTE_PAGE         (1 << 7)
#define PAGING_PDPTE_G            (1 << 8)
#define PAGING_PDPTE_PAT          (1 << 12)
#define PAGING_PDPTE_PD_ADDR      (PAGING_MAXPHYADDR & ~0xfff)
#define PAGING_PDPTE_FRAME_ADDR   (PAGING_MAXPHYADDR & ~0x1fffffff)
#define PAGING_PDPTE_XD           (1 << 63)

// PDE
#define PAGING_PDE_P              (1 << 0)
#define PAGING_PDE_RW             (1 << 1)
#define PAGING_PDE_US             (1 << 2)
#define PAGING_PDE_PWT            (1 << 3)
#define PAGING_PDE_PCD            (1 << 4)
#define PAGING_PDE_A              (1 << 5)
#define PAGING_PDE_D              (1 << 6)
#define PAGING_PDE_PAGE           (1 << 7)
#define PAGING_PDE_G              (1 << 8)
#define PAGING_PDE_PAT            (1 << 12)
#define PAGING_PDE_PT_ADDR        (PAGING_MAXPHYADDR & ~0xfff)
#define PAGING_PDE_FRAME_ADDR     (PAGING_MAXPHYADDR & ~0xfffff)

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
#define PAGING_PTE_FRAME_ADDR     (PAGING_MAXPHYADDR & ~0xfff)

#endif
