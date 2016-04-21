#include "cpu.h"

#include "msr.h"
#include "stdio.h"

// TODO: parce que cpu.c utilise vmcs, il faudrait tout mettre a plat dans le repertoire...
// #include "vmcs.h"

void cpu_read_gdt(uint8_t *gdt_ptr) {
  __asm__ __volatile__("sgdt %0" : : "m" (*gdt_ptr) : "memory");
}

void cpu_read_idt(struct idt_ptr *idt_ptr) {
  __asm__ __volatile__("sidt %0" : : "m" (*idt_ptr) : "memory");
}

void cpu_write_idt(struct idt_ptr *idt_ptr) {
  __asm__ __volatile__("lidt %0" : : "m" (*idt_ptr) : "memory");
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

void cpu_write_cr2(uint64_t value) {
  __asm__ __volatile__("mov %0, %%cr2" : : "a" (value));
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

uint64_t cpu_rdtsc(void) {
  uint32_t tscl, tsch;
  __asm__ __volatile__("rdtsc" : "=d"(tsch), "=a"(tscl));
  return ((uint64_t)tsch) << 32 | tscl;
}
