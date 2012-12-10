#ifndef __CPU_H__
#define __CPU_H__

#include <stdint.h>

typedef struct {
  uint64_t rax;
  uint64_t rcx;
  uint64_t rdx;
  uint64_t rbx;
  uint64_t rsp;
  uint64_t rbp;
  uint64_t rsi;
  uint64_t rdi;
  uint64_t r8;
  uint64_t r9;
  uint64_t r10;
  uint64_t r11;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;
} __attribute__((packed)) gpr64_t;

void cpu_outportb(uint32_t port, uint8_t value);

void cpu_read_gdt(uint8_t *gdt_ptr);

void cpu_read_idt(uint8_t *idt_ptr);

void cpu_read_cr0(uint64_t *reg);

void cpu_read_cr3(uint64_t *reg);

void cpu_read_cr4(uint64_t *reg);

void cpu_read_cs(uint64_t *reg);

void cpu_read_ss(uint64_t *reg);

void cpu_read_ds(uint64_t *reg);

void cpu_read_es(uint64_t *reg);

void cpu_read_fs(uint64_t *reg);

void cpu_read_gs(uint64_t *reg);

void cpu_read_tr(uint64_t *reg);

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

void cpu_stop(void);

#endif
