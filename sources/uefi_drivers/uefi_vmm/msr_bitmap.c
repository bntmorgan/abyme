#include "msr_bitmap.h"

#include "vmcs.h"
#include "string.h"
#include "mtrr.h"
#include "msr.h"

struct msr_bitmap {
  uint8_t low_msrs_read_bitmap[0x400];
  uint8_t high_msrs_read_bitmap[0x400];
  uint8_t low_msrs_write_bitmap[0x400];
  uint8_t high_msrs_write_bitmap[0x400];
} __attribute__((packed));

struct msr_bitmap msr_bitmap __attribute__((aligned(0x1000)));

void msr_bitmap_setup(void) {
  memset(&msr_bitmap, 0, sizeof(msr_bitmap));
}

void msr_bitmap_set_read_write(uint64_t msr) {
  if (msr <= 0x00001fff) {
    msr_bitmap.low_msrs_read_bitmap[msr / 8] |= (1 << (msr % 8));
    msr_bitmap.low_msrs_write_bitmap[msr / 8] |= (1 << (msr % 8));
  } else {
    uint64_t msr_index = msr - 0xc0000000;
    msr_bitmap.low_msrs_read_bitmap[msr_index / 8] |= (1 << (msr_index % 8));
    msr_bitmap.low_msrs_write_bitmap[msr_index / 8] |= (1 << (msr_index % 8));
  }
}

void msr_bitmap_set_for_mtrr(void) {
  uint64_t i;
  uint64_t count = mtrr_get_nb_variable_mtrr();
  for (i = 0; i < count * 2; i++) {
    msr_bitmap_set_read_write(MSR_ADDRESS_IA32_MTRR_PHYBASE0 + i);
  }
  msr_bitmap_set_read_write(MSR_ADDRESS_IA32_MTRRCAP);
  msr_bitmap_set_read_write(MSR_ADDRESS_A32_MTRR_DEF_TYPE);
  msr_bitmap_set_read_write(MSR_ADDRESS_IA32_MTRR_FIX64K_00000);
  msr_bitmap_set_read_write(MSR_ADDRESS_IA32_MTRR_FIX16K_80000);
  msr_bitmap_set_read_write(MSR_ADDRESS_IA32_MTRR_FIX16K_A0000);
  msr_bitmap_set_read_write(MSR_ADDRESS_IA32_MTRR_FIX4K_C0000);
  msr_bitmap_set_read_write(MSR_ADDRESS_IA32_MTRR_FIX4K_C8000);
  msr_bitmap_set_read_write(MSR_ADDRESS_IA32_MTRR_FIX4K_D0000);
  msr_bitmap_set_read_write(MSR_ADDRESS_IA32_MTRR_FIX4K_D8000);
  msr_bitmap_set_read_write(MSR_ADDRESS_IA32_MTRR_FIX4K_E0000);
  msr_bitmap_set_read_write(MSR_ADDRESS_IA32_MTRR_FIX4K_E8000);
  msr_bitmap_set_read_write(MSR_ADDRESS_IA32_MTRR_FIX4K_F0000);
  msr_bitmap_set_read_write(MSR_ADDRESS_IA32_MTRR_FIX4K_F8000);
}

uint64_t msr_bitmap_get_ptr(void) {
  return (uint64_t) &msr_bitmap;
}
