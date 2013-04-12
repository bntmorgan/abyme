#ifndef __MTRR_H__
#define __MTRR_H__

#include <efi.h>
#include "types.h"
#include "vmm_info.h"

#define MTRR_VALID_TYPE(type) ((type == 0) || (type == 1) || (type == 4) || (type == 5) || (type == 6))

uint8_t mtrr_support(void);

void mtrr_fixed_read(void);

void mtrr_print(void);

uint64_t mtrr_variable_count(void);

uint8_t mtrr_initialize(void);

uint8_t mtrr_compute_memory_ranges(void);

void mtrr_print_ranges(void);

struct memory_range {
  uint8_t type;
  uint64_t range_address_begin;
  uint64_t range_address_end;
  struct memory_range *next;
};

uint8_t mtrr_get_nb_variable_mtrr(void);

const struct memory_range *mtrr_get_memory_range(uint64_t address);

#endif
