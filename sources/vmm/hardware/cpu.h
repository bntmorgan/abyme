#ifndef __CPU_H__
#define __CPU_H__

#include "types.h"

/* BOCHS magic breakpoint */
#define BREAKPOINT() __asm__ __volatile__("xchg %bx, %bx")

typedef struct {
  uint64_t rip;
  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t r11;
  uint64_t r10;
  uint64_t r9;
  uint64_t r8;
  uint64_t rdi;
  uint64_t rsi;
  uint64_t rbp;
  uint64_t rsp;
  uint64_t rbx;
  uint64_t rdx;
  uint64_t rcx;
  uint64_t rax;
} __attribute__((packed)) gpr64_t;

void cpu_outportb(uint32_t port, uint8_t value);

uint8_t cpu_inportb(uint32_t port);

void cpu_read_gdt(uint8_t *gdt_ptr);

void cpu_read_idt(uint8_t *idt_ptr);

uint64_t cpu_read_cr0(void);

uint64_t cpu_read_cr3(void);

uint64_t cpu_read_cr4(void);

uint64_t cpu_read_cs(void);

uint64_t cpu_read_ss(void);

uint64_t cpu_read_ds(void);

uint64_t cpu_read_es(void);

uint64_t cpu_read_fs(void);

uint64_t cpu_read_gs(void);

uint64_t cpu_read_tr(void);

void cpu_write_cr0(uint64_t reg);

void cpu_write_cr4(uint64_t reg);

uint32_t cpu_get_seg_desc_base(uint64_t gdt_base, uint16_t seg_sel);

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

uint64_t cpu_adjust64(uint64_t value, uint32_t fixed0_msr, uint32_t fixed1_msr);

#endif
