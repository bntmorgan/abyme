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

#ifndef __SMP_H__
#define __SMP_H__

#include <efi.h>
#include "gdt.h"

struct ap_param {
  // 0x00
  struct gdt_ptr_32 gdt_ptr_pm;
  // 0x06
  struct gdt_ptr_64 gdt_ptr_lm;
  // 0x10
  uint32_t cr3_lm;
  // 0x14
  uint64_t vmm_next;
} __attribute__((packed));

void smp_setup(void);
void smp_activate_cores(void);

#endif//__SMP_H__
