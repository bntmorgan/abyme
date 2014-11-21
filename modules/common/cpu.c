#include "cpu.h"

#include "msr.h"
#include "stdio.h"

// TODO: parce que cpu.c utilise vmcs, il faudrait tout mettre a plat dans le repertoire...
// #include "vmcs.h"

void cpu_read_gdt(uint8_t *gdt_ptr) {
  __asm__ __volatile__("sgdt %0" : : "m" (*gdt_ptr) : "memory");
}

void cpu_read_idt(uint8_t *idt_ptr) {
  __asm__ __volatile__("sidt %0" : : "m" (*idt_ptr) : "memory");
}

uint64_t cpu_read_cr0(void) {
  uint64_t reg;
  __asm__ __volatile__("mov %%cr0, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_cr2(void) {
  uint64_t reg;
  __asm__ __volatile__("mov %%cr2, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_cr3(void) {
  uint64_t reg;
  __asm__ __volatile__("mov %%cr3, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_cr4(void) {
  uint64_t reg;
  __asm__ __volatile__("mov %%cr4, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_cs(void) {
  uint64_t reg;
  __asm__ __volatile__("mov %%cs, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_ldtr(void) {
  uint64_t reg;
  __asm__ __volatile__("sldt %%eax" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_dr7(void) {
  uint64_t reg;
  __asm__ __volatile__("mov %%dr7, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_ss(void) {
  uint64_t reg;
  __asm__ __volatile__("mov %%ss, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_ds(void) {
  uint64_t reg;
  __asm__ __volatile__("mov %%ds, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_es(void) {
  uint64_t reg;
  __asm__ __volatile__("mov %%es, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_fs(void) {
  uint64_t reg;
  __asm__ __volatile__("mov %%fs, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_gs(void) {
  uint64_t reg;
  __asm__ __volatile__("mov %%gs, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_tr(void) {
  uint64_t reg;
  __asm__ __volatile__("str %0" : "=a" (reg));
  return reg;
}

void cpu_write_cr0(uint64_t value) {
  __asm__ __volatile__("mov %0, %%cr0" : : "a" (value));
}

void cpu_write_cr3(uint64_t value) {
  __asm__ __volatile__("mov %0, %%cr3" : : "a" (value));
}

void cpu_write_cr4(uint64_t value) {
  __asm__ __volatile__("mov %0, %%cr4" : : "a" (value));
}

void cpu_enable_ne(void) {
  cpu_write_cr0(cpu_read_cr0() | 0x00000020);
}

uint64_t cpu_read_flags(void) {
  uint64_t flags;
  __asm__ __volatile__(
      "pushfq     ;\n"
      "popq %%rax ;\n"
    : "=a"(flags));
  return flags;
}

void cpu_stop(void) {
  __asm__ __volatile__("cli");
  __asm__ __volatile__("hlt");
  while (1);
}

inline void cpu_outportb(uint32_t port, uint8_t value) {
  __asm__ __volatile__(
      "out %%al, %%dx;"
      : : "a"(value), "d"(port));
}

inline void cpu_outportw(uint32_t port, uint16_t value) {
  __asm__ __volatile__(
      "out %%ax, %%dx;"
      : : "a"(value), "d"(port));
}

inline void cpu_outportd(uint32_t port, uint32_t value) {
  __asm__ __volatile__(
      "out %%eax, %%dx;"
      : : "a"(value), "d"(port));
}

inline uint8_t cpu_inportb(uint32_t port) {
  uint8_t value;
  __asm__ __volatile__(
      "in %%dx, %%al;"
      : "=a"(value) : "d"(port));
  return value;
}

inline uint16_t cpu_inportw(uint32_t port) {
  uint16_t value;
  __asm__ __volatile__(
      "in %%dx, %%ax;"
      : "=a"(value) : "d"(port));
  return value;
}

inline uint32_t cpu_inportd(uint32_t port) {
  uint32_t value;
  __asm__ __volatile__(
      "in %%dx, %%eax;"
      : "=a"(value) : "d"(port));
  return value;
}

inline void cpu_mem_writeb(void *p, uint8_t data) {
  *(volatile uint8_t *)(p) = data;
}

inline uint8_t cpu_mem_readb(void *p) {
  return *(volatile uint8_t *)(p);
}

inline void cpu_mem_writew(void *p, uint16_t data) {
  *(volatile uint16_t *)(p) = data;
}

inline uint16_t cpu_mem_readw(void *p) {
  return *(volatile uint16_t *)(p);
}

inline void cpu_mem_writed(void *p, uint32_t data) {
  *(volatile uint32_t *)(p) = data;
}

inline uint32_t cpu_mem_readd(void *p) {
  return *(volatile uint32_t *)(p);
}

inline void cpu_mem_writeq(void *p, uint64_t data) {
  *(volatile uint64_t *)(p) = data;
}

inline uint64_t cpu_mem_readq(void *p) {
  return *(volatile uint64_t *)(p);
}

uint64_t cpu_read_tsc(void) {
  uint32_t tscl, tsch;
  __asm__ __volatile__("rdtsc" : "=d"(tsch), "=a"(tscl));  
  return ((uint64_t)tsch) << 32 | tscl;
}
