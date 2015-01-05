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

void cpu_enable_ne(void);

void cpu_stop(void);

uint64_t cpu_read_flags(void);

uint64_t cpu_read_tsc(void);

inline void cpu_outportb(uint32_t port, uint8_t value);

inline void cpu_outportw(uint32_t port, uint16_t value);

inline void cpu_outportd(uint32_t port, uint32_t value);

inline uint8_t cpu_inportb(uint32_t port);

inline uint16_t cpu_inportw(uint32_t port);

inline uint32_t cpu_inportd(uint32_t port);

inline void cpu_mem_writeb(void *p, uint8_t data);

inline uint8_t cpu_mem_readb(void *p);

inline void cpu_mem_writew(void *p, uint16_t data);

inline uint16_t cpu_mem_readw(void *p);

inline void cpu_mem_writed(void *p, uint32_t data);

inline uint32_t cpu_mem_readd(void *p);

inline void cpu_mem_writeq(void *p, uint64_t data);

inline uint64_t cpu_mem_readq(void *p);

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
