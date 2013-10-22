#include "cpu.h"

#include "msr.h"
#include "stdio.h"

// TODO: parce que cpu.c utilise vmcs, il faudrait tout mettre a plat dans le repertoire...
#include "vmcs.h"

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

void cpu_write_cr4(uint64_t value) {
  __asm__ __volatile__("mov %0, %%cr4" : : "a" (value));
}

void cpu_enable_ne(void) {
  cpu_write_cr0(cpu_read_cr0() | 0x00000020);
}

void cpu_enable_vmxe(void) {
  cpu_write_cr4(cpu_read_cr4() | 0x00002000);
}

void cpu_vmxon(uint8_t *region) {
  uint8_t ok = 0;
  __asm__ __volatile__(
      "vmxon (%1) ;"
      "setae %%cl ;"
    : "=c" (ok) : "D" (&region));
  if (ok == 0x0) {
    panic("!#CPU VMXON");
  }
}

void cpu_vmclear(uint8_t *region) {
  uint8_t ok = 0;
  __asm__ __volatile__(
      "vmclear (%1) ;"
      "seta %%cl    ;"
    : "=c" (ok) : "D" (&region));
  if (ok == 0x0) {
    panic("!#CPU VMCLEAR");
  }
}

void cpu_vmptrld(uint8_t *region) {
  uint8_t ok = 0;
  __asm__ __volatile__(
      "vmptrld (%1) ;"
      "seta %%cl    ;"
    : "=c" (ok) : "D" (&region));
  if (ok == 0x0) {
    panic("!#CPU VMPTRLD");
  }
}

void cpu_vmlaunch(void) {
  /* Set rsp and rip in VMCS here because they depend on the implementation of this function. */
  uint64_t reg;
  __asm__ __volatile__(
      "push %rax ;\n"
      "push %rbx ;\n"
      "push %rcx ;\n"
      "push %rdx ;\n"
      "push %rsi ;\n"
      "push %rdi ;\n"
      "push %r8  ;\n"
      "push %r9  ;\n"
      "push %r10 ;\n"
      "push %r11 ;\n"
      "push %r12 ;\n"
      "push %r13 ;\n"
      "push %r14 ;\n"
      "push %r15 ;\n"
  );
  __asm__ __volatile__("mov %%rsp, %0" : "=a" (reg));
  cpu_vmwrite(GUEST_RSP, reg);
  cpu_vmwrite(GUEST_RIP, ((uint64_t) &&vm_entrypoint));
  __asm__ __volatile__("vmlaunch;\n");
  panic("!#CPU VMLAUNCH [%x]", cpu_vmread(VM_INSTRUCTION_ERROR));
vm_entrypoint:
  __asm__ __volatile__(
      "pop %r15 ;\n"
      "pop %r14 ;\n"
      "pop %r13 ;\n"
      "pop %r12 ;\n"
      "pop %r11 ;\n"
      "pop %r10 ;\n"
      "pop %r9  ;\n"
      "pop %r8  ;\n"
      "pop %rdi ;\n"
      "pop %rsi ;\n"
      "pop %rdx ;\n"
      "pop %rcx ;\n"
      "pop %rbx ;\n"
      "pop %rax ;\n"
    );
}

uint64_t cpu_read_flags(void) {
  uint64_t flags;
  __asm__ __volatile__(
      "pushfq     ;\n"
      "popq %%rax ;\n"
    : "=a"(flags));
  return flags;
}

void cpu_vmwrite(uint64_t field, uint64_t value) {
  __asm__ __volatile__("vmwrite %%rax, %%rdx" : : "a" (value), "d" (field));
}

uint64_t cpu_vmread(uint64_t field) {
  uint64_t value;
  __asm__ __volatile__("vmread %%rdx, %%rax" : "=a" (value): "d" (field));
  return value;
}

uint32_t cpu_adjust32(uint32_t value, uint32_t msr_address) {
  uint64_t msr_value = msr_read(msr_address);
  value |= (msr_value >>  0) & 0xffffffff;
  value &= (msr_value >> 32) & 0xffffffff;
  return value;
}

uint64_t cpu_adjust64(uint64_t value, uint32_t msr_fixed0, uint32_t msr_fixed1) {
  return (value & msr_read(msr_fixed1)) | msr_read(msr_fixed0);
}

void cpu_stop(void) {
  __asm__ __volatile__("cli");
  __asm__ __volatile__("hlt");
  while (1);
}

uint8_t cpu_vmread_safe(unsigned long field, unsigned long *value)
{
  uint8_t okay;
  __asm__ __volatile__ (
                 "vmread %2, %1\n\t"
                 /* CF==1 or ZF==1 --> rc = 0 */
                 "setnbe %0"
                 : "=qm" (okay), "=rm" (*value)
                 : "r" (field));
  return okay;
}
