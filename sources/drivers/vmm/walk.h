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

#ifndef __WALK_H__
#define __WALK_H__

#include "stdint.h"

enum walk_error {
  WALK_OK,
  WALK_ERROR_PML4E_NOT_PRESENT,
  WALK_ERROR_PDPTE_NOT_PRESENT,
  WALK_ERROR_PDE_NOT_PRESENT,
  WALK_ERROR_PTE_NOT_PRESENT
};

#define WALK_4KO 0x1000
#define WALK_2MB 0x200000
#define WALK_1GB 0x40000000

int walk_long(uint64_t cr3, uint64_t linear, uint64_t *physical, uint64_t
    *size);

uint8_t walk_is_in_page(uint64_t l, uint64_t p, uint64_t size);

#endif//__WALK_H__
