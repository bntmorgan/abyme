#include "msr_bitmap.h"

#include "vmcs.h"
#include "string.h"
#include "mtrr.h"
#include "msr.h"
#include "vmm.h"
#include "efiw.h"

struct msr_bitmap *msr_bitmap;
struct msr_bitmap *msr_bitmap_pool;

void msr_bitmap_setup(void) {
  msr_bitmap = efi_allocate_pages(1);
  msr_bitmap_pool = efi_allocate_pages(VM_NB);
  memset(msr_bitmap, 0, sizeof(msr_bitmap));
  memset(msr_bitmap_pool, 0, sizeof(msr_bitmap) * VM_NB);
}

void msr_bitmap_set_read(uint64_t msr) {
  if (msr <= 0x00001fff) {
    msr_bitmap->low_msrs_read_bitmap[msr / 8] |= (1 << (msr % 8));
  } else {
    uint64_t msr_index = msr - 0xc0000000;
    msr_bitmap->high_msrs_read_bitmap[msr_index / 8] |= (1 << (msr_index % 8));
  }
}

void msr_bitmap_set_write(uint64_t msr) {
  if (msr <= 0x00001fff) {
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
  memcpy(b, msr_bitmap, 0x1000);
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
