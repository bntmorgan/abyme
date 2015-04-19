#include "idt.h"
#include "cpu.h"
#include "string.h"
#include "stdio.h"

extern void *isr;

void idt_debug_bios(void) {
  struct idt_ptr p;
  INFO("BIOS idt\n");
  cpu_read_idt((uint8_t *)&p);
  idt_dump(&p);
}

void idt_debug_host(void) {
  struct idt_ptr p;
  INFO("HOST idt\n");
  idt_get_idt_ptr(&p);
  idt_dump(&p);
}

// VMM IDT
struct idt_gdsc idt[IDT_LOW_IT] __attribute((aligned(0x8)));

void idt_get_idt_ptr(struct idt_ptr *ptr) {
  ptr->base = (uint64_t)&idt[0];
  ptr->limit = IDT_LOW_IT * sizeof(struct idt_gdsc) - 1;
}

void idt_print_gdsc(struct idt_gdsc *gdsc) {
  INFO("o0(0x%04x), ss(0x%04x), ist(0x%01x), t(0x%01x), dpl(0x%01x), p(%d), \
o1(0x%04x), o2(0x%08x)\n", gdsc->o0, gdsc->cs, gdsc->ist, gdsc->t, gdsc->dpl,
    gdsc->p, gdsc->o1, gdsc-> o2);
}

void idt_create(void) {
  uint32_t i;
  uint64_t a = (uint64_t)&isr;
  uint16_t cs = cpu_read_cs() & 0xf8;
  memset(&idt[0], 0, sizeof(struct idt_gdsc) * IDT_LOW_IT);
  INFO("isr start address(0x%016X)\n", a);
  INFO("cs(0x%04x)\n", cs);
  // isr handler are aligned on 16
  for (i = 0; i < IDT_LOW_IT; i++, a += 16) {
    struct idt_gdsc *gdsc = &idt[i];
    // set the 3 parts address
    gdsc->o0 = (a >> 0) & 0xffff;
    gdsc->o1 = (a >> 16) & 0xffff;
    gdsc->o2 = (a >> 32) & 0xffffffff;
    // Interrupt gate descriptor type
    gdsc->t = IDT_TYPE_IT;
    // DPL ring 0
    gdsc->dpl = 0x0;
    // Present
    gdsc->p = 0x1;
    // Code segment selector
    gdsc->cs = cs;
  }
}

void idt_dump(struct idt_ptr *p) {
  uint64_t b;
  uint32_t nb_other = 0, nb_it = 0, nb_np = 0;
  INFO(">>>> IDT\n");
  for (b = p->base; b < p->base + p->limit; b += 0x10) {
    struct idt_gdsc *gdsc = (struct idt_gdsc *) b;
    // idt_print_gdsc(gdsc);
    if (gdsc->p == 1) {
      if ((gdsc->t & IDT_TYPE_IT) == IDT_TYPE_IT) {
        nb_it++;
      } else {
        nb_other++;
      }
    } else {
      nb_np++;
    }
  }
  INFO("First idt entry\n");
  idt_print_gdsc((struct idt_gdsc *)p->base);
  INFO("idt_ptr : base(0x%08x), limit(0x%04x)\n", p->base, p->limit);
  INFO("nb_it(0x%08x), nb_other(0x%08x), nb_np(0x%08x)\n", nb_it, nb_other,
      nb_np);
  INFO("<<<< IDT\n");
}

void interrupt_handler(struct idt_isr_stack is) {
  INFO("ISR int number 0x%x\n", is.number);
  INFO("ISR ERROR CODE 0x%x\n", is.error_code);
  INFO("ISR RIP 0x%x\n", is.rip);
  INFO("ISR cs 0x%x\n", is.cs);
  INFO("ISR rsp 0x%x\n", is.rsp);
  INFO("ISR ss 0x%x\n", is.ss);
}
