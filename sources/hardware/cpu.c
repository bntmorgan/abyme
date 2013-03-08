#include "cpu.h"

#include "msr.h"
#include "stdio.h"
#include "vmm_info.h"

static uintptr_t reg;

void cpu_outportb(uint32_t port, uint8_t value) {
  __asm__ __volatile__("outb %%al, %%dx" : : "d" (port), "a" (value));
}

uint8_t cpu_inportb(uint32_t port) {
  uint8_t value;
  __asm__ __volatile__("inb %%dx, %%al" : "=a" (value) : "d" (port));
  return value;
}

void cpu_read_gdt(uint8_t *gdt_ptr) {
  __asm__ __volatile__("sgdt %0" : : "m" (*gdt_ptr) : "memory");
}

void cpu_read_idt(uint8_t *idt_ptr) {
  __asm__ __volatile__("sidt %0" : : "m" (*idt_ptr) : "memory");
}

uintptr_t cpu_read_cr0(void) {
  __asm__ __volatile__("mov %%cr0, %0" : "=a" (reg));
  return reg;
}

uintptr_t cpu_read_cr3(void) {
  __asm__ __volatile__("mov %%cr3, %0" : "=a" (reg));
  return reg;
}

uintptr_t cpu_read_cr4(void) {
  __asm__ __volatile__("mov %%cr4, %0" : "=a" (reg));
  return reg;
}

uintptr_t cpu_read_cs(void) {
  __asm__ __volatile__("mov %%cs, %0" : "=a" (reg));
  return reg;
}

uintptr_t cpu_read_ss(void) {
  __asm__ __volatile__("mov %%ss, %0" : "=a" (reg));
  return reg;
}

uintptr_t cpu_read_ds(void) {
  __asm__ __volatile__("mov %%ds, %0" : "=a" (reg));
  return reg;
}

uintptr_t cpu_read_es(void) {
  __asm__ __volatile__("mov %%es, %0" : "=a" (reg));
  return reg;
}

uintptr_t cpu_read_fs(void) {
  __asm__ __volatile__("mov %%fs, %0" : "=a" (reg));
  return reg;
}

uintptr_t cpu_read_gs(void) {
  __asm__ __volatile__("mov %%gs, %0" : "=a" (reg));
  return reg;
}

uintptr_t cpu_read_tr(void) {
  __asm__ __volatile__("str %0" : "=a" (reg));
  return reg;
}

void cpu_write_cr0(uintptr_t value) {
  __asm__ __volatile__("mov %%rax, %%cr0" : : "a" (value));
}

void cpu_write_cr4(uintptr_t value) {
  __asm__ __volatile__("mov %%rax, %%cr4" : : "a" (value));
}

uint32_t cpu_get_seg_desc_base(uintptr_t gdt_base, uint16_t seg_sel) {
  /*
   * See [Intel_August_2012], volume 3, section 3.4.2.
   * See [Intel_August_2012], volume 3, section 5.2.1.
   */
  seg_desc_t seg_desc;
  *((uintptr_t*) &seg_desc) = *(((uintptr_t*) gdt_base) + (seg_sel >> 3));
  return ((seg_desc.base2 << 24) | (seg_desc.base1 << 16) | seg_desc.base0);
}

void cpu_enable_ne(void) {
  /*
   * See [Intel_August_2012], volume 3, section 2.5.
   */
  __asm__ __volatile__(
      "mov %%cr0, %%rax      ;"
      "or $0x00000020, %%rax ;"
      "mov %%rax, %%cr0      ;"
    : : : "rax");
}

void cpu_enable_vmxe(void) {
  /*
   * See [Intel_August_2012], volume 3, section 2.5.
   */
  __asm__ __volatile__(
      "mov %%cr4, %%rax      ;"
      "or $0x00002000, %%rax ;"
      "mov %%rax, %%cr4      ;"
    : : : "rax");
}

void cpu_vmxon(uint8_t *region) {
  INFO("vmxon region at %08X\n", (uintptr_t) region);
  uint8_t ok = 0;
  /*
   * vmxon sets the carry flag on error.
   * See [Intel_August_2012], volume 3, section 30.2.
   * See [Intel_August_2012], volume 3, section 30.3.
   */
  __asm__ __volatile__(
      "vmxon (%%rdi) ;"
      "setae %%cl    ;"
    : "=c" (ok) : "D" (&region));
  if (ok) {
    INFO("vmxon successful\n");
  } else {
    ERROR("vmxon failed\n");
  }
}

void cpu_vmclear(uint8_t *region) {
  INFO("vmcs region at %08X\n", (uintptr_t) region);
  uint8_t ok = 0;
  /*
   * vmclear sets the carry flag or the zero flag on error.
   * See [Intel_August_2012], volume 3, section 30.2.
   * See [Intel_August_2012], volume 3, section 30.3.
   */
  __asm__ __volatile__(
      "vmclear (%%rdi) ;"
      "seta %%cl       ;"
    : "=c" (ok) : "D" (&region));
  if (ok) {
    INFO("vmclear successful\n");
  } else {
    ERROR("vmclear failed\n");
  }
}

void cpu_vmptrld(uint8_t *region) {
  INFO("vmcs region at %08X\n", (uintptr_t) region);
  uint8_t ok = 0;
  /*
   * vmlptrld sets the carry flag or the zero flag on error.
   * See [Intel_August_2012], volume 3, section 30.2.
   * See [Intel_August_2012], volume 3, section 30.3.
   */
  __asm__ __volatile__(
      "vmptrld (%%rdi) ;"
      "seta %%cl       ;"
    : "=c" (ok) : "D" (&region));
  if (ok) {
    INFO("vmptrld successful\n");
  } else {
    ERROR("vmptrld failed\n");
  }
}

void cpu_vmlaunch(void) {
  uint8_t ok = 0;
  /*
   * vmlaunch sets the carry flag or the zero flag on error.
   * See [Intel_August_2012], volume 3, section 30.2.
   * See [Intel_August_2012], volume 3, section 30.3.
   */
  __asm__ __volatile__(
      "vmlaunch  ;"
      "seta %%cl ;"
    : "=c" (ok));
  if (ok) {
    INFO("vmlaunch successful\n");
  } else {
    ERROR("vmlaunch failed\n");
  }
}

void cpu_vmresume(void) {
  uint8_t ok = 0;
  /*
   * vmresume sets the carry flag or the zero flag on error.
   * See [Intel_August_2012], volume 3, section 30.2.
   * See [Intel_August_2012], volume 3, section 30.3.
   */
  __asm__ __volatile__(
      "vmresume  ;"
      "seta %%cl ;"
    : "=c" (ok));
  if (ok) {
    INFO("vmresume successful\n");
  } else {
    ERROR("vmresume failed\n");
  }
}

void cpu_vmwrite(uint32_t field, uint32_t value) {
  __asm__ __volatile__("vmwrite %%rax, %%rdx" : : "a" (value), "d" (field));
}

uint32_t cpu_vmread(uint32_t field) {
  uint32_t value;
  __asm__ __volatile__("vmread %%rdx, %%rax" : "=a" (value): "d" (field));
  return value;
}

void cpu_stop(void) {
  __asm__ __volatile__("cli");
  while (1);
}

uint32_t cpu_adjust32(uint32_t value, uint32_t msr) {
  /*
   * TODO: Use new CPU capability MSRs (IA32_VMX_TRUE_PINBASED_CTLS,
   * IA32_VMX_TRUE_PROCBASED_CTLS, IA32_VMX_TRUE_EXIT_CTLS and
   * IA32_VMX_TRUE_ENTRY_CTLS for capability detection of the default1
   * controls.
   * It doesn't matter for now since we simply want to use default values.
   */
  uint32_t eax, ebx;
  msr_read(msr, &eax, &ebx);
  value |= eax;
  value &= ebx;
  return value;
}

uintptr_t cpu_adjust64(uintptr_t value, uint32_t fixed0_msr, uint32_t fixed1_msr) {
  return (value & msr_read64(fixed1_msr)) | msr_read64(fixed0_msr);
}
