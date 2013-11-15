#include "stdio.h"
#include "pat.h"
#include "mtrr.h"
#include "cpu.h"
#include "msr.h"
#include "paging.h"

#define MEMORY_TYPE_UC 0
#define MEMORY_TYPE_WC 1
#define MEMORY_TYPE_WT 4
#define MEMORY_TYPE_WP 5
#define MEMORY_TYPE_WB 6
#define MEMORY_TYPE_UC_MINUS 7

// 0x100 : 0x8 to 0xff are reserved
char *pat_type_strings[0x100] = {
  "Uncacheable (UC)",
  "Write Combining (WC)",
  0x0,
  0x0,
  "Write Through (WT)",
  "Write Protected (WP)",
  "Write Back (WB)",
  "Uncached (UC-)",
  "Uncacheable (UC)"
};

struct pat_entries_t pat_entries;

inline uint8_t pat_decode_entry_pdpte(uint64_t e) {
  return (((e & PAGING_PDPTE_PWT) != 0) << 0) |
         (((e & PAGING_PDPTE_PCD) != 0) << 1) |
         (((e & PAGING_PDPTE_PAT) != 0) << 2) ;
}

inline uint8_t pat_decode_entry_pde(uint64_t e) {
  return (((e & PAGING_PDE_PWT) != 0) << 0) |
         (((e & PAGING_PDE_PCD) != 0) << 1) |
         (((e & PAGING_PDE_PAT) != 0) << 2) ;
}

inline uint8_t pat_decode_entry_pte(uint64_t e) {
  return (((e & PAGING_PTE_PWT) != 0) << 0) |
         (((e & PAGING_PTE_PCD) != 0) << 1) |
         (((e & PAGING_PTE_PAT) != 0) << 2) ;
}

inline uint8_t pat_decode_entry(uint64_t e, uint8_t s) {
  if (s == PAGING_ENTRY_PTPTE) {
    return pat_decode_entry_pdpte(e);
  } else if (s == PAGING_ENTRY_PDE) {
    return pat_decode_entry_pde(e);
  } else if (s == PAGING_ENTRY_PTE) {
    return pat_decode_entry_pte(e);
  }
  return -1;
}

uint8_t pat_get_memory_type(uint64_t e, uint8_t s) {
  return pat_entries.entries[pat_decode_entry(e, s)];
}

void pat_print_entries() {
  INFO("PAT0 0x%02X : %s\n", pat_entries.pat0, PAT_TYPE_STRING(pat_entries.pat0));
  INFO("PAT1 0x%02X : %s\n", pat_entries.pat1, PAT_TYPE_STRING(pat_entries.pat1));
  INFO("PAT2 0x%02X : %s\n", pat_entries.pat2, PAT_TYPE_STRING(pat_entries.pat2));
  INFO("PAT3 0x%02X : %s\n", pat_entries.pat3, PAT_TYPE_STRING(pat_entries.pat3));
  INFO("PAT4 0x%02X : %s\n", pat_entries.pat4, PAT_TYPE_STRING(pat_entries.pat4));
  INFO("PAT5 0x%02X : %s\n", pat_entries.pat5, PAT_TYPE_STRING(pat_entries.pat5));
  INFO("PAT6 0x%02X : %s\n", pat_entries.pat6, PAT_TYPE_STRING(pat_entries.pat6));
  INFO("PAT7 0x%02X : %s\n", pat_entries.pat7, PAT_TYPE_STRING(pat_entries.pat7));
}

void pat_read_pat() {
  // Read the pat
  pat_entries.msr = msr_read(MSR_ADDRESS_IA32_PAT);
}

void pat_init_strings() {
  uint32_t i;
  // Init the strings
  pat_type_strings[2] = "Reserved";
  pat_type_strings[3] = pat_type_strings[2];
  for (i = 8; i < 0x100; i++) {
    pat_type_strings[i] = pat_type_strings[2];
  }
}


void pat_setup() {
  pat_init_strings();
  pat_read_pat();
  pat_print_entries();
}

uint8_t pat_supported() {
  return 1;
}
