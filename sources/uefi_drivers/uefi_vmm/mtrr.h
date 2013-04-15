#ifndef __MTRR_H__
#define __MTRR_H__

#include <efi.h>
#include "types.h"

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

#endif
