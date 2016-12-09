#include "idt.h"
#include "cpu.h"
#include "string.h"
#include "stdio.h"
#include "error.h"
#include "vmm.h"
#include "apic.h"
#include "vmcs.h"

extern void *isr;

void idt_debug_bios(void) {
  struct idt_ptr p;
  INFO("BIOS idt\n");
  cpu_read_idt(&p);
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

enum idt_exceptions {
  IDT_DIVIDE_ERROR,
  IDT_DEBUG,
  IDT_NMI_INTERRUPT,
  IDT_BREAKPOINT,
  IDT_OVERFLOW,
  IDT_BOUND_RANGE_EXEEDED,
  IDT_INVALID_OPCODE,
  IDT_DEVICE_NOT_AVAILABLE,
  IDT_DOUBLE_FAULT,
  IDT_COPROCESSOR_SEGMENT_OVERRUN,
  IDT_INVALID_TSS,
  IDT_SEGMENT_NOT_PRESENT,
  IDT_STACK_SEGMENT_FAULT,
  IDT_GENERAL_PROTECTION,
  IDT_PAGE_FAULT,
  IDT_RESERVED,
  IDT_FLOATING_POINT_ERROR,
  IDT_ALIGNMENT_CHECK,
  IDT_MACHINE_CHECK,
  IDT_SMID_FLOATING_POINT_EXCEPTION
};

// Host pointer
void idt_get_idt_ptr(struct idt_ptr *ptr) {
  ptr->base = (uint64_t)&idt[0];
  ptr->limit = IDT_LOW_IT * sizeof(struct idt_gdsc) - 1;
}

void idt_print_gdsc(struct idt_gdsc *gdsc) {
  INFO("o0(0x%04x), ss(0x%04x), ist(0x%01x), t(0x%01x), dpl(0x%01x), p(%d), \
o1(0x%04x), o2(0x%08x)\n", gdsc->o0, gdsc->cs, gdsc->ist, gdsc->t, gdsc->dpl,
    gdsc->p, gdsc->o1, gdsc-> o2);
}

struct idt_ptr guest_idt_ptr;

void idt_create(void) {
	// Host
  uint32_t i;
  uint64_t a = (uint64_t)&isr;
  uint16_t cs = cpu_read_cs() & 0xf8;
  memset(&idt[0], 0, sizeof(struct idt_gdsc) * IDT_LOW_IT);
  INFO("isr start address(0x%016X)\n", a);
  INFO("cs(0x%04x)\n", cs);
  // isr handler are aligned on 32
  for (i = 0; i < IDT_LOW_IT; i++, a += 32) {
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
	// Guest firmware copy TODO
	cpu_read_idt(&guest_idt_ptr);
	idt_dump(&guest_idt_ptr);
	struct idt_ptr host_idt_ptr;
	idt_get_idt_ptr(&host_idt_ptr);
	idt_dump(&host_idt_ptr);
	cpu_write_idt(&host_idt_ptr);
}

void idt_get_guest_idt_ptr(struct idt_ptr *p) {
  p->base = guest_idt_ptr.base;
  p->limit = guest_idt_ptr.limit;
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

#define IDT_PRINT(__T__) \
   INFO(#__T__ "\n");

void idt_decode(struct idt_isr_stack *is) {
	INFO("int number 0x%x\n", is->number);
  INFO("ERROR CODE 0x%x\n", is->error_code);
  INFO("RIP 0x%x\n", is->rip);
  INFO("cs 0x%x\n", is->cs);
  INFO("rsp 0x%x\n", is->rsp);
  INFO("ss 0x%x\n", is->ss);
  INFO("RFLAGS 0x%016x\n", cpu_read_flags());
  switch(is->number) {
    case IDT_DIVIDE_ERROR:
      IDT_PRINT(IDT_DIVIDE_ERROR);
      break;
    case IDT_DEBUG:
      IDT_PRINT(IDT_DEBUG);
      break;
    case IDT_NMI_INTERRUPT:
      IDT_PRINT(IDT_NMI_INTERRUPT);
      break;
    case IDT_BREAKPOINT:
      IDT_PRINT(IDT_BREAKPOINT);
      break;
    case IDT_OVERFLOW:
      IDT_PRINT(IDT_OVERFLOW);
      break;
    case IDT_BOUND_RANGE_EXEEDED:
      IDT_PRINT(IDT_BOUND_RANGE_EXEEDED);
      break;
    case IDT_INVALID_OPCODE:
      IDT_PRINT(IDT_INVALID_OPCODE);
      break;
    case IDT_DEVICE_NOT_AVAILABLE:
      IDT_PRINT(IDT_DEVICE_NOT_AVAILABLE);
      break;
    case IDT_DOUBLE_FAULT:
      IDT_PRINT(IDT_DOUBLE_FAULT);
      break;
    case IDT_COPROCESSOR_SEGMENT_OVERRUN:
      IDT_PRINT(IDT_COPROCESSOR_SEGMENT_OVERRUN);
      break;
    case IDT_INVALID_TSS:
      IDT_PRINT(IDT_INVALID_TSS);
      break;
    case IDT_SEGMENT_NOT_PRESENT:
      IDT_PRINT(IDT_SEGMENT_NOT_PRESENT);
      break;
    case IDT_STACK_SEGMENT_FAULT:
      IDT_PRINT(IDT_STACK_SEGMENT_FAULT);
      break;
    case IDT_GENERAL_PROTECTION:
      IDT_PRINT(IDT_GENERAL_PROTECTION);
      ERROR("Unsupported\n");
      break;
    case IDT_PAGE_FAULT: {
      IDT_PRINT(IDT_PAGE_FAULT);
      uint64_t cr2 = cpu_read_cr2();
      union idt_error_code_page_fault code = {.raw = is->error_code};
      INFO("Access to @0x%016X\n", cr2);
      INFO("Error code:");
      PRINT_FIELD(&code, p);
      PRINT_FIELD(&code, wr);
      PRINT_FIELD(&code, us);
      PRINT_FIELD(&code, rsvd);
      PRINT_FIELD(&code, id);
      PRINT_FIELD(&code, pk);
      PRINT_FIELD(&code, r0);
      ERROR("Unsupported\n");
      break;
    }
    case IDT_RESERVED:
      IDT_PRINT(IDT_RESERVED);
      break;
    case IDT_FLOATING_POINT_ERROR:
      IDT_PRINT(IDT_FLOATING_POINT_ERROR);
      break;
    case IDT_ALIGNMENT_CHECK:
      IDT_PRINT(IDT_ALIGNMENT_CHECK);
      break;
    case IDT_MACHINE_CHECK:
      IDT_PRINT(IDT_MACHINE_CHECK);
      break;
    case IDT_SMID_FLOATING_POINT_EXCEPTION:
      IDT_PRINT(IDT_SMID_FLOATING_POINT_EXCEPTION);
      break;
    case 0xef: { // Lets assume that is the Local APIC timer
      INFO("YOLO 0xef\n");
      INFO("Event injection for the local APIC timer\n");
      vm_interrupt_set(is->number, 0, is->error_code); // Type is external
      // and fire for the vmresume
      vm_interrupt_inject();
      // XXX
      vmcs_update();
      vmcs_dump(vmcs);
      // VMR(ctrls.ex.secondary_vm_exec_control);
      // vmcs->ctrls.ex.secondary_vm_exec_control.virtual_interrupt_delivery = 1;
      // VMD(ctrls.ex.secondary_vm_exec_control);
//       if (apic_is_vector_apic_timer(is->number)) {
//         apic_emulate_apic_timer_expiration();
//       }
      break;
    }
  }
}

void interrupt_handler(struct idt_isr_stack is) {
  idt_decode(&is);
}
