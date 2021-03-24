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

#ifndef __CPUID_H__
#define __CPUID_H__

#include <efi.h>
#include "types.h"

void cpuid_setup(void);

uint8_t cpuid_is_long_mode_supported(void);

uint8_t cpuid_is_pae_supported(void);

uint8_t cpuid_is_page1g_supported(void);

uint8_t cpuid_is_vmx_supported(void);

uint8_t cpuid_has_local_apic(void);

uint8_t cpuid_get_maxphyaddr(void);

uint8_t cpuid_are_mtrr_supported(void);

uint8_t cpuid_is_x2APIC_supported(void);

#endif
