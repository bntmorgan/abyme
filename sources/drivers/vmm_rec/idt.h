#ifndef __IDT_H__
#define __IDT_H__

#include "types.h"
#include "cpu.h"

struct idt_gdsc {
  uint32_t o0:16;
  uint32_t cs:16;
  uint32_t ist:3;
  uint32_t zeroes:5;
  uint32_t t:4; // type
  uint32_t zeroe:1;
  uint32_t dpl:2;
  uint32_t p:1;
  uint32_t o1:16;
  uint32_t o2:32;
  uint32_t :32;
} __attribute__((packed));

struct idt_isr_stack {
  uint64_t number;
  uint8_t error_code_valid;
  uint64_t error_code;
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
} __attribute__((packed));

#define IDT_TYPE_D 3

#define IDT_TYPE_IT 0xe

#define IDT_LOW_IT 256

void idt_dump(struct idt_ptr *p);
void idt_debug_bios(void);
void idt_debug_host(void);
void idt_get_idt_ptr(struct idt_ptr *ptr);
void idt_create(void);
void idt_get_guest_idt_ptr(struct idt_ptr *p);

union idt_error_code_page_fault {
  struct {
    uint32_t p:1;
    uint32_t wr:1;
    uint32_t us:1;
    uint32_t rsvd:1;
    uint32_t id:1;
    uint32_t pk:1;
    uint32_t r0:26;
  };
  uint32_t raw;
};

#endif
