#ifndef __MTRR_H__
#define __MTRR_H__

#include <efi.h>
#include "types.h"
#include "vmm_info.h"

#define MTRR_VALID_TYPE(type) ((type == 0) || (type == 1) || (type == 4) || (type == 5) || (type == 6))

extern mtrr_fixed_t mtrr_fixed;

uint8_t mtrr_support(void);

void mtrr_fixed_read(void);

void mtrr_print(void);

uint64_t mtrr_variable_count(void);

#endif
