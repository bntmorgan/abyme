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

enum segment_descriptor_type {
  SEGMENT_DESCRIPTOR_TYPE_DATA = 0x0,
  SEGMENT_DESCRIPTOR_TYPE_CODE = 0x1,
};

struct segment_descriptor {
  uint16_t limit0;
  uint16_t base0;
  uint8_t base1;
  union {
    struct {
      uint8_t a:1;
      uint8_t :1;
      uint8_t :1;
      uint8_t t:1;
      uint8_t s:1;
      uint8_t dpl:2;
      uint8_t p:1;
    };
    struct {
      uint8_t type:4;
      uint8_t access_rights:4;
    };
    struct {
      uint8_t :1;
      uint8_t w:1;
      uint8_t r:1;
      uint8_t :5;
    };
    struct {
      uint8_t :1;
      uint8_t e:1;
      uint8_t c:1;
      uint8_t :5;
    };
  }; // Byte 5
  union {
    struct {
      uint8_t limit1:4;
      uint8_t avl:1;
      uint8_t l:1;
      uint8_t :1;
      uint8_t g:1;
    };
    struct {
      uint8_t :4;
      uint8_t granularity:4;
    };
    struct {
      uint8_t :6;
      uint8_t d:1;
      uint8_t :1;
    };
    struct {
      uint8_t :6;
      uint8_t b:1;
      uint8_t :1;
    };
  }; // Byte 6
  uint8_t base2;
} __attribute__((packed));

void gdt_print_gdt(struct gdt_ptr_64 *gdt_ptr);
void gdt_setup_guest_gdt(void);
void gdt_setup_host_gdt(void);
void gdt_print_host_gdt(void);
void gdt_get_entry(struct segment_descriptor *gdt, uint64_t selector, struct
    segment_descriptor **dsc);
void gdt_get_host_entry(uint16_t selector, struct segment_descriptor **dsc);
uint64_t gdt_get_host_base(void);
uint64_t gdt_get_host_limit(void);
void gdt_get_guest_entry(uint64_t selector, struct segment_descriptor **dsc);
uint64_t gdt_get_guest_base(void);
uint64_t gdt_get_guest_limit(void);
uint32_t gdt_descriptor_get_base(struct segment_descriptor *dsc);
uint32_t gdt_descriptor_get_limit(struct segment_descriptor *dsc);
void gdt_init(void);

extern uint32_t gdt_legacy_segment_descriptor;

#endif
