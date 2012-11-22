#include "cpu_int.h"

#include "common/string_int.h"

void cpu_outportb(uint32_t port, uint8_t value) {
  __asm__ __volatile__("outb %%al,%%dx"::"d" (port), "a" (value));
}

void cpu_read_gdt(uint8_t *gdt_ptr) {
  __asm__ __volatile__("sgdt %0" : : "m" (*gdt_ptr) : "memory");
}

void cpu_read_cr3(uint64_t *reg) {
  __asm__ __volatile__("mov %%cr3, %0" : "=a" (*reg));
}

void cpu_read_cr4(uint64_t *reg) {
  __asm__ __volatile__("mov %%cr4, %0" : "=a" (*reg));
}

void cpu_read_cr0(uint64_t *reg) {
  __asm__ __volatile__("mov %%cr0, %0" : "=a" (*reg));
}

void cpu_write_cr0(uint64_t value) {
  __asm__ __volatile__("mov %%rax, %%cr0" : : "a"(value));
}

void cpu_write_cr4(uint64_t value) {
  __asm__ __volatile__("mov %%rax, %%cr4" : : "a"(value));
}

void cpu_enable_ne(void) {
  /*
   * See Volume 3, Section 2.5 of intel documentation.
   */
  uint64_t tmp;
  __asm__ __volatile__(
      "mov %%cr0, %%rax       ;"
      "or $0x00000020, %%rax ;"
      "mov %%rax, %%cr0       ;" : "=a" (tmp));
}

void cpu_enable_vmxe(void) {
  /*
   * See Volume 3, Section 2.5 of intel documentation.
   */
  uint64_t tmp;
  __asm__ __volatile__(
      "mov %%cr4, %%rax       ;"
      "or $0x00002000, %%rax ;"
      "mov %%rax, %%cr4       ;" : "=a" (tmp));
}

void cpu_vmxon(uint8_t *region) {
  INFO("vmxon region at %08x\n", (uint32_t) (uint64_t) region);
  uint8_t status = 1;
  __asm__ __volatile__(
      "vmxon (%%rdi) ;"
      "jnc _ok       ;"
      "mov $0, %%al  ;"
      "_ok:          ;"
    : : "D" (&region), "a" (status));
  if (status != 1) {
    ERROR("vmxon failed\n");
  } else {
    INFO("vmxon successful\n");
  }
}

void cpu_stop(void) {
  __asm__ __volatile__("cli");
  while (1);
}
