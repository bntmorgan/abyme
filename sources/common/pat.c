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

#define PAT_MTRR_ENCODING_SIZE 7
#define PAT_PAT_ENCODING_SIZE 8

union pat_entries pat_entries;

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

uint8_t pat_decode_mtrr_pat_type[PAT_MTRR_ENCODING_SIZE][PAT_PAT_ENCODING_SIZE] = {
  { 
    // mtrr UC
    MEMORY_TYPE_UC, // pat UC
    MEMORY_TYPE_WC, // pat WC
    -1,
    -1,
    MEMORY_TYPE_UC, // pat WT
    MEMORY_TYPE_UC, // pat WP
    MEMORY_TYPE_UC, // pat WB
    MEMORY_TYPE_UC, // pat UC-
  },{
    // mtrr WC
    MEMORY_TYPE_UC, // pat UC
    MEMORY_TYPE_WC, // pat WC
    -1,
    -1,
    MEMORY_TYPE_UC, // pat WT
    MEMORY_TYPE_UC, // pat WP
    MEMORY_TYPE_WC, // pat WB
    MEMORY_TYPE_WC, // pat UC-
  },{
    -1, -1, -1, -1, -1, -1, -1, -1
  },{
    -1, -1, -1, -1, -1, -1, -1, -1
  },{
    // mtrr WT
    MEMORY_TYPE_UC, // pat UC
    MEMORY_TYPE_WC, // pat WC
    -1,
    -1,
    MEMORY_TYPE_WT, // pat WT
    MEMORY_TYPE_WP, // pat WP
    MEMORY_TYPE_WT, // pat WB
    MEMORY_TYPE_UC, // pat UC-
  },{
    // mtrr WP
    MEMORY_TYPE_UC, // pat UC
    MEMORY_TYPE_WC, // pat WC
    -1,
    -1,
    MEMORY_TYPE_WT, // pat WT
    MEMORY_TYPE_WP, // pat WP
    MEMORY_TYPE_WP, // pat WB
    MEMORY_TYPE_WC, // pat UC-
  },{
    // mtrr WB
    MEMORY_TYPE_UC, // pat UC
    MEMORY_TYPE_WC, // pat WC
    -1,
    -1,
    MEMORY_TYPE_WT, // pat WT
    MEMORY_TYPE_WP, // pat WP
    MEMORY_TYPE_WB, // pat WB
    MEMORY_TYPE_UC, // pat UC-
  }
};

uint8_t pat_cache_request(uint8_t mtrr_type, uint8_t mem_type, uint8_t *pat_type) {
  uint8_t i;
  for (i = 0; i < PAT_PAT_ENCODING_SIZE; i++) {
     if (pat_decode_mtrr_pat_type[mtrr_type][i] == mem_type) {
       // We have found the PAT type which gives the requested memory type
       *pat_type = i;
       return 0;
     }
  }
  return -1;
}

uint8_t pat_get_entry(uint8_t pat_type, uint8_t *pat_entry) {
  uint8_t i;
  for (i = 0; i < sizeof(pat_entries.entries); i++) {
     if ((pat_entries.entries[i] & 0x7) == pat_type) {
       // INFO("PAT%x : %02x\n", i, (pat_entries.entries[i] & 0x8));
       // We have found the PAT entry which give the right pat type
       *pat_entry = i;
       return 0;
     }
  }
  return -1;
}

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

static inline uint8_t pat_decode_entry_pdpte(uint64_t e) {
  return (((e & PAGING_PDPTE_PWT) != 0) << 0) |
         (((e & PAGING_PDPTE_PCD) != 0) << 1) |
         (((e & PAGING_PDPTE_PAT) != 0) << 2) ;
}

static inline uint8_t pat_decode_entry_pde(uint64_t e) {
  return (((e & PAGING_PDE_PWT) != 0) << 0) |
         (((e & PAGING_PDE_PCD) != 0) << 1) |
         (((e & PAGING_PDE_PAT) != 0) << 2) ;
}

static inline uint8_t pat_decode_entry_pte(uint64_t e) {
  return (((e & PAGING_PTE_PWT) != 0) << 0) |
         (((e & PAGING_PTE_PCD) != 0) << 1) |
         (((e & PAGING_PTE_PAT) != 0) << 2) ;
}

static inline uint8_t pat_decode_entry(uint64_t e, uint8_t s) {
  if (s == PAGING_ENTRY_PDPTE) {
    // INFO("PDPTE 0x%02x\n", pat_decode_entry_pdpte(e));
    return pat_decode_entry_pdpte(e);
  } else if (s == PAGING_ENTRY_PDE) {
    // INFO("PDE 0x%02x\n", pat_decode_entry_pde(e));
    return pat_decode_entry_pde(e);
  } else if (s == PAGING_ENTRY_PTE) {
    // INFO("PTE 0x%02x\n", pat_decode_entry_pte(e));
    return pat_decode_entry_pte(e);
  }
  return -1;
}

static inline uint8_t pat_encode_entry_pdpte(uint64_t *e, uint8_t pat_entry) {
  *e &= ~(PAGING_PDPTE_PWT | PAGING_PDPTE_PCD | PAGING_PDPTE_PAT);
  *e |= (((pat_entry & (1 << 0))) ? PAGING_PDPTE_PWT : 0) |
        (((pat_entry & (1 << 1))) ? PAGING_PDPTE_PCD : 0) |
        (((pat_entry & (1 << 2))) ? PAGING_PDPTE_PAT : 0) ;
  return 0;
}

static inline uint8_t pat_encode_entry_pde(uint64_t *e, uint8_t pat_entry) {
  *e &= ~(PAGING_PDE_PWT | PAGING_PDE_PCD | PAGING_PDE_PAT);
  *e |= (((pat_entry & (1 << 0))) ? PAGING_PDE_PWT : 0) |
        (((pat_entry & (1 << 1))) ? PAGING_PDE_PCD : 0) |
        (((pat_entry & (1 << 2))) ? PAGING_PDE_PAT : 0) ;
  return 0;
}

static inline uint8_t pat_encode_entry_pte(uint64_t *e, uint8_t pat_entry) {
  *e &= ~(PAGING_PTE_PWT | PAGING_PTE_PCD | PAGING_PTE_PAT);
  *e |= (((pat_entry & (1 << 0))) ? PAGING_PTE_PWT : 0) |
        (((pat_entry & (1 << 1))) ? PAGING_PTE_PCD : 0) |
        (((pat_entry & (1 << 2))) ? PAGING_PTE_PAT : 0) ;
  return 0;
}

static inline uint8_t pat_encode_entry(uint64_t *e, uint8_t s, uint8_t pat_entry) {
  if (s == PAGING_ENTRY_PDPTE) {
    // INFO("PDPTE\n");
    return pat_encode_entry_pdpte(e, pat_entry);
  } else if (s == PAGING_ENTRY_PDE) {
    // INFO("PDE\n");
    return pat_encode_entry_pde(e, pat_entry);
  } else if (s == PAGING_ENTRY_PTE) {
    // INFO("PTE\n");
    return pat_encode_entry_pte(e, pat_entry);
  }
  return -1;
}

uint8_t pat_get_memory_type(uint64_t e, uint8_t s) {
  INFO("%02x %02x %02x %02x %02x %02x %02x %02x\n",
      pat_entries.entries[0],
      pat_entries.entries[1],
      pat_entries.entries[2],
      pat_entries.entries[3],
      pat_entries.entries[4],
      pat_entries.entries[5],
      pat_entries.entries[6],
      pat_entries.entries[7]);
  return pat_entries.entries[pat_decode_entry(e, s)];
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

uint8_t pat_set_memory_type_range(uint64_t address, uint8_t mem_type, uint64_t size) {
  uint64_t laddr = address;
  uint64_t frame_addr = 0;
  uint64_t *entry = NULL;
  uint8_t type;
  uint64_t cr3 = cpu_read_cr3();
  while (laddr < address + size) {
    // rx_bufs
    if (paging_walk(cr3, laddr, &entry, &frame_addr, &type)) {
      INFO("Error while walking 0x%016X\n\n", frame_addr);
      return -1;
    }
    if (pat_set_memory_type(entry, type, mem_type)) {
      INFO("Failed to install the right memory type for 0x%016X\n", frame_addr);
      return -1;
    }
    switch (type) {
      case PAGING_ENTRY_PDPTE:
        INFO("PDPTE entry");
        laddr += (1 << 30);
        break;
      case PAGING_ENTRY_PDE:
        INFO("PDE entry");
        laddr += (1 << 21);
        break;
      case PAGING_ENTRY_PTE:
        INFO("PTE entry");
        laddr += (1 << 12);
        break;
    }
    printk(" for linear 0x%016X\n is %s\n", laddr, PAT_TYPE_STRING(mem_type));
  }
  return 0;
}

uint8_t pat_set_memory_type(uint64_t *entry, uint8_t type, uint8_t mem_type) {
  uint64_t frame_addr;
  const struct memory_range *mtrr_range;
  uint8_t mtrr_type;
  uint8_t pat_type;
  uint8_t pat_entry;
  // Get the page address
  if (paging_get_frame_address(*entry, type, &frame_addr)) {
    return -1;
  }
  // INFO("Frame address : 0x%016X\n", frame_addr);
  // Get the mtrr memory type
  mtrr_range = mtrr_get_memory_range(frame_addr);
  mtrr_type = mtrr_range->type;
  // INFO("Mtrr memory type : 0x%02x\n", mtrr_type);
  // Change the memory type
  // We look in the table if the overriding is possible
  if (pat_cache_request(mtrr_type, mem_type, &pat_type)) {
    INFO("Unable to find a PAT value which gives the requested memory type\n");
    return -1;
  }
  // INFO("PAT type for the request 0x%02x\n", pat_type);
  // We look in the PAT entries the entry which gives the requested PAT type
  if (pat_get_entry(pat_type, &pat_entry)){
    INFO("Unable to find a PAT entry which give the requested type\n");
    return -1;
  }
  // INFO("PAT entry for the request 0x%02x\n", pat_entry);
  // We modifies the entry according to the type
  if (pat_encode_entry(entry, type, pat_entry)) {
    INFO("Unable to encode PAT entry in the page entry\n");
    return -1;
  }
  // TEST
  pat_decode_entry(*entry, type);
  // INFO("Modified entry 0x%016X\n", *entry);
  return 0;
}
