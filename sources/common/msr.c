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
