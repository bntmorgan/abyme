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
    uint64_t cpt:52; // Context table address
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

// Functions
void dmar_init(void);

#endif//__DMAR_H__
