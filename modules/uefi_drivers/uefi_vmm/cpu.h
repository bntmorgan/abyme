#ifndef __CPU_H__
#define __CPU_H__

#include <efi.h>
#include "types.h"

void cpu_read_gdt(uint8_t *gdt_ptr);

void cpu_read_idt(uint8_t *idt_ptr);

uint64_t cpu_read_cr0(void);

uint64_t cpu_read_cr2(void);

uint64_t cpu_read_cr3(void);

uint64_t cpu_read_cr4(void);

uint64_t cpu_read_cs(void);

uint64_t cpu_read_ss(void);

uint64_t cpu_read_ds(void);

uint64_t cpu_read_es(void);

uint64_t cpu_read_fs(void);

uint64_t cpu_read_gs(void);

uint64_t cpu_read_tr(void);

uint64_t cpu_read_ldtr(void);

uint64_t cpu_read_dr7(void);

void cpu_write_cr0(uint64_t reg);

void cpu_write_cr3(uint64_t reg);

void cpu_write_cr4(uint64_t reg);

void cpu_enable_vmxe(void);

void cpu_enable_ne(void);

void cpu_vmxon(uint8_t *region);

void cpu_vmclear(uint8_t *region);

void cpu_vmptrld(uint8_t *region);

void cpu_vmlaunch(void);

void cpu_vmwrite(uint64_t field, uint64_t value);

uint64_t cpu_vmread(uint64_t field);

uint8_t cpu_vmread_safe(unsigned long field, unsigned long *value);

void cpu_stop(void);

uint32_t cpu_adjust32(uint32_t value, uint32_t msr);

uint64_t cpu_adjust64(uint64_t value, uint32_t msr_fixed0, uint32_t msr_fixed1);

uint64_t cpu_read_flags(void);

///* BOCHS magic breakpoint */
////#define BREAKPOINT() __asm__ __volatile__("xchg %bx, %bx")
//void cpu_outportb(uint32_t port, uint8_t value);
//uint8_t cpu_inportb(uint32_t port);
//void cpu_write_cr3(uint64_t reg);
//uint32_t cpu_get_seg_desc_base(uint64_t gdt_base, uint16_t seg_sel);
//void cpu_vmresume(void);
//uint8_t cpu_is_paging_enabled(void);
//uint8_t cpu_is_protected_mode_enabled(void);
//void cpu_enable_pae(void);
//void cpu_enable_paging(void);
//void cpu_enable_long_mode(void);
//uint8_t cpu_is_ept_supported(void);
//uint8_t cpu_is_unrestricted_guest_supported(void);
//uint64_t cpu_read_rsp(void);

#endif
