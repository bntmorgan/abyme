#ifndef __MSR_BITMAP_H__
#define __MSR_BITMAP_H__

#include <efi.h>
#include "types.h"

struct msr_bitmap {
  uint8_t low_msrs_read_bitmap[0x400];
  uint8_t high_msrs_read_bitmap[0x400];
  uint8_t low_msrs_write_bitmap[0x400];
  uint8_t high_msrs_write_bitmap[0x400];
} __attribute__((packed));

extern struct msr_bitmap *msr_bitmap_pool;

void msr_bitmap_setup(void);

void msr_bitmap_set_for_mtrr(void);

void msr_bitmap_set_read_write(uint64_t msr);
void msr_bitmap_set_write(uint64_t msr);
void msr_bitmap_set_read(uint64_t msr);

void msr_bitmap_dump(struct msr_bitmap *bm);

void msr_bitmap_clone(uint8_t *b);
void msr_bitmap_or(uint8_t *b_dst, uint8_t *b_src);

int msr_bitmap_write_host_redirect(uint64_t msr);
int msr_bitmap_read_host_redirect(uint64_t msr);

extern struct msr_bitmap *msr_bitmap_shadow;

#endif
