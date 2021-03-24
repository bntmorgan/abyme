/*
Copyright (C) 2021  Benoît Morgan

This file is part of abyme

abyme is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

abyme is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with abyme.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "msr_bitmap.h"

#include "vmcs.h"
#include "string.h"
#include "stdio.h"
#include "mtrr.h"
#include "msr.h"
#include "vmm.h"
#include "efiw.h"

struct msr_bitmap *msr_bitmap;
struct msr_bitmap *msr_bitmap_pool;
struct msr_bitmap *msr_bitmap_shadow;

void msr_bitmap_setup(void) {
  msr_bitmap = efi_allocate_pages(1);
  msr_bitmap_pool = efi_allocate_pages(VM_NB);
  memset(msr_bitmap, 0, sizeof(struct msr_bitmap));
  memset(msr_bitmap_pool, 0, sizeof(struct msr_bitmap) * VM_NB);
  msr_bitmap_dump(msr_bitmap);
  msr_bitmap_dump(&msr_bitmap_pool[0]);
}

void msr_bitmap_set_read(uint64_t msr) {
  if (msr <= 0x00001fff) {
    msr_bitmap->low_msrs_read_bitmap[msr / 8] |= (1 << (msr % 8));
  } else {
    uint64_t msr_index = msr - 0xc0000000;
    msr_bitmap->high_msrs_read_bitmap[msr_index / 8] |= (1 << (msr_index % 8));
  }
}

void msr_bitmap_dump(struct msr_bitmap *bm) {
  uint32_t msr, msrh;
  uint8_t rl, wl, rh, wh;
  INFO("MSR bitmap(@0x%016X)\n", (uint64_t)bm);
  for (msr = 0, msrh = 0xc0000000; msr < 0x2000; msr++, msrh++) {
    rl = 0, rh = 0,  wl = 0, wh = 0;
    if (msr_bitmap->low_msrs_write_bitmap[msr / 8] & (1 << (msr % 8))) {
      wl = 1;
    }
    if (msr_bitmap->low_msrs_read_bitmap[msr / 8] & (1 << (msr % 8))) {
      rl = 1;
    }
    if (msr_bitmap->high_msrs_write_bitmap[msr / 8] & (1 << (msr % 8))) {
      wh = 1;
    }
    if (msr_bitmap->high_msrs_read_bitmap[msr / 8] & (1 << (msr % 8))) {
      rh = 1;
    }
    if (rl | wl) {
      INFO("  0x%08x(r: %d, w: %d)\n", msr, rl, wl);
    }
    if (rh | wh) {
      INFO("  0x%08x(r: %d, w: %d)\n", msrh, rh, wh);
    }
  }
}

void msr_bitmap_set_write(uint64_t msr) {
  if (msr < 0x00002000) {
    msr_bitmap->low_msrs_write_bitmap[msr / 8] |= (1 << (msr % 8));
  } else {
    uint64_t msr_index = msr - 0xc0000000;
    msr_bitmap->high_msrs_write_bitmap[msr_index / 8] |= (1 << (msr_index % 8));
  }
}

void msr_bitmap_set_read_write(uint64_t msr) {
  msr_bitmap_set_read(msr);
  msr_bitmap_set_write(msr);
}

void msr_bitmap_set_for_mtrr(void) {
  uint64_t i;
  uint64_t count = mtrr_get_nb_variable_mtrr();
  for (i = 0; i < count * 2; i++) {
    msr_bitmap_set_write(MSR_ADDRESS_IA32_MTRR_PHYSBASE0 + i);
  }
  msr_bitmap_set_write(MSR_ADDRESS_IA32_MTRRCAP);
  msr_bitmap_set_write(MSR_ADDRESS_IA32_MTRR_DEF_TYPE);
  msr_bitmap_set_write(MSR_ADDRESS_IA32_MTRR_FIX64K_00000);
  msr_bitmap_set_write(MSR_ADDRESS_IA32_MTRR_FIX16K_80000);
  msr_bitmap_set_write(MSR_ADDRESS_IA32_MTRR_FIX16K_A0000);
  msr_bitmap_set_write(MSR_ADDRESS_IA32_MTRR_FIX4K_C0000);
  msr_bitmap_set_write(MSR_ADDRESS_IA32_MTRR_FIX4K_C8000);
  msr_bitmap_set_write(MSR_ADDRESS_IA32_MTRR_FIX4K_D0000);
  msr_bitmap_set_write(MSR_ADDRESS_IA32_MTRR_FIX4K_D8000);
  msr_bitmap_set_write(MSR_ADDRESS_IA32_MTRR_FIX4K_E0000);
  msr_bitmap_set_write(MSR_ADDRESS_IA32_MTRR_FIX4K_E8000);
  msr_bitmap_set_write(MSR_ADDRESS_IA32_MTRR_FIX4K_F0000);
  msr_bitmap_set_write(MSR_ADDRESS_IA32_MTRR_FIX4K_F8000);
}

/**
 * Initialise the mrst bitmap with the host configuration
 */
void msr_bitmap_clone(uint8_t *b) {
  memcpy(b, msr_bitmap, sizeof(struct msr_bitmap));
}

/**
 * Create an executive msr bitmap with the host configuration and the
 * virtualized hypervisor bitmap
 */
void msr_bitmap_or(uint8_t *b_dst, uint8_t *b_src) {
  uint32_t i;
  for (i = 0; i < 0x1000; i++) {
    b_dst[i] = ((uint8_t *)msr_bitmap)[i] | b_src[i];
  }
}

/**
 * Checks if the current MSR access needs to be redirected to l1 host
 */

int msr_bitmap_write_host_redirect(uint64_t msr) {
  return (msr < 0x2000) ?
      msr_bitmap_shadow->low_msrs_write_bitmap[msr >> 3] & (1 << (msr % 8)) :
      msr_bitmap_shadow->high_msrs_write_bitmap[(msr - 0xc0000000) >> 3]
          & (1 << ((msr - 0xc0000000) % 8)) ;
}

int msr_bitmap_read_host_redirect(uint64_t msr) {
  return (msr < 0x2000) ?
      msr_bitmap_shadow->low_msrs_read_bitmap[msr >> 3] & (1 << (msr % 8)) :
      msr_bitmap_shadow->high_msrs_read_bitmap[(msr - 0xc0000000) >> 3]
          & (1 << ((msr - 0xc0000000) % 8)) ;
}
