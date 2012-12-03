#ifndef __CPU_H__
#define __CPU_H__

#include <stdint.h>

void cpu_outportb(uint32_t port, uint8_t value);

void cpu_read_gdt(uint8_t *gdt_ptr);

void cpu_read_cr3(uint64_t *reg);

void cpu_read_cr0(uint64_t *reg);

void cpu_read_cr4(uint64_t *reg);

void cpu_write_cr0(uint64_t reg);

void cpu_write_cr4(uint64_t reg);

void cpu_enable_vmxe(void);

void cpu_enable_ne(void);

void cpu_vmxon(uint8_t *region);

void cpu_vmwrite(uint32_t field, uint32_t value);

void cpu_stop(void);

#endif
