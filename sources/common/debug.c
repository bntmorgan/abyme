#include "debug.h"
#include "stdio.h"
#include "hardware/msr.h"

#ifdef _CODE16GCC_
__asm__(".code16gcc\n");
#endif

void dump_core_state(struct core_gpr *gpr) {
  uint32_t msr_low, msr_high;
  struct core_state st;
  st.cr0 = cpu_read_cr0();
  st.cr2 = cpu_read_cr2();
  st.cr3 = cpu_read_cr3();
  st.cr4 = cpu_read_cr4();

  printk("cr0 : %08x, cr2 : %08x, cr3 : %08x, cr4 : %08x\n", st.cr0, st.cr2, st.cr3, st.cr4);
  printk("rax : %08x, rbx : %08x, rcx : %08x, rdx : %08x\n", gpr->rax, gpr->rbx, gpr->rcx, gpr->rdx);
  printk("rsp : %08x, rbp : %08x, rsi : %08x, rdi : %08x\n", gpr->rsp, gpr->rbp, gpr->rsi, gpr->rdi);
  printk("rip : %08x\n", gpr->rip, gpr->rsp, gpr->rbp);
  printk("ts : %08x, gs : %08x, fs : %08x, es : %08x\n", gpr->tr, gpr->gs, gpr->fs, gpr->es);
  printk("ds : %08x, ss : %08x, cs : %08x\n", gpr->ds, gpr->ss, gpr->cs);

  dump((void *) 0x6000, 4, 60, 0x6000, 4);

  msr_read(MSR_ADDRESS_IA32_EFER, &msr_low, &msr_high);
  printk("IA32_EFER low : %08x, high : %08x\n", msr_low, msr_high);
  msr_read(MSR_ADDRESS_IA32_VMX_BASIC, &msr_low, &msr_high);
  printk("IA32_VMX_BASIC low : %08x, high : %08x\n", msr_low, msr_high);
  msr_read(MSR_ADDRESS_IA32_VMX_PINBASED_CTLS, &msr_low, &msr_high);
  printk("IA32_VMX_PINBASED_CTLS low : %08x, high : %08x\n", msr_low, msr_high);
  msr_read(MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS, &msr_low, &msr_high);
  printk("IA32_VMX_PROCBASED_CTLS low : %08x, high : %08x\n", msr_low, msr_high);
  msr_read(MSR_ADDRESS_IA32_VMX_EXIT_CTLS, &msr_low, &msr_high);
  printk("IA32_VMX_EXIT_CTLS low : %08x, high : %08x\n", msr_low, msr_high);
  msr_read(MSR_ADDRESS_IA32_VMX_ENTRY_CTLS, &msr_low, &msr_high);
  printk("IA32_VMX_ENTRY_CTLS low : %08x, high : %08x\n", msr_low, msr_high);
}

void dump(void *fields, uint32_t fds, uint32_t fdss, uint32_t offset, uint32_t step) { \
  uint32_t i, j;
  uint32_t cycles = fdss / fds;
  for (i = 0; i < cycles; i++) {
    if (i % 4 == 0) {
      printk("%08x: ", i * step + offset);
    }
    for (j = 0; j < fds; j++) {
      printk("%02x", *((uint8_t*)fields + i * fds + (fds - j - 1)));
    }
    printk(" ");
    if (i % 4 == 3) {
      printk("\n");
    }
  }
  if (i % 4 != 3) {
    printk("\n");
  }
}
