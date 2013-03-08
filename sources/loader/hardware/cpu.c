#include "cpu.h"

#include "stdio.h"

#define MSR_ADDRESS_IA32_EFER                 0xc0000080
#define MSR_ADDRESS_IA32_EFER__BIT__LME       0x8
#define MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS2  0x48B

/*
 * eax is the low value and edx the high value.
 */
void msr_read(uint32_t address, uint32_t *eax, uint32_t *edx) {
  __asm__ __volatile__("rdmsr" : "=a" (*eax), "=d" (*edx) : "c" (address));
}

void cpu_write_gdt(uint32_t gdt_ptr, uint32_t code_seg, uint32_t data_seg) {
  __asm__ __volatile__(
      /*
       * Prepare the far jump using a retf through the stack.
       */
      "pushl %1        ;"
      "pushl $1f       ;"
      /*
       * Change the gdt. Segmentation will take place when the descriptor
       * caches have changed. They change on reload of corresponding segment
       * registers. For cs, this change will take place only after a far jump
       * (which force a modification of the code segment selector cs).
       * Fortunatly, because we want to control the next instruction executed
       * by the CPU (otherwise it would not have been the one that comes just
       * after lgdt!).
       * See [Intel_August_2012], volume 3, section 3.4.3.
       */
      "mov %0, %%eax   ;"
      "lgdt (%%eax)    ;"
      /*
       * Change the data segment.
       */
      "mov %2, %%ebx   ;"
      "mov %%ebx, %%ds ;"
      "mov %%ebx, %%es ;"
      "mov %%ebx, %%fs ;"
      "mov %%ebx, %%gs ;"
      "mov %%ebx, %%ss ;"
      /*
       * Far return used to simulate a long jump to the new code segment.
       */
      "retf            ;"
      "1:\n"
    : : "m" (gdt_ptr), "m" (code_seg), "m" (data_seg) : "memory");
}

void cpu_read_gdt(uint32_t *gdt_ptr) {
  __asm__ __volatile__("sgdt %0" : : "m" (*gdt_ptr) : "memory");
}

void cpu_read_ds(uint32_t *reg) {
  __asm__ __volatile__("mov %%ds, %0" : "=a" (*reg));
}

void cpu_read_ss(uint32_t *reg) {
  __asm__ __volatile__("mov %%ss, %0" : "=a" (*reg));
}

void cpu_read_cs(uint32_t *reg) {
  __asm__ __volatile__("mov %%cs, %0" : "=a" (*reg));
}

void cpu_read_cr3(uint32_t *reg) {
  __asm__ __volatile__("mov %%cr3, %0" : "=a" (*reg));
}

void cpu_read_cr0(uint32_t *reg) {
  __asm__ __volatile__("mov %%cr0, %0" : "=a" (*reg));
}

void cpu_write_cr3(uint32_t value) {
  __asm__ __volatile__("movl %%eax, %%cr3" : : "a"(value));
}

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
      "movl %cr0, %eax ;"
      "bts $31, %eax   ;"
      "movl %eax, %cr0 ;"
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

void cpu_enable_pae(void) {
  /*
   * See [Intel_August_2012], volume 3, section 2.5.
   */
  __asm__ __volatile__(
      "movl %cr4, %eax ;"
      "bts $5, %eax    ;"
      "movl %eax, %cr4 ;"
  );
}

void cpu_outportb(uint32_t port, uint8_t value) {
  __asm__ __volatile__("outb %%al,%%dx" : : "d" (port), "a" (value));
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
  cpu_read_cr0(&cr0);
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
  cpu_read_cr0(&cr0);
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

void cpu_print_info(void) {
  uint32_t cs;
  uint32_t ds;
  uint32_t ss;
  uint32_t cr0;
  uint32_t eip;
  cpu_read_cs(&cs);
  cpu_read_ds(&ds);
  cpu_read_ss(&ss);
  cpu_read_cr0(&cr0);
  eip = CPU_READ_EIP();
  INFO("cpu: cs=%04x ds=%04x ss=%04x eip=%08x cr0=%08x\n", cs, ds, ss, eip, cr0);
}

uint8_t cpu_inportb(uint32_t port) {
  uint8_t value;
  __asm__ __volatile__("inb %%dx, %%al" : "=a" (value) : "d" (port));
  return value;
}
