#ifndef __CPU_H__
#define __CPU_H__

#include "types.h"

/* BOCHS magic breakpoint */
#define BREAKPOINT() __asm__ __volatile__("xchg %bx, %bx")

typedef struct {
  uintptr_t rip;
  uintptr_t r15;
  uintptr_t r14;
  uintptr_t r13;
  uintptr_t r12;
  uintptr_t r11;
  uintptr_t r10;
  uintptr_t r9;
  uintptr_t r8;
  uintptr_t rdi;
  uintptr_t rsi;
  uintptr_t rbp;
  uintptr_t rsp;
  uintptr_t rbx;
  uintptr_t rdx;
  uintptr_t rcx;
  uintptr_t rax;
} __attribute__((packed)) gpr64_t;

void cpu_outportb(uint32_t port, uint8_t value);

uint8_t cpu_inportb(uint32_t port);

void cpu_read_gdt(uint8_t *gdt_ptr);

void cpu_read_idt(uint8_t *idt_ptr);

uintptr_t cpu_read_cr0(void);

uintptr_t cpu_read_cr3(void);

uintptr_t cpu_read_cr4(void);

uintptr_t cpu_read_cs(void);

uintptr_t cpu_read_ss(void);

uintptr_t cpu_read_ds(void);

uintptr_t cpu_read_es(void);

uintptr_t cpu_read_fs(void);

uintptr_t cpu_read_gs(void);

uintptr_t cpu_read_tr(void);

void cpu_write_cr0(uintptr_t reg);

void cpu_write_cr4(uintptr_t reg);

uint32_t cpu_get_seg_desc_base(uintptr_t gdt_base, uint16_t seg_sel);

void cpu_enable_vmxe(void);

void cpu_enable_ne(void);

void cpu_vmxon(uint8_t *region);

void cpu_vmclear(uint8_t *region);

void cpu_vmptrld(uint8_t *region);

void cpu_vmlaunch(void);

void cpu_vmresume(void);

void cpu_vmwrite(uint32_t field, uint32_t value);

uint32_t cpu_vmread(uint32_t field);

void cpu_stop(void);

uint32_t cpu_adjust32(uint32_t value, uint32_t msr);

uintptr_t cpu_adjust64(uintptr_t value, uint32_t fixed0_msr, uint32_t fixed1_msr);

#endif
