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

struct ept_tables {
  uint64_t PML4[512]                __attribute__((aligned(0x1000)));
  uint64_t PML40_PDPT[512]          __attribute__((aligned(0x1000)));
  uint64_t PML40_PDPT_PD[512][512]  __attribute__((aligned(0x1000)));
  uint64_t PML40_PDPT0_PD0_PT[512]  __attribute__((aligned(0x1000)));
  uint64_t PD_PT_VMM [2][512]       __attribute__((aligned(0x1000)));
  uint64_t PT_ETH_MMIO[512]         __attribute__((aligned(0x1000)));
  uint64_t PT_ETH_BAR0[512]         __attribute__((aligned(0x1000)));
} __attribute__((aligned(8)));


inline static uint16_t get_PDPT_offset(uint64_t addr);
inline static uint16_t get_PD_offset(uint64_t addr);
inline static uint16_t get_PT_offset(uint64_t addr);
inline static void check_memory_range(const struct memory_range *memory_range,
                                      uint64_t address, uint64_t page_size);

struct ept_tables ept_tables;

extern uint8_t _protected_begin;
extern uint8_t _protected_end;

uint8_t trap_pci[0x1000] __attribute__((aligned(0x1000)));
uint8_t trap_bar[0x1000] __attribute__((aligned(0x1000)));

/* All the physical memory is mapped using identity mapping. */
void ept_create_tables(void) {

  uint64_t i;
  uint64_t j;
  uint64_t address;
  uint64_t PDPT_offset;
  uint64_t PD_offset;
  const struct memory_range *memory_range;

  /* Initialize traps with 0xff */
  for (i = 0; i < sizeof(trap_pci); i++) {
    trap_pci[i] = 0xff;
    trap_bar[i] = 0xff;
  }

  /* Everything stands into the first 4GB, so we only need the first entry of PML4. */
  ept_tables.PML4[0] = ((uint64_t) &ept_tables.PML40_PDPT[0]) | 0x07;
  for (i = 1; i < 512; i++) {
    ept_tables.PML4[i] = 0;
  }

  /* Map the first 2mb with 4ko pages. */
  address = 0;
  memory_range = mtrr_get_memory_range(address);

  ept_tables.PML40_PDPT_PD[0][0] = ((uint64_t) &ept_tables.PML40_PDPT0_PD0_PT[0]) | 0x7;

  for (i = 0; i < 512; i++) {
    if (address > memory_range->range_address_end) {
      memory_range = mtrr_get_memory_range(address);
    }
    check_memory_range(memory_range, address, 0x1000);

    ept_tables.PML40_PDPT0_PD0_PT[i] = (i << 12) | (memory_range->type << 3) | 0x7;

    address += 0x1000;
  }

  /* Map the remaining memory with 2Mo. */
  uint8_t max_phyaddr = cpuid_get_maxphyaddr();
  uint16_t max_pdpt;
  if (max_phyaddr > 38) {
    // It is more than one entry of PML4
    max_pdpt = 512;
  } else {
    max_pdpt = 1 << (max_phyaddr - 30);
  }
  for (i = 0; i < max_pdpt; i++) {
    ept_tables.PML40_PDPT[i] = ((uint64_t) &ept_tables.PML40_PDPT_PD[i][0]) | 0x7;

    for (j = (i==0)? 1 : 0 ; j < 512; j++) {
      if (memory_range->range_address_end < address) {
        memory_range = mtrr_get_memory_range(address);
      }
      check_memory_range(memory_range, address, 0x200000);

      ept_tables.PML40_PDPT_PD[i][j] = (i << 30) | (j << 21) | (1 << 7) | (memory_range->type << 3) | 0x7;

      address += 0x200000;
    }
  }

  //
  // Protect VMM pages : we split the first and the last 2Mo pages corresponding to VMM memory into 4Ko pages
  //

  /* Already aligned on 4Ko (cf efi.ld) */
  uint64_t p_begin = (uint64_t) &_protected_begin;
  uint64_t p_end = (uint64_t) &_protected_end;

  PDPT_offset = get_PDPT_offset(p_begin);
  uint8_t PD_offset_begin = get_PD_offset(p_begin);
  uint8_t PD_offset_end = get_PD_offset(p_end);

  if (PDPT_offset != get_PDPT_offset(p_end)) {
    panic("!#EPT protected_zone doesn't fit in 1 PDPT\n");
  }

  address = p_begin & ~(0x200000 -1);
  memory_range = mtrr_get_memory_range(address);
  i=0;
  for (PD_offset = PD_offset_begin; PD_offset <= PD_offset_end; PD_offset++) {

    /* We split the first and the last PDs into 4Ko pages */
    if ((PD_offset == PD_offset_begin) || (PD_offset == PD_offset_end)) {
      ept_tables.PML40_PDPT_PD[PDPT_offset][PD_offset] = ((uint64_t) &ept_tables.PD_PT_VMM[i][0]) | 0x7;

      for (j = 0; j < 512; j++) {
        if (address > memory_range->range_address_end) {
          memory_range = mtrr_get_memory_range(address);
        }
        check_memory_range(memory_range, address, 0x1000);

        ept_tables.PD_PT_VMM[i][j] = (PDPT_offset        << 30)
                                   | (PD_offset          << 21)
                                   | (j                  << 12)
                                   | (memory_range->type << 3)
                                   | 0x7;
        /* We protect pages that belongs to vmm addr range */
        if ((address >= p_begin) && (address < p_end)) {
          ept_tables.PD_PT_VMM[i][j] &= ~((uint64_t)0x7);
        }

        address += 0x1000;
      }
      i++;

    } else {
      /* We protect the other PDs pages */
      if (memory_range->range_address_end < address) {
        memory_range = mtrr_get_memory_range(address);
      }
      check_memory_range(memory_range, address, 0x200000);

      ept_tables.PML40_PDPT_PD[PDPT_offset][PD_offset] &= ~((uint64_t)0x7);

      address += 0x200000;
    }
  }


  //
  // Network card
  // MMIO pci space protection
  //
#ifdef _DEBUG_SERVER
  uint64_t PT_offset;
  uint8_t MMCONFIG_length;
  uint16_t MMCONFIG_mask;

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
  uint16_t MMCONFIG = pci_readw(0,0x63) & MMCONFIG_mask;

  uint64_t base_addr =  ((uint64_t)MMCONFIG << 16)
                        + PCI_MAKE_MMCONFIG(eth->pci_addr.bus,
                                      eth->pci_addr.device,
                                      eth->pci_addr.function);

  PDPT_offset = get_PDPT_offset(base_addr);
  PD_offset = get_PD_offset(base_addr);
  PT_offset = get_PT_offset(base_addr);

  /* Map memory associated to eth MMIO configuration with 4ko pages */
  ept_tables.PML40_PDPT_PD[PDPT_offset][PD_offset] = ((uint64_t) &ept_tables.PT_ETH_MMIO[0]) | 0x7;

  address = base_addr & ~(0x200000 -1);
  memory_range = mtrr_get_memory_range(address);
  for (i = 0; i < 512; i++) {
    if (address > memory_range->range_address_end) {
      memory_range = mtrr_get_memory_range(address);
    }
    check_memory_range(memory_range, address, 0x1000);

    ept_tables.PT_ETH_MMIO[i] = (PDPT_offset << 30) | (PD_offset << 21) | (i << 12) | (memory_range->type << 3) | 0x7;

    address += 0x1000;
  }
  /* We hide the first 4Ko of MMIO address with trap_pci redirection */
  ept_tables.PT_ETH_MMIO[PT_offset] = ((uint64_t) &trap_pci[0]) | (memory_range->type << 3) | 0x7;


  //
  // Network card
  // bar0 space protection
  //
  base_addr = eth->bar0;
  PDPT_offset = get_PDPT_offset(base_addr);
  PD_offset = get_PD_offset(base_addr);
  PT_offset = get_PT_offset(base_addr);

  /* Map memory associated to eth bar0 configuration with 4ko pages */
  ept_tables.PML40_PDPT_PD[PDPT_offset][PD_offset] = ((uint64_t) &ept_tables.PT_ETH_BAR0[0]) | 0x7;

  address = base_addr & ~(0x200000 - 1);
  memory_range = mtrr_get_memory_range(address);
  for (i = 0; i < 512; i++) {
    if (address > memory_range->range_address_end) {
      memory_range = mtrr_get_memory_range(address);
    }
    check_memory_range(memory_range, address, 0x1000);

    ept_tables.PT_ETH_BAR0[i] = (PDPT_offset << 30) | (PD_offset << 21) | (i << 12) | (memory_range->type << 3) | 0x7;

    address += 0x1000;
  }
  /* We hide the first 4Ko of Bar0 address with trap_bar redirection */
  ept_tables.PT_ETH_BAR0[PT_offset] = ((uint64_t) &trap_bar[0]) | (memory_range->type << 3) | 0x7;

#endif

}

uint64_t ept_get_eptp(void) {
  return ((uint64_t) &ept_tables.PML4[0]) | (3 << 3) | (0x6 << 0);
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

inline static void check_memory_range(const struct memory_range *memory_range,
                                      uint64_t address, uint64_t page_size) {
  if (memory_range == NULL) {
    panic("!#EPT MR4KB [NULL], address=%016X, page_size=%016X\n", address, page_size);
  }
  if (address + page_size-1 > memory_range->range_address_end) {
    panic("!#EPT MR4KB [?%x<%X<0x%X:%d]\n", memory_range->range_address_begin
                                          , address, memory_range->range_address_end
                                          , memory_range->type);
  }
}
