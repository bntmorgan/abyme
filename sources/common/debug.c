#include "debug.h"
#include "stdio.h"
#include "hardware/msr.h"

#ifdef _CODE16GCC_
__asm__(".code16gcc\n");
#endif

void dump_core_state() {
  struct core_state st;
  st.cr0 = cpu_read_cr0();
  st.cr2 = cpu_read_cr2();
  st.cr3 = cpu_read_cr3();
  st.cr4 = cpu_read_cr4();
  st.cs = cpu_read_cs();
  st.ds = cpu_read_ds();
  st.es = cpu_read_es();
  st.fs = cpu_read_fs();
  st.gs = cpu_read_gs();

  // XXX Moisi ne correspond pas au bon état il faut le faire plus tôt
  gpr64_t *gpr = (gpr64_t *)&st;
  __asm__ __volatile__(
    "nop;"
  : 
    "=S"(gpr->rsi),
    "=D"(gpr->rdi),
    "=a"(gpr->rax),
    "=b"(gpr->rbx),
    "=c"(gpr->rcx),
    "=d"(gpr->rdx)
  );

  uint32_t msr_ia32_efer_low, msr_ia32_efer_high;
  msr_read(MSR_ADDRESS_IA32_EFER, &msr_ia32_efer_low, &msr_ia32_efer_high);

  printk("cr0 : %08x, cr2 : %08x, cr3 : %08x, cr4 : %08x\n", st.cr0, st.cr2, st.cr3, st.cr4);
  printk("cs : %08x, ds : %08x, fs : %08x, gs : %08x\n", st.cs, st.ds, st.fs, st.gs);
  printk("rsi : %08x, rdi : %08x\n", gpr->rsi, gpr->rdi);
  printk("rax : %08x, rbx : %08x, rcx : %08x, rdx : %08x\n", gpr->rax, gpr->rbx, gpr->rcx, gpr->rdx);
  printk("IA32_EFER low : %08x, IA32_EFER high : %08x\n", msr_ia32_efer_low, msr_ia32_efer_high);
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
