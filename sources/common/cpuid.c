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

#include "cpuid.h"

#include "cpu.h"
#include "stdio.h"

#define cpuid(idx, eax, ebx, ecx, edx) \
    __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(idx))

union cpuid_80000001_edx {
  struct {
    uint64_t _undef_1:26;
    uint64_t page1g:1;
    uint64_t _undef_2:5;
  } __attribute__((packed));
  uint64_t value;
};

union cpuid_80000008_eax {
  struct {
    uint64_t maxphyaddr:8;
    uint64_t maxlinaddr:8;
    uint64_t reserved_1:16;
  } __attribute__((packed));
  uint64_t value;
};

union cpuid_00000001_ecx {
  struct {
    uint64_t _undef_1:5;
    uint64_t vmx:1;
    uint64_t _undef_2:15;
    uint64_t x2APIC:1;
    uint64_t _undef_3:42;
  } __attribute__((packed));
  uint64_t value;
};

union cpuid_00000001_edx {
  struct {
    uint64_t _undef_1:12;
    uint64_t mtrr:1;
    uint64_t _undef_2:51;
  } __attribute__((packed));
  uint64_t value;
};

uint32_t cpuid_00000000_ebx;
uint32_t cpuid_00000000_ecx;
uint32_t cpuid_00000000_edx;

union cpuid_00000001_ecx cpuid_00000001_ecx;
union cpuid_00000001_edx cpuid_00000001_edx;
uint32_t cpuid_80000000_eax;

union cpuid_80000001_edx cpuid_80000001_edx;

union cpuid_80000008_eax cpuid_80000008_eax;

uint8_t cpuid_is_x2APIC_supported(void) {
  return cpuid_00000001_ecx.x2APIC;
}

uint8_t cpuid_is_page1g_supported(void) {
  return cpuid_80000001_edx.page1g;
}

uint8_t cpuid_are_mtrr_supported(void) {
  return cpuid_00000001_edx.mtrr;
}

uint8_t cpuid_is_vmx_supported(void) {
  return cpuid_00000001_ecx.vmx;
}

uint8_t cpuid_get_maxphyaddr(void) {
  return cpuid_80000008_eax.maxphyaddr;
}

void cpuid_setup(void) {
  uint32_t tmp_a;
  uint32_t tmp_b;
  uint32_t tmp_c;
  uint32_t tmp_d;

  cpuid(0x00000000, tmp_a, cpuid_00000000_ebx, cpuid_00000000_ecx, cpuid_00000000_edx);
  if (cpuid_00000000_ebx != 0x756e6547 || cpuid_00000000_edx != 0x6c65746e || cpuid_00000000_ecx != 0x49656e69) {
    /*TODO panic("!#CPUID VENDOR"); */
  }

  cpuid(0x80000000, cpuid_80000000_eax, tmp_b, tmp_c, tmp_d);
  if (cpuid_80000000_eax < 0x80000008) {
    panic("!#CPUID MAXINPUT");
  }
  cpuid(0x00000001, tmp_a,              tmp_b,              cpuid_00000001_ecx, cpuid_00000001_edx);
  cpuid(0x80000001, tmp_a,              tmp_b,              tmp_c,              cpuid_80000001_edx);
  cpuid(0x80000008, cpuid_80000008_eax, tmp_b,              tmp_c,              tmp_d);
}
