#include "cpu.h"

#include "msr.h"
#include "stdio.h"
#include "vmm_info.h"
#include "vmm.h"


static uint64_t reg;

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

uint64_t cpu_read_cr0(void) {
  __asm__ __volatile__("mov %%cr0, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_cr2(void) {
  __asm__ __volatile__("mov %%cr2, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_cr3(void) {
  __asm__ __volatile__("mov %%cr3, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_cr4(void) {
  __asm__ __volatile__("mov %%cr4, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_cs(void) {
  __asm__ __volatile__("mov %%cs, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_ldtr(void) {
  __asm__ __volatile__(
    "sldt %%eax;" 
  : "=a" (reg));
  return reg;
}

uint64_t cpu_read_dr7(void) {
  __asm__ __volatile__("mov %%dr7, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_ss(void) {
  __asm__ __volatile__("mov %%ss, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_ds(void) {
  __asm__ __volatile__("mov %%ds, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_es(void) {
  __asm__ __volatile__("mov %%es, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_fs(void) {
  __asm__ __volatile__("mov %%fs, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_gs(void) {
  __asm__ __volatile__("mov %%gs, %0" : "=a" (reg));
  return reg;
}

uint64_t cpu_read_tr(void) {
  __asm__ __volatile__("str %0" : "=a" (reg));
  return reg;
}

void cpu_write_cr0(uint64_t value) {
  __asm__ __volatile__("mov %0, %%cr0" : : "a" (value));
}

void cpu_write_cr4(uint64_t value) {
  __asm__ __volatile__("mov %0, %%cr4" : : "a" (value));
}

void cpu_write_cr3(uint64_t value) {
  __asm__ __volatile__("mov %0, %%cr3" : : "a"(value));
}

uint64_t cpu_read_rsp(void) {
  __asm__ __volatile__("mov %%rsp, %0" : "=a" (reg));
  return reg;
}

uint32_t cpu_get_seg_desc_base(uint64_t gdt_base, uint16_t seg_sel) {
  /*
   * See [Intel_August_2012], volume 3, section 3.4.2.
   * See [Intel_August_2012], volume 3, section 5.2.1.
   */
  seg_desc_t seg_desc;
  *((uint64_t*) &seg_desc) = *(((uint64_t*) gdt_base) + (seg_sel >> 3));
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
    : : : "eax");
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
  INFO("vmxon region at %08X\n", (uint64_t) region);
  uint8_t ok = 0;
  /*
   * vmxon sets the carry flag on error.
   * See [Intel_August_2012], volume 3, section 30.2.
   * See [Intel_August_2012], volume 3, section 30.3.
   */
  __asm__ __volatile__(
      "vmxon (%1) ;"
      "setae %%cl    ;"
    : "=c" (ok) : "D" (&region));
  if (ok) {
    INFO("vmxon successful\n");
  } else {
    ERROR("vmxon failed\n");
  }
}

void cpu_vmclear(uint8_t *region) {
  INFO("vmcs region at %08X\n", (uint64_t) region);
  uint8_t ok = 0;
  /*
   * vmclear sets the carry flag or the zero flag on error.
   * See [Intel_August_2012], volume 3, section 30.2.
   * See [Intel_August_2012], volume 3, section 30.3.
   */
  __asm__ __volatile__(
      "vmclear (%1) ;"
      "seta %%cl       ;"
    : "=c" (ok) : "D" (&region));
  if (ok) {
    INFO("vmclear successful\n");
  } else {
    ERROR("vmclear failed\n");
  }
}

void cpu_vmptrld(uint8_t *region) {
  INFO("vmcs region at %08X\n", (uint64_t) region);
  uint8_t ok = 0;
  /*
   * vmlptrld sets the carry flag or the zero flag on error.
   * See [Intel_August_2012], volume 3, section 30.2.
   * See [Intel_August_2012], volume 3, section 30.3.
   */
  __asm__ __volatile__(
      "vmptrld (%1) ;"
      "seta %%cl       ;"
    : "=c" (ok) : "D" (&region));
  if (ok) {
    INFO("vmptrld successful\n");
  } else {
    ERROR("vmptrld failed\n");
  }
}

// Export Guest RIP
__asm__(".global vmm_setup_vm_launched");
extern uint8_t changed;
void cpu_vmlaunch(void) {
  uint8_t ok = 0;
  /**
   * We set current rsp and rip into the guest fields
   */
  uint64_t reg;
  __asm__ __volatile__(
//TODO: autres registres !!!!!
//TODO: autres registres !!!!!
//TODO: autres registres !!!!!
//TODO: autres registres !!!!!
//TODO: autres registres !!!!!
//TODO: autres registres !!!!!
      "push %rax ;\n"
      "push %rbx ;\n"
      "push %rcx ;\n"
      "push %rdx ;\n"
      "push %rsi ;\n"
      "push %rdi ;\n"
  );
  __asm__ __volatile__("mov %%rsp, %0" : "=a" (reg));
  cpu_vmwrite(GUEST_RSP, reg);
//  cpu_vmwrite(GUEST_RIP, (uint32_t)((uint64_t)&vmm_setup_vm_launched));// label après vmlaunch
  /*
   * vmlaunch sets the carry flag or the zero flag on error.
   * See [Intel_August_2012], volume 3, section 30.2.
   * See [Intel_August_2012], volume 3, section 30.3.
   *
   * 66 0F 38 80 
   */
/*  uint8_t i;
__asm__(".global changed");
  (&changed)[0] = 0x66;
  (&changed)[1] = 0x0f;
  (&changed)[2] = 0x38;
  (&changed)[3] = 0x80;
  for (i = 0; i < 16; i++) {
    printk("%d %02x\n", i, (&changed)[i]);
  }*/
  __asm__ __volatile__(
/*"changed:\n"
      "nop  ;"
      "nop  ;"
      "nop  ;"
      "nop  ;"
      "invd  ;"*/
      "vmlaunch  ;"
"vmm_setup_vm_launched:\n"
      "seta %%cl ;"
    : "=c" (ok));
  if (ok) {
    INFO("vmlaunch successful\n");
  __asm__ __volatile__(
      "pop %rdi ;\n"
      "pop %rsi ;\n"
      "pop %rdx ;\n"
      "pop %rcx ;\n"
      "pop %rbx ;\n"
      "pop %rax ;\n"
    );
  } else {
    uint32_t error = cpu_vmread(VM_INSTRUCTION_ERROR);
    ERROR("vmlaunch failed, reason : %X\n", error);
  }
}

uint32_t cpu_read_flags(void) {
  uint32_t flags;
  __asm__ __volatile__(
    "pushfq;"
    "popq %%rax;"
  : "=a"(flags));
  return flags;
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

void cpu_vmwrite(uint64_t field, uint64_t value) {
  __asm__ __volatile__("vmwrite %%rax, %%rdx" : : "a" (value), "d" (field));
}

uint32_t cpu_vmread(uint32_t field) {
  uint32_t value;
  __asm__ __volatile__("vmread %%rdx, %%rax" : "=a" (value): "d" (field));
  return value;
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

uint64_t cpu_adjust64(uint64_t value, uint32_t fixed0_msr, uint32_t fixed1_msr) {
  return (value & msr_read64(fixed1_msr)) | msr_read64(fixed0_msr);
}

#define MSR_ADDRESS_IA32_EFER                 0xc0000080
#define MSR_ADDRESS_IA32_EFER__BIT__LME       0x8
#define MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS2  0x48B

void cpu_enable_paging(void) {
  /*
   * The paging is setup with identity mapping without offset.
   * So, the linear and physical addresses are equal.
   * It is important for switching to long mode.
   * See [Intel_August_2012], volume 3, section 2.5.
   * See [Intel_August_2012], volume 3, section 22.30.3.
   * See [Intel_August_2012], volume 3, section 9.8.5.
   */
  /*
   * TODO: we should jump after enabling paging.
   * See [Intel_August_2012], volume 3, section 22.30.3.
   */
  __asm__ __volatile__(
      "mov %cr0, %rax ;"
      "bts $31, %rax   ;"
      "mov %rax, %cr0 ;"
  );
}

void cpu_enable_long_mode(void) {
  /*
   * See [Intel_August_2012], volume 3, section 9.8.5.
   * See [Intel_August_2012], volume 3, section 35.2.
   */
  __asm__ __volatile__(
      "mov $0xc0000080, %ecx ;"
      "rdmsr                 ;"
      "bts $0x8, %eax        ;"
      "wrmsr                 ;"
  );
}

void cpu_stop(void) {
  __asm__ __volatile__("cli");
  __asm__ __volatile__("hlt");
  while (1);
}

uint8_t cpu_is_paging_enabled(void) {
  /*
   * See [Intel_August_2012], volume 3, section 2.5.
   */
  uint32_t cr0;
  cr0 = cpu_read_cr0();
  if ((cr0 & 0x80000000) == 0x80000000) {
    return 1;
  }
  return 0;
}

uint8_t cpu_is_protected_mode_enabled(void) {
  /*
   * See [Intel_August_2012], volume 3, section 2.5.
   */
  uint32_t cr0;
  cr0 = cpu_read_cr0();
  if ((cr0 & 0x1) == 0x1) {
    return 1;
  }
  return 0;
}

uint8_t cpu_is_ept_supported(void) {
  /*
   * See [Intel_August_2012], volume 3, section A.3.3.
   * See [Intel_August_2012], volume 3, section 24.6.2, table 24-7.
   */
  uint32_t eax, edx;
  msr_read(MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS2, &eax, &edx);
  return (edx & (1 << 1)) == (1 << 1);
}

uint8_t cpu_is_unrestricted_guest_supported(void) {
  /*
   * See [Intel_August_2012], volume 3, section A.3.3.
   * See [Intel_August_2012], volume 3, section 24.6.2, table 24-7.
   */
  uint32_t eax, edx;
  msr_read(MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS2, &eax, &edx);
  return (edx & (1 << 7)) == (1 << 7);
}
