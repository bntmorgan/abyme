/*
Copyright (C) 2021  Benoît Morgan

This file is part of abyme

abyme is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

abyme is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with abyme.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ept.h"

// TODO: pour inclure stdio dans ept? => trouver une autre solution?
#include "stdio.h"

#include "mtrr.h"
#include "debug_server/debug_server.h"
#include "pci.h"
#include "cpuid.h"
#include "paging.h"
#include "efiw.h"
#include "msr.h"
#include "vmm.h"
#include "error.h"

struct ept_tables {
  uint64_t PML4[512]                                 __attribute__((aligned(0x1000)));
  // 1 GB mapped over 512 giga bytes
  uint64_t PML4_PDPT[512][512]                       __attribute__((aligned(0x1000)));
  // 2 MB mapped over 4 giga bytes
  uint64_t PML40_PDPT_PD[512 - EPTN][512]            __attribute__((aligned(0x1000)));
  // 4 KB mapped under EPTN giga bytes
  // VM_NB number of contexts under EPTN GB
  uint64_t PML40_PDPT0_N_PD[VM_NB][EPTN][512]         __attribute__((aligned(0x1000)));
  uint64_t PML40_PDPT0_N_PD_PT[VM_NB][EPTN][512][512] __attribute__((aligned(0x1000)));
} __attribute__((aligned(8)));


inline static uint16_t get_PML4_offset(uint64_t addr);
inline static uint16_t get_PDPT_offset(uint64_t addr);
inline static uint16_t get_PD_offset(uint64_t addr);
inline static uint16_t get_PT_offset(uint64_t addr);
static struct ept_tables *ept_tables;

uint8_t *trap_pci;
uint8_t *trap_bar;

static const struct memory_range *memory_range;

static uint8_t ctx = 0;

uint8_t ept_get_ctx(void) {
  return ctx;
}

inline static void set_memory_type(uint64_t *entry, uint64_t address,
    uint64_t page_size, uint8_t max_phyaddr) {
  uint64_t max_address = ((uint64_t)1 << max_phyaddr);
  if (address > memory_range->range_address_end) {
    memory_range = mtrr_get_memory_range(address);
  }
  if (address < max_address) {
    if (memory_range == NULL) {
      panic("!#EPT MR4KB [NULL], address=%016X, page_size=%016X\n", address,
          page_size);
    }
    if (address + page_size-1 > memory_range->range_address_end) {
      panic("!#EPT MR4KB [?%x<0x%X<0x%X:%d]\n",
          memory_range->range_address_begin, address,
          memory_range->range_address_end, memory_range->type);
    }
    // XXX faux
    *entry |= (memory_range->type << 3);
  } else {
    // XXX faux
    *entry |= (0 << 3);
  }
}

void ept_remap(uint64_t virt, uint64_t phy, uint8_t rights, uint8_t c) {
  uint16_t PML4_offset = get_PML4_offset(virt);
  uint16_t PDPT_offset = get_PDPT_offset(virt);
  uint16_t PD_offset = get_PD_offset(virt);
  uint16_t PT_offset = get_PT_offset(virt);

  if (PML4_offset > 0) {
    panic("!#EPT protected_zone doesn't fit in the first PML4 entry\n", EPTN);
  }

  if (PDPT_offset > EPTN) {
    panic("!#EPT protected_zone doesn't fit in %d PDPT entries\n", EPTN);
  }

  INFO("ctx(0x%02x), PDPT(%d), PD(%d), PT(%d), virt(@0x%016X) => "
      "phy(@0x%016X)\n", c, PDPT_offset, PD_offset, PT_offset, virt, phy);
  // XXX Save the cache level...
  ept_tables->PML40_PDPT0_N_PD_PT[c][PDPT_offset][PD_offset][PT_offset] = phy |
    (rights & 0x7);
}

/**
 * Juste relink the pages the TLB are handled via VPIDs
 * by default ctx is zero, set by create tables
 */
void ept_set_ctx(uint8_t c) {
  uint64_t j; // PTPT
  ctx = c;
  if (c >= VM_NB) {
    ERROR_N_REBOOT("Invalid context number\n");
  }
  for (j = 0; j < EPTN; j++) {
    ept_tables->PML4_PDPT[0][j] =
      ((uint64_t) &ept_tables->PML40_PDPT0_N_PD[ctx][j][0]) | 0x07;
  }
  // DBG("CTX 0x%x set\n", ctx);
}

void ept_perm(uint64_t address, uint32_t pages, uint8_t rights, uint8_t c) {
  uint64_t p_begin = address;
  uint64_t p_end = address + (pages * 0x1000);

  uint64_t j; // PDPT
  uint64_t k; // PD
  uint64_t l; // PT

  if (pages == 0) {
    panic("!#EPT wrong page number\n");
  }

  uint16_t PML4_offset_end = get_PML4_offset(p_end);
  uint16_t PDPT_offset_begin = get_PDPT_offset(p_begin);
  uint16_t PDPT_offset_end = get_PDPT_offset(p_end);
  uint16_t PD_offset_begin = get_PD_offset(p_begin);
  uint16_t PT_offset_begin = get_PT_offset(p_begin);

  INFO("ctx(0x%02x), virt(@0x%016X), rights(0x%02x), pages(0x%02x)\n", c,
      address, rights, pages);

  if (PML4_offset_end > 0) {
    panic("!#EPT protected_zone doesn't fit in the first PML4 entry\n", EPTN);
  }

  if (PDPT_offset_end >= EPTN) {
    panic("!#EPT protected_zone doesn't fit in %d PDPT entries\n", EPTN);
  }

  uint64_t a = address & (~(uint64_t)0xfff);
  // INFO("begin a 0x%016X, p_end 0x%016X\n", a, p_end);

  k = PD_offset_begin;
  l = PT_offset_begin;
  for (j = PDPT_offset_begin; j < EPTN; j++) {
    for (; k <= 512; k++) {
      for (; l < 512; l++, a += ((uint64_t)1 << 12)) {
        if (a < p_end) {
          ept_tables->PML40_PDPT0_N_PD_PT[c][j][k][l] &= (uint64_t)~0x7;
          ept_tables->PML40_PDPT0_N_PD_PT[c][j][k][l] |= rights & 0x7;
        } else {
          return;
        }
      }
      l = 0;
    }
    k = 0;
  }
}

void ept_cache(void) {
  uint64_t i; // MPL4
  uint64_t j; // PDPT
  uint64_t k; // PD
  uint64_t l; // PT
  uint64_t m; // context
  uint8_t max_phyaddr = cpuid_get_maxphyaddr(); // Physical address space
  uint64_t address; // Current address

  // Display physical address bus width
  INFO("Phy address bus width : %d\n", max_phyaddr);

  // Init ranges
  memory_range = mtrr_get_memory_range(0);

  /* ID Map all virtual memory taking into account MTRR and Hypervisor code */
  for (i = 0; i < 512; i++) {
    for (j = 0; j < 512; j++) {
      if (i == 0) {
        for (k = 0; k < 512; k++) {
          if (j < EPTN) {
            for (l = 0; l < 512; l++) {
              address = (j << 30) | (k << 21) | (l << 12);
              for (m = 0; m < VM_NB; m++) {
                set_memory_type(&ept_tables->PML40_PDPT0_N_PD_PT[m][j][k][l],
                    address, 0x1000, max_phyaddr);
              }
            }
          } else {
            address = (j << 30) | (k << 21);
            set_memory_type(&ept_tables->PML40_PDPT_PD[j - EPTN][k], address,
                0x200000, max_phyaddr);
          }
        }
      } else {
        // Over 512 giga bytes : 1 GB mapped
        address = (i << 39) | (j << 30);
        set_memory_type(&ept_tables->PML4_PDPT[i][j], address, 0x40000000,
            max_phyaddr);
      }
    }
  }
}

/* All the physical memory is mapped using identity mapping. */
void ept_create_tables(uint64_t protected_begin, uint64_t protected_end) {
  uint64_t i; // MPL4
  uint64_t j; // PDPT
  uint64_t k; // PD
  uint64_t l; // PT
  uint64_t m; // context
  uint64_t address; // Current address

  // Check if EPT is available
  uint64_t msr = msr_read(MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS2);
  if ((msr & ((uint64_t)1 << (1 + 32))) == 0) {
    ERROR_N_REBOOT("EPT is not available on this platform\n");
  }

  ept_tables = efi_allocate_pages(sizeof(struct ept_tables) >> 12);
  trap_pci = efi_allocate_pages(1);
  trap_bar = efi_allocate_pages(1);

  uint64_t max_addr = ((uint64_t)1 << cpuid_get_maxphyaddr());

  /* ID Map all virtual memory with rwx rights */
  for (i = 0; i < 512; i++) {
    ept_tables->PML4[i] = ((uint64_t) &ept_tables->PML4_PDPT[i][0]) | 0x07;
    for (j = 0; j < 512; j++) {
      if (i == 0) {
        // Under 512 giga bytes : 2 MB mapped
        if (j < EPTN) {
          // Under 4 gigabytes first context PD
          ept_tables->PML4_PDPT[i][j] =
            ((uint64_t) &ept_tables->PML40_PDPT0_N_PD[0][j][0]) | 0x07;
        } else {
          // Under 512 giga bytes : 2 MB mapped
          ept_tables->PML4_PDPT[i][j] =
            ((uint64_t) &ept_tables->PML40_PDPT_PD[j - EPTN][0]) | 0x07;
        }
        for (k = 0; k < 512; k++) {
          if (j < EPTN) {
            // Under EPTN giga bytes : 4 kB mapped
            // Link context 0
            for (m = 0; m < VM_NB; m++) {
              ept_tables->PML40_PDPT0_N_PD[m][j][k] = ((uint64_t)
                  &ept_tables->PML40_PDPT0_N_PD_PT[m][j][k][0]) | 0x07;
              for (l = 0; l < 512; l++) {
                address = (j << 30) | (k << 21) | (l << 12);
                // Map 4kB pages for all contexts
                ept_tables->PML40_PDPT0_N_PD_PT[m][j][k][l] = address | 0x07;
              }
            }
          } else {
            address = (j << 30) | (k << 21);
            ept_tables->PML40_PDPT_PD[j - EPTN][k] = address | (1 << 7) | 0x07;
          }
        }
      } else {
        // Over 512 giga bytes : 1 GB mapped
        address = (i << 39) | (j << 30);
        // We don't map over the physical address space
        if (address >= max_addr) {
          ept_tables->PML4_PDPT[i][j] = 0;
        } else {
          ept_tables->PML4_PDPT[i][j] = address | (1 << 7) | 0x07;
        }
      }
    }
  }

  ept_check_mapping();
  INFO("EPT CHECK MAPPING DONE\n");

  //
  // Protect VMM pages : we split the first and the last 2Mo pages corresponding
  // to VMM memory into 4Ko pages
  //

  /* Already aligned on 4Ko (cf efi.ld) */
  uint64_t p_begin = protected_begin;
  uint64_t p_end = protected_end;

  // Protect the VMM memory space
  // Map 4kB pages for all contexts
  for (m = 0; m < VM_NB; m++) {
    // XXX firware is reading our image to build physical memory space asked by
    // linux @see RuntimeDriverSetVirtualAddressMap
    ept_perm(p_begin, (p_end - p_begin) >> 12, 0x1, m);
  }

  // Compute the cache policy thanks to the mtrrs ranges
  ept_cache();

  //
  // Network card
  // MMIO pci space protection
  //
#ifdef _DEBUG_SERVER
  if (debug_server) {
    uint64_t MMCONFIG_base;
    uint64_t base_addr;

    /* Initialize traps with 0xff */
    for (i = 0; i < sizeof(trap_pci); i++) {
      trap_pci[i] = 0xff;
      trap_bar[i] = 0xff;
    }

    MMCONFIG_base = pci_mmconfig_base();

    INFO("MMCONFIG : base(@0x%016X)\n", MMCONFIG_base);
    base_addr = MMCONFIG_base + PCI_MAKE_MMCONFIG(eth->pci_addr.bus,
        eth->pci_addr.device, eth->pci_addr.function);

    INFO("MMIO NIC config space(@0x%016X)\n", base_addr);

    // XXX For the moment the VMs can use the network card with the UEFI driver
    // for debug purpose
    for (m = 0; m < VM_NB; m++) {
      ept_remap(base_addr, (uint64_t)&trap_pci[0], 0x0, m);
      // XXX We only hide bar0 which is not use anymore by the network driver
      ept_remap(eth->bar0, (uint64_t)&trap_bar[0], 0x7, m);
      // XXX TODO here we should protect dynamically allocate pages
      // * page tables
      // * Ethernet buffers
      // * Ethernet memory space
    }
  }
#endif
}

int ept_walk(uint64_t cr3, uint64_t linear, uint64_t **e, uint64_t *a, uint8_t *s) {
  uint64_t max_phyaddr = cpuid_get_maxphyaddr();
  // INFO("Max phy 0x%016x, 0x%016x\n", max_phyaddr, PAGING_MAXPHYADDR(max_phyaddr));
  *e = &cr3;
  // INFO("// Cr3 -> PML4E\n");
  *e = paging_get_pml4e(**e, linear);
  if (!(**e & EPT_PML4E_P)) {
    paging_error = PAGING_WALK_NOT_PRESENT;
    return -1;
  }
  // INFO("// PML4E -> PDPTE\n");
  *e = paging_get_pdpte(**e, linear);
  if (!(**e & EPT_PDPTE_P)) {
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
  if (!(**e & EPT_PDE_P)) {
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
  if (!(**e & EPT_PTE_P)) {
    paging_error = PAGING_WALK_NOT_PRESENT;
    return -1;
  }
  // INFO("// 4 Ko Frame\n");
  *a = (**e & PAGING_PTE_FRAME_ADDR) | PAGING_LINEAR_PTE_OFFSET(linear);
  *s = PAGING_ENTRY_PTE;
  return 0;
}

int ept_iterate(uint64_t eptp, int (*cb)(uint64_t *, uint64_t, uint8_t)) {
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

  uint64_t max_phyaddr = cpuid_get_maxphyaddr();
  INFO("Max phy 0x%016x, 0x%016x\n", max_phyaddr, PAGING_MAXPHYADDR(max_phyaddr));

  INFO("eptp 0x%016X\n", eptp);

  // Initiate the algorithm
  pml4 = (uint64_t *)(eptp & PAGING_CR3_PLM4_ADDR);
  //INFO("@PML4 0x%016X\n", pml4);

  // PML4
  for (i = 0; i < 512; i++) {
    e = &pml4[i];
    //INFO("@PML4E 0x%016X\n", *e);
    if (!(*e & EPT_PML4E_P)) {
      paging_error = PAGING_WALK_NOT_PRESENT;
      return -1;
    }
    pdpt = (uint64_t *)(*e & PAGING_PML4E_PDPT_ADDR);
    //INFO("@PDPT 0x%016X\n", pdpt);
    // PDPT
    for (j = 0; j < 512; j++) {
	    e = &pdpt[j];
      //INFO("PDPTE 0x%016X\n", *e);
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
      //INFO("@PD 0x%016X\n", pd);
      // PD
      for (k = 0; k < 512; k++) {
        e = &pd[k];
        // INFO("PDE 0x%016X\n", *e);
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
        //INFO("@PT 0x%016X\n", pt);
        // PT
        for (l = 0; l < 512; l++) {
          e = &pt[l];
          //INFO("PTE 0x%016X\n", *e);
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

// XXX
// Private callback for chunk detection
static uint64_t pa = 0, ta = 0;
int cb(uint64_t *e, uint64_t a, uint8_t s) {
  uint8_t max_phyaddr = cpuid_get_maxphyaddr();
  if (a == ta) {
    pa = a;
  } else {
    INFO("a(0x%016X), s(0x%x), address(0x%016X), ta(0x%016X)\n", a, s, pa, ta);
    ERROR_N_REBOOT("id mapping error yolord\n");
  }
  switch (s) {
    case PAGING_ENTRY_PTE:
      ta = pa + 0x1000;
      break;
    case PAGING_ENTRY_PDE:
      ta = pa + 0x200000;
      break;
    case PAGING_ENTRY_PDPTE:
      ta = pa + 0x40000000;
      break;
    default:
      ERROR_N_REBOOT("BAD page size\n");
  }
  if (ta == ((uint64_t)1 << max_phyaddr)) {
    INFO("CHECK END maxphyaddr reached\n");
    return 0;
  } else {
    return 1;
  }
}

void ept_check_mapping(void) {
  uint64_t eptp = ept_get_eptp();
  INFO("EPTP 0x%016X\n", eptp);
  if (ept_iterate(eptp, &cb)) {
    ERROR_N_REBOOT("PAGING error... 0x%x\n", paging_error);
  }
}

uint64_t ept_get_eptp(void) {
  return ((uint64_t) &ept_tables->PML4[0]) | (3 << 3) | (0x6 << 0);
}

inline static uint16_t get_PML4_offset(uint64_t addr)
{
  return ((addr >> 39) & 0x1FF);
}

inline static uint16_t get_PDPT_offset(uint64_t addr)
{
  return ((addr >> 30) & 0x1FF);
}

inline static uint16_t get_PD_offset(uint64_t addr)
{
  return ((addr >> 21) & 0x1FF);
}

inline static uint16_t get_PT_offset(uint64_t addr)
{
  return ((addr >> 12) & 0x1FF);
}

// XXX TODO mieux que ça bordel
void ept_inv_tlb(void) {
  uint64_t desc[2] = {
    0x0000000000000000,
    0x000000000000ffff | 0x0000
  };
  uint64_t type = 0x2;
  __asm__ __volatile__("invept %0, %1" : : "m"(desc), "r"(type));
}
