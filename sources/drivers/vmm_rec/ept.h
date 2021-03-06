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

#ifndef __EPT_H__
#define __EPT_H__

/* TODO: pourquoi les deux ? */
#include <efi.h>
#include "types.h"

/**
 * Physical memory under 4 GB is 4k id mapped, other is 2mb
 */
#define EPTN 4

enum ept_error {
  EPT_OK,
  EPT_E_BAD_PARAMETER,
};

void ept_create_tables(uint64_t protected_begin, uint64_t protected_end);
void ept_cache(void);
int ept_iterate(uint64_t eptp, int (*cb)(uint64_t *, uint64_t, uint8_t));
void ept_set_ctx(uint8_t c);

uint64_t ept_get_eptp(void);
void ept_perm(uint64_t address, uint32_t pages, uint8_t rights, uint8_t c);
void ept_remap(uint64_t virt, uint64_t phy, uint8_t rights, uint8_t c);
void ept_check_mapping(void);
uint8_t ept_get_ctx(void);
int ept_walk(uint64_t cr3, uint64_t linear, uint64_t **e, uint64_t *a,
    uint8_t *s);
void ept_inv_tlb(void);

//
// Paging masks
// And Macros
//

// PML4E
#define EPT_PML4E_P          (7 << 0)
#define EPT_PDPTE_P          (7 << 0)
#define EPT_PDE_P            (7 << 0)
#define EPT_PTE_P            (7 << 0)

#endif
