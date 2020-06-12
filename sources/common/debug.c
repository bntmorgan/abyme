#include "debug.h"
#include "stdio.h"
#include "stdlib.h"
#include "msr.h"
#include "cpu.h"
#include "string.h"

extern void *ImageBase;
extern void *_data;

void qemu_putc(uint8_t value) {
  __asm__ __volatile__(
    "mov $0x402, %%dx\r\n"
    "out %%al, %%dx\r\n" : : "a"(value)
  );
}

void qemu_send_io(char *s, int size) {
  int i;
  for (i = 0; i < size; i++) {
    qemu_putc(s[i]);
  }
}

void qemu_send_address(char *filename) {
//  int i;

  char addr_str[9];
  char str_0[] = "Dirty ass hack...\n";
  char str_1[] = "Loading driver at 0x";
  char str_2[] = " EntryPoint=0x";

  qemu_send_io(str_0, strlen(str_0));
  qemu_send_io(str_1, strlen(str_1));

  // XXX Support 64 bits
  itoa((int8_t *)&addr_str[0], 16, (uint32_t)(uintptr_t)&ImageBase);
  qemu_send_io(addr_str, strlen(addr_str));

  qemu_send_io(str_2, strlen(str_2));

  // XXX Support 64 bits
  itoa((int8_t *)&addr_str[0], 16, (uint32_t)(uintptr_t)&_data);
  qemu_send_io(addr_str, strlen(addr_str));

  qemu_send_io(" ", 1);

  qemu_send_io(filename, strlen(filename));

  qemu_send_io("\n", 1);

}

void read_core_state(struct core_gpr *gpr, struct core_cr *cr) {
  // Control registers
  cr->cr0 = cpu_read_cr0();
  cr->cr2 = cpu_read_cr2();
  cr->cr3 = cpu_read_cr3();
  cr->cr4 = cpu_read_cr4();
  // General purpose registers
  __asm__ __volatile__("" : "=a" (gpr->rax));
  __asm__ __volatile__("" : "=b" (gpr->rbx));
  __asm__ __volatile__("" : "=c" (gpr->rcx));
  __asm__ __volatile__("" : "=d" (gpr->rdx));
  __asm__ __volatile__("mov %%rsp, %%rax" : "=a" (gpr->rsp));
  __asm__ __volatile__("mov %%rbp, %%rax" : "=a" (gpr->rbp));
  __asm__ __volatile__("mov %%rsi, %%rax" : "=a" (gpr->rsi));
  __asm__ __volatile__("mov %%rdi, %%rax" : "=a" (gpr->rdi));
  __asm__ __volatile__(
    "call h;"    
    "h:;"    
    "pop %%rax"
  : "=a"(gpr->rip));
    gpr->tr = cpu_read_tr();
    gpr->gs = cpu_read_gs();
    gpr->fs = cpu_read_fs();
    gpr->es = cpu_read_es();
    gpr->ds = cpu_read_ds();
    gpr->ss = cpu_read_ss();
    gpr->cs = cpu_read_cs();
}   

void dump_core_state(struct core_gpr *gpr, struct core_cr *cr) {
  printk("cr0 : %08x, cr2 : %08x, cr3 : %08x, cr4 : %08x\n", cr->cr0, cr->cr2, cr->cr3, cr->cr4);
  printk("rax : %08x, rbx : %08x, rcx : %08x, rdx : %08x\n", gpr->rax, gpr->rbx, gpr->rcx, gpr->rdx);
  printk("rsp : %08x, rbp : %08x, rsi : %08x, rdi : %08x\n", gpr->rsp, gpr->rbp, gpr->rsi, gpr->rdi);
  printk("rip : %08x\n", gpr->rip, gpr->rsp, gpr->rbp);
  printk("tr : %08x, gs : %08x, fs : %08x, es : %08x\n", gpr->tr, gpr->gs, gpr->fs, gpr->es);
  printk("ds : %08x, ss : %08x, cs : %08x\n", gpr->ds, gpr->ss, gpr->cs);

  printk("IA32_EFER: %016X\n", msr_read(MSR_ADDRESS_IA32_EFER));
  printk("IA32_VMX_BASIC: %016X\n", msr_read(MSR_ADDRESS_IA32_VMX_BASIC));
  printk("IA32_VMX_PINBASED_CTLS: %016X\n", msr_read(MSR_ADDRESS_IA32_VMX_PINBASED_CTLS));
  printk("IA32_VMX_PROCBASED_CTLS: %016X\n", msr_read(MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS));
  printk("IA32_VMX_EXIT_CTLS: %016X\n", msr_read(MSR_ADDRESS_IA32_VMX_EXIT_CTLS));
  printk("IA32_VMX_ENTRY_CTLS: %016X\n", msr_read(MSR_ADDRESS_IA32_VMX_ENTRY_CTLS));
}

void dump(void *fields, uint32_t fds, uint32_t fdss, uint32_t fpl, uint64_t
    offset, uint32_t step) {
  uint32_t i, j;
  uint32_t cycles = fdss / fds;
  for (i = 0; i < cycles; i++) {
    if (i % fpl == 0) {
      printk("%08x: ", i * step + offset);
    }
    for (j = 0; j < fds; j++) {
      printk("%02x", *((uint8_t*)fields + i * fds + (fds - j - 1)));
    }
    printk(" ");
    if (i % fpl == fpl - 1) {
      printk("\n");
    }
  }
  if (i % fpl != fpl - 1) {
    printk("\n");
  }
}
