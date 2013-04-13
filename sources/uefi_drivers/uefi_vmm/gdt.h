#ifndef __GDT_H__
#define __GDT_H__

#include <efi.h>

#include "types.h"

struct gdt_entry {
  uint32_t base;
  uint32_t limit;
  uint8_t access;
  uint8_t granularity;
};

void gdt_create_guest_gdt(void);

void gdt_print_guest(void);

void gdt_get_guest_entry(uint64_t selector, struct gdt_entry *gdt_entry);

uint64_t gdt_get_guest_base(void);

uint64_t gdt_get_guest_limit(void);

#endif
