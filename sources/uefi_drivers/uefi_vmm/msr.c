#include "msr.h"
#include "cpu.h"

#include "stdio.h"

uint64_t msr_read(uint64_t msr_address) {
  uint32_t eax, edx;
  __asm__ __volatile__("rdmsr" : "=a" (eax), "=d" (edx) : "c" (msr_address));
  return (((uint64_t) edx) << 32) | ((uint64_t) eax);
}

void msr_write(uint64_t msr_address, uint64_t msr_value) {
  __asm__ __volatile__("wrmsr" :
      : "a" (msr_value & 0xffffffff), "d" ((msr_value >> 32) & 0xffffffff), "c" (msr_address));
}

void msr_check_feature_control_msr_lock(void) {
  uint64_t msr_value = msr_read(MSR_ADDRESS_IA32_FEATURE_CONTROL_MSR);
  if ((msr_value & 0x4) == 0) {
    INFO("need to change FEATURE_CONTROL_MSR to allow vmxon outside smx mode\n");
    if ((msr_value & 0x1) == 1) {
      panic("feature control msr locked, change bios configuration\n");
    }
    msr_value = msr_value | 0x4;
    msr_write(MSR_ADDRESS_IA32_FEATURE_CONTROL_MSR, msr_value);
  }
  if ((msr_value & 0x1) == 0) {
    INFO("feature control msr unlocked, trying to lock\n");
    msr_value = msr_value | 0x1;
    msr_write(MSR_ADDRESS_IA32_FEATURE_CONTROL_MSR, msr_value);
  }
}
