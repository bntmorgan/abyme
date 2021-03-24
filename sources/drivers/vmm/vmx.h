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

#ifndef __VMX_H__
#define __VMX_H__

#include <efi.h>
#include "types.h"

void cpu_enable_vmxe(void);

void cpu_vmxon(uint8_t *region);

void cpu_vmclear(uint8_t *region);

void cpu_vmptrld(uint8_t *region);

void cpu_vmlaunch(void);

void cpu_vmwrite(uint64_t field, uint64_t value);

uint64_t cpu_vmread(uint64_t field);

uint8_t cpu_vmread_safe(unsigned long field, unsigned long *value);

uint32_t cpu_adjust32(uint32_t value, uint32_t msr);

uint64_t cpu_adjust64(uint64_t value, uint32_t msr_fixed0, uint32_t msr_fixed1);

/* We fix the calling conventions to sysv_abi */
void vmx_transition_display_error(uint8_t VMfailInvalid, uint8_t VMfailValid) __attribute__((sysv_abi));

#endif//__VMX_H__
