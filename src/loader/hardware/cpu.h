#ifndef __CPU_H__
#define __CPU_H__

#include "types.h"

#define CPU_READ_EIP() ({                            \
    uint32_t reg;                                    \
    __asm__ __volatile__("mov $., %0" : "=a" (reg)); \
    reg;                                             \
  })

void cpu_write_gdt(uint32_t gdt_ptr, uint32_t code_seg, uint32_t data_seg);

void cpu_read_gdt(uint32_t *gdt_ptr);

void cpu_read_cs(uint32_t *reg);

void cpu_read_ds(uint32_t *reg);

void cpu_read_cr3(uint32_t *reg);

void cpu_outportb(uint32_t port, uint8_t value);

void cpu_stop(void);

void cpu_print_info(void);

uint8_t cpu_is_paging_enabled(void);

uint8_t cpu_is_protected_mode_enabled(void);

void cpu_enable_pae(void);

void cpu_enable_paging(void);

void cpu_write_cr3(uint32_t value);

void cpu_enable_long_mode(void);

#endif
