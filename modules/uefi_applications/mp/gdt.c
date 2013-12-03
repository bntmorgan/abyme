#include "gdt.h"

#include "cpu.h"
#include "stdio.h"
#include "string.h"

#define VMEM_GDT_SIZE 0x48

uint8_t host_gdt[VMEM_GDT_SIZE] __attribute((aligned(0x4)));
struct gdt_ptr_64 host_gdt_ptr;
struct gdt_ptr_64 guest_gdt_ptr;

void gdt_copy_entry(uint8_t *gdt_desc, struct gdt_entry *entry) {
  entry->base         = (*((uint16_t *) (gdt_desc + 2)) <<  0) & 0x0000ffff;
  entry->base        |= (*((uint8_t *)  (gdt_desc + 4)) << 16) & 0x00ff0000;
  entry->base        |= (*((uint8_t *)  (gdt_desc + 7)) << 24) & 0xff000000;
  entry->limit        = (*((uint16_t *) (gdt_desc + 0)) <<  0) & 0x0000ffff;
  entry->limit       |= (*((uint8_t *)  (gdt_desc + 6)) << 16) & 0x000f0000;
  entry->granularity  = (*((uint8_t *)  (gdt_desc + 6)) <<  0) & 0x000000f0;
  entry->access       = (*((uint8_t *)  (gdt_desc + 5)) <<  0) & 0x000000ff;
}

void gdt_copy_desc(struct gdt_entry *entry, uint8_t *gdt_desc) {
  *((uint16_t *) (gdt_desc + 2))  = (entry->base        >>  0) & 0x0000ffff;
  *((uint8_t *)  (gdt_desc + 4))  = (entry->base        >> 16) & 0x000000ff;
  *((uint8_t *)  (gdt_desc + 7))  = (entry->base        >> 24) & 0x000000ff;
  *((uint16_t *) (gdt_desc + 0))  = (entry->limit       >>  0) & 0x0000ffff;
  *((uint8_t *)  (gdt_desc + 6))  = (entry->limit       >> 16) & 0x0000000f;
  *((uint8_t *)  (gdt_desc + 6)) |= (entry->granularity >>  0) & 0x000000f0;
  *((uint8_t *)  (gdt_desc + 5))  = (entry->access      >>  0) & 0x000000ff;
}

void gdt_setup_guest_gdt(void) {
  cpu_read_gdt((uint8_t *) &guest_gdt_ptr);
}

void gdt_setup_host_gdt(void) {
  struct gdt_ptr_64 previous_gdt_ptr;
  cpu_read_gdt((uint8_t *) &previous_gdt_ptr);
  host_gdt_ptr.base = (uint64_t) &host_gdt[0];
  host_gdt_ptr.limit = previous_gdt_ptr.limit;
  if (host_gdt_ptr.limit >= VMEM_GDT_SIZE) {
    panic("!#GDT SZ [%d]", host_gdt_ptr.limit);
  }
  /* Test the entries: we don't manage 16 bytes entries or non flat-entries. */
  struct gdt_entry entry;
  uint8_t *gdt_desc = (uint8_t *) host_gdt_ptr.base;
  while (gdt_desc < (uint8_t *) host_gdt_ptr.base + host_gdt_ptr.limit) {
    gdt_copy_entry(gdt_desc, &entry);
    if (*((uint64_t *) gdt_desc) != 0x0) {
      if (!((entry.access >> 4) & 0x1)) {
        panic("!#GDT SD");
      }
      if (entry.base != 0x0) {
        panic("!#GDT FLAT");
      }
    }
    gdt_desc = gdt_desc + 8;
  }
  memcpy(host_gdt, (uint8_t *) previous_gdt_ptr.base, previous_gdt_ptr.limit);
}

void gdt_print_host_gdt(void) {
  struct gdt_entry entry;
  uint8_t *gdt_desc = (uint8_t *) host_gdt_ptr.base;
  printk("gdt: base=%08X limit=%8x\n", (uint64_t) host_gdt_ptr.base, host_gdt_ptr.limit);
  while (gdt_desc < (uint8_t *) host_gdt_ptr.base + host_gdt_ptr.limit) {
    gdt_copy_entry(gdt_desc, &entry);
    printk("B:%08x L:%08x A:%02x G:%02x\n", entry.base, entry.limit, entry.access, entry.granularity);
    gdt_desc = gdt_desc + 8;
  }
}

void gdt_get_host_entry(uint64_t selector, struct gdt_entry *gdt_entry) {
  gdt_copy_entry((uint8_t *) host_gdt_ptr.base + selector, gdt_entry);
}

uint64_t gdt_get_host_base(void) {
  return host_gdt_ptr.base;
}

uint64_t gdt_get_host_limit(void) {
  return host_gdt_ptr.limit;
}

void gdt_get_guest_entry(uint64_t selector, struct gdt_entry *gdt_entry) {
  gdt_copy_entry((uint8_t *) guest_gdt_ptr.base + selector, gdt_entry);
}

uint64_t gdt_get_guest_base(void) {
  return guest_gdt_ptr.base;
}

uint64_t gdt_get_guest_limit(void) {
  return guest_gdt_ptr.limit;
}
