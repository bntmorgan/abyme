#ifndef __IO_BITMAP_H__
#define __IO_BITMAP_H__

#include <efi.h>
#include "types.h"

extern uint8_t (*io_bitmap_a_pool)[0x1000];
extern uint8_t (*io_bitmap_b_pool)[0x1000];

void io_bitmap_setup(void);

void io_bitmap_set_for_port(uint64_t port);

void io_bitmap_clone_a_b(uint8_t *a, uint8_t *b);

void io_bitmap_or(uint8_t *a_dst, uint8_t *a_src, uint8_t *b_dst,
    uint8_t *b_src);

#endif
