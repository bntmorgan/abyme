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

#ifndef __DMAR_H__
#define __DMAR_H__

#include "stdint.h"

/**
 * Caution: this driver will only work for the Intel 4th generation core
 * processors like the i7 4770
 * See: 4th-gen-core-family-desktop-vol-2-datasheet.pdf
 *
 * Device to domain and address translation without PASID
 * Consequently, there is no level one paging
 */

// PCI Express requester id
union requester_id {
  struct {
    uint16_t fun:3;
    uint16_t dev:5;
    uint16_t bus:8;
  };
  uint16_t raw;
} __attribute__((packed));

// PCI Express field
union dmar_root_table_address {
  struct {
    uint64_t r0:11;
    uint64_t rtt:1; // Root table type: 0 normal, 1 extended
    uint64_t rta:52; // Root table address: IOMMU CR3...
  };
  uint64_t raw;
} __attribute__((packed));

// Device to domain mapping tables
union dmar_root_entry {
  struct {
    uint64_t p:1; // Present
    uint64_t r0:11;
    uint64_t ctp:52; // Context table address
    uint64_t ro1;
  };
  uint64_t raw[2];
} __attribute__((packed));

union dmar_context_entry {
  struct {
    uint64_t p:1; // Present
    uint64_t fpd:1; // Fault Processing Disable Flag
    uint64_t t:2; // Translation Type
    uint64_t r0:8;
    uint64_t slptptr:52; // Second level page table pointer: idem CR3 & long mode paging
    uint64_t aw:3; // Address width
    uint64_t avail:4;
    uint64_t did:16;
    uint64_t r1:41;
  };
  uint64_t raw[2];
} __attribute__((packed));

// Level 2 paging

union dmar_sl_pml4_entry {
  struct {
    uint64_t r:1;
    uint64_t w:1;
    uint64_t x:1;
    uint64_t ign0:4;
    uint64_t r0:1;
    uint64_t ign1:3;
    uint64_t r1:1;
    uint64_t a:40;
    uint64_t ign2:10;
    uint64_t r2:1;
    uint64_t ign3:1;
  };
  uint64_t raw;
};

union dmar_sl_pdpt_entry {
  struct {
    uint64_t r:1;
    uint64_t w:1;
    uint64_t x:1;
    uint64_t emt:3;
    uint64_t ipat:1;
    uint64_t ps:1; // Page size
    uint64_t ign0:3;
    uint64_t snp:1;
    uint64_t a:40;
    uint64_t ign1:10;
    uint64_t mt:1;
    uint64_t ign2:1;
  };
  uint64_t raw;
};

union dmar_sl_pd_entry {
  struct {
    uint64_t r:1;
    uint64_t w:1;
    uint64_t x:1;
    uint64_t emt:3;
    uint64_t ipat:1;
    uint64_t ps:1; // Page size
    uint64_t ign0:3;
    uint64_t snp:1;
    uint64_t a:40;
    uint64_t ign1:10;
    uint64_t mt:1;
    uint64_t ign2:1;
  };
  uint64_t raw;
};

union dmar_sl_pt_entry {
  struct {
    uint64_t r:1;
    uint64_t w:1;
    uint64_t x:1;
    uint64_t emt:3;
    uint64_t ipat:1;
    uint64_t ign0:4;
    uint64_t snp:1;
    uint64_t a:40;
    uint64_t ign1:10;
    uint64_t mt:1;
    uint64_t ign2:1;
  };
  uint64_t raw;
};

struct dmar_pages {
  // Device to domain
  union dmar_root_entry root_table[256] __attribute__((aligned(0x1000)));
  union dmar_context_entry context_table[256][256] __attribute__((aligned(0x1000)));
  // Address translation level 2
  union dmar_sl_pml4_entry pml4[512] __attribute((aligned(0x1000)));
  union dmar_sl_pdpt_entry pdpt[512][512] __attribute((aligned(0x1000)));
};

//
// PCI Registers
//

// XXX Some PCI STUFF
#define PCI_LPC_RCBA 0xf0
#define PCI_HOST_BRIDGE_MCHBAR 0x48
#define PCI_MCHBAR_VTD1 0x5400
#define PCI_MCHBAR_VTD2 0x5410

//
// VC0PREMAP
//
#define PCI_VC0PREMAP_VER 0x0
#define PCI_VC0PREMAP_CAP 0x8
#define PCI_VC0PREMAP_ECAP 0x10
#define PCI_VC0PREMAP_GCMD 0x18
#define PCI_VC0PREMAP_GSTS 0x1C
#define PCI_VC0PREMAP_RTADDR 0x20
#define PCI_VC0PREMAP_CCMD 0x28
#define PCI_VC0PREMAP_FSTS 0x34
#define PCI_VC0PREMAP_FECTL 0x38
#define PCI_VC0PREMAP_FEDATA 0x3C
#define PCI_VC0PREMAP_FEADDR 0x40
#define PCI_VC0PREMAP_FEUADDR 0x44
#define PCI_VC0PREMAP_AFLOG 0x58
#define PCI_VC0PREMAP_PMEN 0x64
#define PCI_VC0PREMAP_PLMBASE 0x68
#define PCI_VC0PREMAP_PLMLIMIT 0x6C
#define PCI_VC0PREMAP_PHMBASE 0x70
#define PCI_VC0PREMAP_PHMLIMIT 0x78
#define PCI_VC0PREMAP_IQH 0x80
#define PCI_VC0PREMAP_IQT 0x88
#define PCI_VC0PREMAP_IQA 0x90
#define PCI_VC0PREMAP_ICS 0x9C
#define PCI_VC0PREMAP_IECTL 0xA0
#define PCI_VC0PREMAP_IEDATA 0xA4
#define PCI_VC0PREMAP_IEADDR 0xA8
#define PCI_VC0PREMAP_IEUADDR 0xAC
#define PCI_VC0PREMAP_IRTA 0xB8
#define PCI_VC0PREMAP_IVA 0x100
#define PCI_VC0PREMAP_IOTLB 0x108
#define PCI_VC0PREMAP_FRCDL 0x200
#define PCI_VC0PREMAP_FRCDH 0x208

//
// GFXVTBAR
//
#define PCI_GFXVTBAR_VER 0x0
#define PCI_GFXVTBAR_CAP 0x8
#define PCI_GFXVTBAR_ECAP 0x10
#define PCI_GFXVTBAR_GCMD 0x18
#define PCI_GFXVTBAR_GSTS 0x1C
#define PCI_GFXVTBAR_RTADDR 0x20
#define PCI_GFXVTBAR_CCMD 0x28
#define PCI_GFXVTBAR_FSTS 0x34
#define PCI_GFXVTBAR_FECTL 0x38
#define PCI_GFXVTBAR_FEDATA 0x3C
#define PCI_GFXVTBAR_FEADDR 0x40
#define PCI_GFXVTBAR_FEUADDR 0x44
#define PCI_GFXVTBAR_AFLOG 0x58
#define PCI_GFXVTBAR_PMEN 0x64
#define PCI_GFXVTBAR_PLMBASE 0x68
#define PCI_GFXVTBAR_PLMLIMIT 0x6C
#define PCI_GFXVTBAR_PHMBASE 0x70
#define PCI_GFXVTBAR_PHMLIMIT 0x78
#define PCI_GFXVTBAR_IQH 0x80
#define PCI_GFXVTBAR_IQT 0x88
#define PCI_GFXVTBAR_IQA 0x90
#define PCI_GFXVTBAR_ICS 0x9C
#define PCI_GFXVTBAR_IECTL 0xA0
#define PCI_GFXVTBAR_IEDATA 0xA4
#define PCI_GFXVTBAR_IEADDR 0xA8
#define PCI_GFXVTBAR_IEUADDR 0xAC
#define PCI_GFXVTBAR_IRTA 0xB8
#define PCI_GFXVTBAR_IVA 0x100
#define PCI_GFXVTBAR_IOTLB 0x108
#define PCI_GFXVTBAR_FRCDL 0x200
#define PCI_GFXVTBAR_FRCDH 0x208
#define PCI_GFXVTBAR_VTPOLICY 0xFF0

//
// Registers description
//
union vtd_ver {
  struct {
    uint32_t minor:4;
    uint32_t major:4;
    uint32_t r0:24;
  };
  uint32_t raw;
};

union vtd_cap {
  struct {
    uint64_t nd:3;
    uint64_t afl:1;
    uint64_t rwbf:1;
    uint64_t plmr:1;
    uint64_t phmr:1;
    uint64_t cm:1;
    uint64_t sagaw:5;
    uint64_t r0:3;
    uint64_t mgaw:6;
    uint64_t zlr:1;
    uint64_t isoch:1;
    uint64_t fro:10;
    uint64_t sps:4;
    uint64_t r1:1;
    uint64_t psi:1;
    uint64_t nfr:8;
    uint64_t mamv:6;
    uint64_t dwd:1;
    uint64_t drd:1;
    uint64_t r2:8;
  };
  uint64_t raw;
};

// No extended context supported : no bit 24..., no PASID support
union vtd_ecap {
  struct {
    uint64_t c:1;
    uint64_t qi:1;
    uint64_t di:1;
    uint64_t ir:1;
    uint64_t eim:1;
    uint64_t ch:1;
    uint64_t pt:1;
    uint64_t sc:1;
    uint64_t iro:10;
    uint64_t r0:2;
    uint64_t mhmv:4;
    uint64_t r1:40;
  };
  uint64_t raw;
};

union vtd_gcmd {
  struct {
    uint32_t r0:23;
    uint32_t cfi:1;
    uint32_t sirtp:1;
    uint32_t ire:1;
    uint32_t qie:1;
    uint32_t wbf:1;
    uint32_t eafl:1;
    uint32_t sfl:1;
    uint32_t srtp:1;
    uint32_t te:1;
  };
  uint32_t raw;
};

union vtd_gsts {
  struct {
    uint32_t r0:23;
    uint32_t cfis:1;
    uint32_t irtps:1;
    uint32_t ires:1;
    uint32_t qies:1;
    uint32_t wbfs:1;
    uint32_t afls:1;
    uint32_t fls:1;
    uint32_t rtps:1;
    uint32_t tes:1;
  };
  uint32_t raw;
};

union vtd_ccmd {
  struct {
    uint64_t did:8;
    uint64_t r0:8;
    uint64_t sid:16;
    uint64_t fm:2;
    uint64_t r1:25;
    uint64_t caig:2;
    uint64_t cirg:2;
    uint64_t icc:1;
  };
  uint64_t raw;
};

union vtd_fsts {
  struct {
    uint32_t pfo:1;
    uint32_t ppf:1;
    uint32_t afo:1;
    uint32_t apf:1;
    uint32_t iqe:1;
    uint32_t ice:1;
    uint32_t ite:1;
    uint32_t r0:1;
    uint32_t fri:8;
    uint32_t r1:16;
  };
  uint32_t raw;
};

union vtd_fectl {
  struct {
    uint32_t r0:30;
    uint32_t ip:1;
    uint32_t im:1;
  };
  uint32_t raw;
};

union vtd_rtaddr {
  struct {
    uint64_t r0:11;
    uint64_t rtt:1;
    uint64_t rta:27;
    uint64_t r1:25;
  };
  uint64_t raw;
};

union vtd_frcdl {
  struct {
    uint64_t r0:12;
    uint64_t fi:52;
  };
  uint64_t raw;
};

union vtd_frcdh {
  struct {
    uint64_t sid:16;
    uint64_t r0:16;
    uint64_t fr:8;
    uint64_t r1:20;
    uint64_t at:2;
    uint64_t t:1;
    uint64_t f:1;
  };
  uint64_t raw;
};

void dmar_init(void);
void dmar_dump_iommu(uint64_t addr);
void dmar_print_vtd_cap(union vtd_cap f);
void dmar_print_vtd_ecap(union vtd_ecap f);
void dmar_print_vtd_gcmd(union vtd_gcmd f);
void dmar_print_vtd_gsts(union vtd_gsts f);
void dmar_print_vtd_ccmd(union vtd_ccmd f);
void dmar_print_vtd_fsts(union vtd_fsts f);
void dmar_print_vtd_fectl(union vtd_fectl f);
void dmar_print_vtd_rtaddr(union vtd_rtaddr f);
void dmar_print_vtd_frcdl(union vtd_frcdl f);
void dmar_print_vtd_frcdh(union vtd_frcdh f);
void dmar_print_dmar_sl_pml4_entry(union dmar_sl_pml4_entry e);
void dmar_print_dmar_sl_pdpt_entry(union dmar_sl_pdpt_entry e);
void print_protected_memory(void);
void dmar_print_dmar_root_entry(union dmar_root_entry e);
void dmar_print_dmar_context_entry(union dmar_context_entry e);
void dmar_dump(union vtd_rtaddr rta);

// XXX YOLO
extern uint64_t vtd1, vtd2;

#endif//__DMAR_H__
