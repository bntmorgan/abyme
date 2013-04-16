#ifndef __IO_BITMAP_H__
#define __IO_BITMAP_H__

#include <efi.h>
#include "types.h"

void io_bitmap_setup(void);

void msr_bitmap_set_for_port(uint64_t port);

uint64_t io_bitmap_get_ptr_a(void);

uint64_t io_bitmap_get_ptr_b(void);

#endif
