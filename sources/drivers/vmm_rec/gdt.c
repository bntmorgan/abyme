#include "gdt.h"

#include "cpu.h"
#include "efiw.h"
#include "stdio.h"
#include "string.h"

#define VMEM_GDT_SIZE 3

struct segment_descriptor *host_gdt;
struct segment_descriptor *guest_gdt;
struct gdt_ptr_64 host_gdt_ptr;
struct gdt_ptr_64 guest_gdt_ptr;
uint32_t gdt_legacy_segment_descriptor = 0;

void gdt_setup_guest_gdt(void) {
  cpu_read_gdt((uint8_t *)&guest_gdt_ptr);
  guest_gdt = (struct segment_descriptor*)guest_gdt_ptr.base;
}

void gdt_init(void) {
  host_gdt = efi_allocate_pages(1);
  memset(host_gdt, 0, 0x1000);
}

void gdt_setup_host_gdt(void) {
  struct gdt_ptr_64 current_gdt_ptr;
  // Read the current GDT ptr
  cpu_read_gdt((uint8_t*)&current_gdt_ptr);
  // Copy the current GDT
  memcpy(host_gdt, (uint8_t *)current_gdt_ptr.base, current_gdt_ptr.limit + 1);
  // Set host gdt_ptr
  host_gdt_ptr.base = (uint64_t)host_gdt;
  host_gdt_ptr.limit = current_gdt_ptr.limit;
  INFO("Current GDT ptr limit 0x%x\n", host_gdt_ptr.limit);
  // This GDT must be valid because we are in long mode. But TODO we need to
  // check the descriptors (flatness, types and rights)
  // We add a legacy code segment descriptor (to go down from long mode)
  gdt_legacy_segment_descriptor = ((host_gdt_ptr.limit + 1) >> 3);
  INFO("New descritpor index 0x%x\n", gdt_legacy_segment_descriptor);
  struct segment_descriptor *dsc = &host_gdt[gdt_legacy_segment_descriptor];
  // Flat code segment in legacy mode
  dsc->limit0 = 0xffff;
  dsc->limit1 = 0xf;
  dsc->t = SEGMENT_DESCRIPTOR_TYPE_CODE;
  dsc->a = 1; // XXX lol why ?
  dsc->e = 1; // Expand down
  dsc->w = 1; // Read/Write
  dsc->s = 1; // Usable by the system
  dsc->p = 1; // Present
  dsc->g = 1; // limit << 12
  // Add the segment
  host_gdt_ptr.limit += 8;
  INFO("New GDT ptr limit 0x%x\n", host_gdt_ptr.limit);
}

void gdt_print_gdt(struct gdt_ptr_64 *gdt_ptr) {
  uint32_t i;
  struct segment_descriptor *dsc= (struct segment_descriptor *)gdt_ptr->base;
  struct segment_descriptor *d;
  INFO("SIZE OF 0x%x\n", sizeof(struct segment_descriptor));
  INFO("gdt: base=%08X limit=%08x\n", (uint64_t) gdt_ptr->base,
      gdt_ptr->limit);
  for (i = 0; i < ((gdt_ptr->limit + 1) >> 3); i++) {
    d = &dsc[i];
    printk("  Entry 0x%x: ", i);
    printk("base(0x%08x), ", gdt_descriptor_get_base(d));
    printk("limit(0x%05x), ", gdt_descriptor_get_limit(d));
    printk("a(0x%x), ", d->a);
    if (d->t == SEGMENT_DESCRIPTOR_TYPE_DATA) {
      printk("r(0x%x), ", d->r);
      printk("c(0x%x), ", d->c);
    } else { // CODE
      printk("w(0x%x), ", d->w);
      printk("e(0x%x), ", d->e);
    }
    printk("type(0x%x),\n", d->t);
    printk("    s(0x%x), ", d->s);
    printk("dpl(0x%x), ", d->dpl);
    printk("p(0x%x), ", d->p);
    printk("avl(0x%x), ", d->avl);
    printk("l(0x%x), ", d->l);
    printk("d/b(0x%x), ", d->b);
    printk("g(0x%x)\n", d->g);
  }
}

void gdt_print_host_gdt(void) {
  gdt_print_gdt(&host_gdt_ptr);
}

uint32_t gdt_descriptor_get_base(struct segment_descriptor *dsc) {
  return dsc->base0 | ((uint32_t)dsc->base1 << 16) | ((uint32_t)dsc->base2 << 24);
}

uint32_t gdt_descriptor_get_limit(struct segment_descriptor *dsc) {
  return (dsc->limit0 | ((uint32_t)(dsc->limit1 & 0xf) << 16)) & 0xfffff;
}

void gdt_get_entry(struct segment_descriptor *gdt, uint64_t selector, struct
    segment_descriptor **dsc) {
  *dsc =  gdt + (selector >> 3);
}

void gdt_get_host_entry(uint16_t selector, struct segment_descriptor **dsc) {
  gdt_get_entry(host_gdt, selector, dsc);
}

uint64_t gdt_get_host_base(void) {
  return host_gdt_ptr.base;
}

uint64_t gdt_get_host_limit(void) {
  return host_gdt_ptr.limit;
}

void gdt_get_guest_entry(uint64_t selector, struct segment_descriptor **dsc) {
  gdt_get_entry(guest_gdt, selector, dsc);
}

uint64_t gdt_get_guest_base(void) {
  return guest_gdt_ptr.base;
}

uint64_t gdt_get_guest_limit(void) {
  return guest_gdt_ptr.limit;
}
