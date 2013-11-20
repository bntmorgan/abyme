#ifndef __MTRR_H__
#define __MTRR_H__

#include <efi.h>
#include "types.h"

#define MEMORY_TYPE_UC 0
#define MEMORY_TYPE_WC 1
#define MEMORY_TYPE_WT 4
#define MEMORY_TYPE_WP 5
#define MEMORY_TYPE_WB 6

struct memory_range {
  uint8_t type;
  uint64_t range_address_begin;
  uint64_t range_address_end;
  struct memory_range *next;
};

void mtrr_print_ranges(void);

uint8_t mtrr_create_ranges(void);

uint8_t mtrr_get_nb_variable_mtrr(void);

const struct memory_range *mtrr_get_memory_range(uint64_t address);

struct msr_mtrrcap {
  struct {
    uint64_t address;
    union {
      struct {
        uint64_t vcnt:8;
        uint64_t fix:1;
        uint64_t reserved_1:1;
        uint64_t wc:1;
        uint64_t smrr:1;
        uint64_t reserved_2:52;
      } __attribute__((packed));
      uint64_t value;
    } __attribute__((packed));
  } msr;
};

extern struct msr_mtrrcap mtrr_cap;

#endif
