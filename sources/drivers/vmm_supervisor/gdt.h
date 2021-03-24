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

#ifndef __GDT_H__
#define __GDT_H__

#include <efi.h>

#include "types.h"

struct gdt_ptr_32 {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

struct gdt_ptr_64 {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed));

struct gdt_entry {
  uint32_t base;
  uint32_t limit;
  uint8_t access;
  uint8_t granularity;
};

void gdt_setup_guest_gdt(void);

void gdt_setup_host_gdt(void);

void gdt_print_host_gdt(void);

void gdt_get_host_entry(uint64_t selector, struct gdt_entry *gdt_entry);

uint64_t gdt_get_host_base(void);

uint64_t gdt_get_host_limit(void);

void gdt_get_guest_entry(uint64_t selector, struct gdt_entry *gdt_entry);

uint64_t gdt_get_guest_base(void);

uint64_t gdt_get_guest_limit(void);

void gdt_copy_desc(struct gdt_entry *entry, uint8_t *gdt_desc);

void gdt_copy_entry(uint8_t *gdt_desc, struct gdt_entry *entry);

#endif
