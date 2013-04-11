#include "mtrr.h"
#include "hardware/msr.h"

#include "stdio.h"

mtrr_fixed_t mtrr_fixed;

void mtrr_fixed_read(void) {
  mtrr_fixed.fix64K_00000.q = msr_read64(MSR_ADDRESS_IA32_MTRR_FIX64K_00000);
  mtrr_fixed.fix16K_80000.q = msr_read64(MSR_ADDRESS_IA32_MTRR_FIX16K_80000);
  mtrr_fixed.fix16K_A0000.q = msr_read64(MSR_ADDRESS_IA32_MTRR_FIX16K_A0000);
  mtrr_fixed.fix4K_C0000.q = msr_read64(MSR_ADDRESS_IA32_MTRR_FIX4K_C0000);
  mtrr_fixed.fix4K_C8000.q = msr_read64(MSR_ADDRESS_IA32_MTRR_FIX4K_C8000);
  mtrr_fixed.fix4K_D0000.q = msr_read64(MSR_ADDRESS_IA32_MTRR_FIX4K_D0000);
  mtrr_fixed.fix4K_D8000.q = msr_read64(MSR_ADDRESS_IA32_MTRR_FIX4K_D8000);
  mtrr_fixed.fix4K_E0000.q = msr_read64(MSR_ADDRESS_IA32_MTRR_FIX4K_E0000);
  mtrr_fixed.fix4K_E8000.q = msr_read64(MSR_ADDRESS_IA32_MTRR_FIX4K_E8000);
  mtrr_fixed.fix4K_F0000.q = msr_read64(MSR_ADDRESS_IA32_MTRR_FIX4K_F0000);
  mtrr_fixed.fix4K_F8000.q = msr_read64(MSR_ADDRESS_IA32_MTRR_FIX4K_F8000);
}

#define MTRR_PRINT_FIXED(zone, field) \
  printk(zone ": %02x | %02x | %02x | %02x | %02x | %02x | %02x | %02x\n", \
    mtrr_fixed.field.b0, mtrr_fixed.field.b1, mtrr_fixed.field.b2, \
    mtrr_fixed.field.b3, mtrr_fixed.field.b4, mtrr_fixed.field.b5, \
    mtrr_fixed.field.b6, mtrr_fixed.field.b7);

void mtrr_print_fixed() {
  MTRR_PRINT_FIXED("00000-7ffff", fix64K_00000);
  MTRR_PRINT_FIXED("80000-9ffff", fix16K_80000);
  MTRR_PRINT_FIXED("a0000-bffff", fix16K_A0000);
  MTRR_PRINT_FIXED("c0000-c7fff", fix4K_C0000);
  MTRR_PRINT_FIXED("c8000-cffff", fix4K_C8000);
  MTRR_PRINT_FIXED("d0000-d7fff", fix4K_D0000);
  MTRR_PRINT_FIXED("d8000-dffff", fix4K_D8000);
  MTRR_PRINT_FIXED("e0000-e7fff", fix4K_E0000);
  MTRR_PRINT_FIXED("e8000-effff", fix4K_E8000);
  MTRR_PRINT_FIXED("f0000-f7fff", fix4K_F0000);
  MTRR_PRINT_FIXED("f8000-fffff", fix4K_F8000);
}

uint64_t mtrr_variable_count() {
  return msr_read64(MSR_ADDRESS_IA32_MTRR_CAP) & 0xff;
}

void mtrr_print() {
  uint64_t mtrr_cap_msr = msr_read64(MSR_ADDRESS_IA32_MTRR_CAP);
  INFO("\nCache types :\n0x0 Uncacheable (UC)\n0x1 Write Combining (WC)\n0x2 Reserved*\n\
0x3 Reserved*\n0x4 Write-through (WT)\n0x5 Write-protected (WP)\n0x6 Writeback (WB)\n0x7 - 0xff Reserved*\n");
  INFO("MTRR CAP MSR SMRR : %d, WC : %d, FIX : %d, VCNT : %x\n", (mtrr_cap_msr >> 11) & 0x1, (mtrr_cap_msr >> 10) & 0x1, (mtrr_cap_msr >> 8) & 0x1, mtrr_cap_msr & 0xff);
  uint64_t mtrr_def_type_msr = msr_read64(MSR_ADDRESS_IA32_MTRR_DEF_TYPE);
  INFO("MTRR DEF TYPE MSR E : %d, FE : %d, Type : %x\n", (mtrr_def_type_msr >> 11) & 0x1, (mtrr_def_type_msr >> 10) & 0x1, mtrr_def_type_msr & 0xff);
  mtrr_print_fixed();
}
