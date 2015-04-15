#include "ept.h"

// TODO: pour inclure stdio dans ept? => trouver une autre solution?
#include "stdio.h"

#include "mtrr.h"
#include "debug_server/debug_server.h"
#include "pci.h"
#include "cpuid.h"
#include "paging.h"
#include "efiw.h"

struct ept_tables {
  uint64_t PML4[512]                           __attribute__((aligned(0x1000)));
  // 1 GB mapped over 512 giga bytes
  uint64_t PML4_PDPT[512][512]                 __attribute__((aligned(0x1000)));
  // 2 MB mapped over 4 giga bytes
  uint64_t PML40_PDPT_PD[512][512]             __attribute__((aligned(0x1000)));
  // 4 KB mapped under 4 giga bytes
  uint64_t PML40_PDPT0_N_PD_PT[EPTN][512][512] __attribute__((aligned(0x1000)));
} __attribute__((aligned(8)));


inline static uint16_t get_PML4_offset(uint64_t addr);
inline static uint16_t get_PDPT_offset(uint64_t addr);
inline static uint16_t get_PD_offset(uint64_t addr);
inline static uint16_t get_PT_offset(uint64_t addr);
static struct ept_tables *ept_tables;

extern uint8_t _protected_begin;
extern uint8_t _protected_end;

uint8_t trap_pci[0x1000] __attribute__((aligned(0x1000)));
uint8_t trap_bar[0x1000] __attribute__((aligned(0x1000)));
  
static const struct memory_range *memory_range;

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
      panic("!#EPT MR4KB [?%x<0x%X<0x%X:%d]\n", memory_range->range_address_begin
          , address, memory_range->range_address_end , memory_range->type);
    }
    *entry |= (memory_range->type << 3);
  } else {
    *entry |= (0 << 3);
  }
}

void ept_remap(uint64_t virt, uint64_t phy, uint8_t rights) {
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

  INFO("PDPT(%d), PD(%d), PT(%d), virt(@0x%016X) => phy(@0x%016X)\n",
      PDPT_offset, PD_offset, PT_offset, virt, phy);
  ept_tables->PML40_PDPT0_N_PD_PT[PDPT_offset][PD_offset][PT_offset] = phy |
    (rights & 0x7);
} 

void ept_perm(uint64_t address, uint32_t pages, uint8_t rights) {
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
          ept_tables->PML40_PDPT0_N_PD_PT[j][k][l] &= (uint64_t)~0x7;
          ept_tables->PML40_PDPT0_N_PD_PT[j][k][l] |= rights & 0x7;
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
              set_memory_type(&ept_tables->PML40_PDPT0_N_PD_PT[j][k][l], address,
                  0x1000, max_phyaddr);
            }
          } else {
            address = (j << 30) | (k << 21);
            set_memory_type(&ept_tables->PML40_PDPT_PD[j][k], address, 0x200000,
                max_phyaddr);
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
void ept_create_tables(void) {
  uint64_t i; // MPL4
  uint64_t j; // PDPT
  uint64_t k; // PD
  uint64_t l; // PT
  uint64_t address; // Current address

  ept_tables = efi_allocate_pages(sizeof(struct ept_tables) >> 12);

  /* ID Map all virtual memory with rwx rights */
  for (i = 0; i < 512; i++) {
    ept_tables->PML4[i] = ((uint64_t) &ept_tables->PML4_PDPT[i][0]) | 0x07;
    for (j = 0; j < 512; j++) {
      if (i == 0) {
        // Under 512 giga bytes : 2 MB mapped
        ept_tables->PML4_PDPT[i][j] = 
          ((uint64_t) &ept_tables->PML40_PDPT_PD[j][0]) | 0x07;
        for (k = 0; k < 512; k++) {
          if (j < EPTN) {
            // Under EPTN giga bytes : 4 kB mapped
            ept_tables->PML40_PDPT_PD[j][k] = 
              ((uint64_t) &ept_tables->PML40_PDPT0_N_PD_PT[j][k][0]) | 0x07;
            for (l = 0; l < 512; l++) {
              address = (j << 30) | (k << 21) | (l << 12);
              ept_tables->PML40_PDPT0_N_PD_PT[j][k][l] = address | 0x07;
            }
          } else {
            address = (j << 30) | (k << 21);
            ept_tables->PML40_PDPT_PD[j][k] = address | (1 << 7) | 0x07;
          }
        }
      } else {
        // Over 512 giga bytes : 1 GB mapped
        address = (i << 39) | (j << 30);
        ept_tables->PML4_PDPT[i][j] = address | (1 << 7) | 0x07;
      }
    }
  }

  //
  // Protect VMM pages : we split the first and the last 2Mo pages corresponding
  // to VMM memory into 4Ko pages
  //

  /* Already aligned on 4Ko (cf efi.ld) */
  uint64_t p_begin = (uint64_t) &_protected_begin;
  uint64_t p_end = (uint64_t) &_protected_end;

  extern uint64_t vm_RIP;
  INFO("vm start 0x%016X\n", vm_RIP);

  // Protect the VMM memory space
  ept_perm(p_begin + 0x1000, (p_end - p_begin) >> 12, 0x0);

  // Compute the cache policy thanks to the mtrrs ranges
  ept_cache();

  //
  // Network card
  // MMIO pci space protection
  //
#ifdef _DEBUG_SERVER
  if (debug_server) {
    uint64_t MMCONFIG_base;
    uint8_t MMCONFIG_length;
    uint16_t MMCONFIG_mask;
    uint64_t base_addr;
    uint64_t MMCONFIG;

    /* Initialize traps with 0xff */
    for (i = 0; i < sizeof(trap_pci); i++) {
      trap_pci[i] = 0xff;
      trap_bar[i] = 0xff;
    }

    /* MMCONFIG_length : bit 1:2 of PCIEXBAR (offset 60) */
    MMCONFIG_length = (pci_readb(0,0x60) & 0x6) >> 1;

    /* MMCONFIG corresponds to bits 38 to 28 of the pci base address
       MMCONFIG_length decrease to 27 or 26 the lsb of MMCONFIG */
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

    /* MMCONFIG : bit 38:28-26 of PCIEXBAR (offset 60) -> 14:4-2 of PCIEXBAR + 3 */
    MMCONFIG = pci_readw(0,0x63) & MMCONFIG_mask;
    MMCONFIG_base = ((uint64_t)MMCONFIG << 16);

    INFO("MMCONFIG : base(@0x%016X)\n", MMCONFIG_base);
    base_addr = MMCONFIG_base + PCI_MAKE_MMCONFIG(eth->pci_addr.bus,
        eth->pci_addr.device, eth->pci_addr.function);

    // ept_perm(base_addr, 1, 0x7);
    INFO("MMIO NIC config space(@0x%016X)\n", base_addr);

    // XXX For the moment the VMs can use the network card with the UEFI driver
    // for debug purpose
    // ept_remap(base_addr, (uint64_t)&trap_pci[0], 0x7);
    ept_remap(eth->bar0, (uint64_t)&trap_bar[0], 0x7);
  }
#endif
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

  INFO("eptp 0x%016X\n", eptp);

  // Initiate the algorithm
  pml4 = (uint64_t *)(eptp & PAGING_CR3_PLM4_ADDR);
  INFO("@PML4 0x%016X\n", pml4);

  // PML4
  for (i = 0; i < 512; i++) {
    e = &pml4[i];
    // INFO("@PML4E 0x%016X\n", *e);
    if (!(*e & EPT_PML4E_P)) {
      paging_error = PAGING_WALK_NOT_PRESENT;
      INFO("YO1 (%x, %x, %x, %x)\n", i, j, k, l);
      return -1;
    }
    pdpt = (uint64_t *)(*e & PAGING_PML4E_PDPT_ADDR);
    // INFO("@PDPT 0x%016X\n", pdpt);
    // PDPT
    for (j = 0; j < 512; j++) {
      e = &pdpt[j];
      // INFO("@PDPTE 0x%016X\n", *e);
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
