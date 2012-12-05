#include "cpu_int.h"

#include "common/string_int.h"
#include "include/vmem.h"

void cpu_outportb(uint32_t port, uint8_t value) {
  __asm__ __volatile__("outb %%al,%%dx"::"d" (port), "a" (value));
}

void cpu_read_gdt(uint8_t *gdt_ptr) {
  __asm__ __volatile__("sgdt %0" : : "m" (*gdt_ptr) : "memory");
}

void cpu_read_idt(uint8_t *idt_ptr) {
  __asm__ __volatile__("sidt %0" : : "m" (*idt_ptr) : "memory");
}

// TODO: return instead of writing an output parameter?
void cpu_read_cr0(uint64_t *reg) {
  __asm__ __volatile__("mov %%cr0, %0" : "=a" (*reg));
}

void cpu_read_cr3(uint64_t *reg) {
  __asm__ __volatile__("mov %%cr3, %0" : "=a" (*reg));
}

void cpu_read_cr4(uint64_t *reg) {
  __asm__ __volatile__("mov %%cr4, %0" : "=a" (*reg));
}

void cpu_read_cs(uint64_t *reg) {
  __asm__ __volatile__("mov %%cs, %0" : "=a" (*reg));
}

void cpu_read_ss(uint64_t *reg) {
  __asm__ __volatile__("mov %%ss, %0" : "=a" (*reg));
}

void cpu_read_ds(uint64_t *reg) {
  __asm__ __volatile__("mov %%ds, %0" : "=a" (*reg));
}

void cpu_read_es(uint64_t *reg) {
  __asm__ __volatile__("mov %%es, %0" : "=a" (*reg));
}

void cpu_read_fs(uint64_t *reg) {
  __asm__ __volatile__("mov %%fs, %0" : "=a" (*reg));
}

void cpu_read_gs(uint64_t *reg) {
  __asm__ __volatile__("mov %%gs, %0" : "=a" (*reg));
}

void cpu_read_tr(uint64_t *reg) {
  __asm__ __volatile__("str %0" : "=a" (*reg));
}

void cpu_write_cr0(uint64_t value) {
  __asm__ __volatile__("mov %%rax, %%cr0" : : "a"(value));
}

void cpu_write_cr4(uint64_t value) {
  __asm__ __volatile__("mov %%rax, %%cr4" : : "a"(value));
}

uint32_t cpu_get_seg_desc_base(uint64_t gdt_base, uint16_t seg_sel) {
  seg_desc_t seg_desc;

  *((uint64_t*) &seg_desc) = *(((uint64_t*) gdt_base) + (seg_sel >> 3));

  return ((seg_desc.base2 << 24) | (seg_desc.base1 << 16) | seg_desc.base0);
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
      /*
       * vmxon sets the carry flag on error.
       * See Volume 3, Section 30.2 of intel documentation.
       * See Volume 3, Section 30.3 of intel documentation.
       */
      "vmxon (%%rdi) ;"
      /*
       * TODO: test if it is correct to replace following sequence by setae.
       *
       * "jnc _vmxon_ok ;"
       * "mov $0, %%al  ;"
       * "_vmxon_ok:    ;"
       * : : "D" (&region), "a" (status));
       */
      "setae %%cl    ;"
    : : "D" (&region), "c" (status));
  // Note: BOCHS doesn't seem to raise flags when an error occurs...
  if (status != 1) {
    ERROR("vmxon failed\n");
  } else {
    INFO("vmxon successful\n");
  }
}

void cpu_vmclear(uint8_t *region) {
  INFO("vmcs region at %08x\n", (uint32_t) (uint64_t) region);
  uint8_t status = 0;
  __asm__ __volatile__(
      /*
       * vmclear sets the carry flag or the zero flag on error.
       * See Volume 3, Section 30.2 of intel documentation.
       * See Volume 3, Section 30.3 of intel documentation.
       */
      "vmclear (%%rdi)  ;"
      /*
       * TODO: test if it is correct to replace following sequence by seta.
       *
       * "jc _vmclear_fail ;"
       * "jz _vmclear_fail ;"
       * "jmp _vmclear_ok  ;"
       * "_vmclear_fail:   ;"
       * "mov $0, %%al     ;"
       * "_vmclear_ok:     ;"
       * : : "D" (&region), "c" (status));
       */
      "seta %%cl        ;"
    : : "D" (&region), "c" (status));
  // Note: BOCHS doesn't seem to raise flags when an error occurs...
  if (status != 1) {
    ERROR("vmclear failed\n");
  } else {
    INFO("vmclear successful\n");
  }
}

void cpu_vmptrld(uint8_t *region) {
  __asm__ __volatile__("vmptrld (%%rdi)" : : "D" (&region));
}

void cpu_vmlaunch(void) {
  uint8_t status = 1;
  __asm__ __volatile__(
      /*
       * vmlaunch sets the carry flag or the zero flag on error.
       * See Volume 3, Section 30.2 of intel documentation.
       * See Volume 3, Section 30.3 of intel documentation.
       */
      "vmlaunch          ;"
      /*
       * TODO: test if it is correct to replace following sequence by seta.
       *
       * "jc _vmlaunch_fail ;"
       * "jz _vmlaunch_fail ;"
       * "jmp _vmlaunch_ok  ;"
       * "_vmlaunch_fail:   ;"
       * "mov $0, %%al      ;"
       * "_vmlaunch_ok:     ;"
       * : : "a" (status));
       */
      "seta %%cl        ;"
    : : "c" (status));
  // Note: BOCHS doesn't seem to raise flags when an error occurs...
  if (status != 1) {
    ERROR("vmlaunch failed\n");
  } else {
    INFO("vmlaunch successful\n");
  }
}

void cpu_vmwrite(uint32_t field, uint32_t value) {
  __asm__ __volatile__("vmwrite %%rax, %%rdx" : : "a" (value), "d" (field));
}

void cpu_stop(void) {
  __asm__ __volatile__("cli");
  while (1);
}
