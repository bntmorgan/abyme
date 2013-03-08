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

void cpu_write_cr3(uintptr_t reg);

uint32_t cpu_get_seg_desc_base(uintptr_t gdt_base, uint16_t seg_sel);

#ifdef __X86_64__
void cpu_enable_vmxe(void);
#endif

#ifdef __X86_64__
void cpu_enable_ne(void);
#endif

void cpu_vmxon(uint8_t *region);

void cpu_vmclear(uint8_t *region);

void cpu_vmptrld(uint8_t *region);

void cpu_vmlaunch(void);

void cpu_vmresume(void);

#ifdef __X86_64__
void cpu_vmwrite(uint32_t field, uint32_t value);
#endif

#ifdef __X86_64__
uint32_t cpu_vmread(uint32_t field);
#endif

void cpu_stop(void);

uint32_t cpu_adjust32(uint32_t value, uint32_t msr);

#ifdef __X86_64__
uintptr_t cpu_adjust64(uintptr_t value, uint32_t fixed0_msr, uint32_t fixed1_msr);
#endif

#ifndef __X86_64__
void cpu_write_gdt(uint32_t gdt_ptr, uint32_t code_seg, uint32_t data_seg);
#endif

#define CPU_READ_EIP() ({                            \
    uint32_t reg;                                    \
    __asm__ __volatile__("mov $., %0" : "=a" (reg)); \
    reg;                                             \
  })

void cpu_stop(void);

#ifndef __X86_64__
void cpu_print_info(void);
#endif

#ifndef __X86_64__
uint8_t cpu_is_paging_enabled(void);
#endif

#ifndef __X86_64__
uint8_t cpu_is_protected_mode_enabled(void);
#endif

#ifndef __X86_64__
void cpu_enable_pae(void);
#endif

#ifndef __X86_64__
void cpu_enable_paging(void);
#endif

void cpu_enable_long_mode(void);

uint8_t cpu_is_ept_supported(void);

uint8_t cpu_is_unrestricted_guest_supported(void);

#endif
