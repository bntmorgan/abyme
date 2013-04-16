#ifndef __MSR_BITMAP_H__
#define __MSR_BITMAP_H__

#include <efi.h>
#include "types.h"

void msr_bitmap_setup(void);

void msr_bitmap_set_for_mtrr(void);

uint64_t msr_bitmap_get_ptr(void);

#endif
