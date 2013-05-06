#include "vmm.h"
#include "stdio.h"
#include "cpu.h"
#include "msr.h"
#include <efi.h>
#include <efilib.h>

#include "systab.h"
#include "vmcs.h"
#include "debug.h"
#include "debug_server/debug_server.h"

void vmm_print_guest_regs(struct registers *guest_regs) {
  INFO("rax=%X\n", guest_regs->rax);
  INFO("rbx=%X\n", guest_regs->rbx);
  INFO("rcx=%X\n", guest_regs->rcx);
  INFO("rdx=%X\n", guest_regs->rdx);
}

void vmm_WAIT(void) {
  uint64_t key = 0;
  while (key != 0xd0000) {
    uefi_call_wrapper(systab->ConIn->ReadKeyStroke, 2, systab->ConIn, &key);
  }
}

void vmm_handle_vm_exit(struct registers guest_regs) {
  guest_regs.rsp = cpu_vmread(GUEST_RSP);
  guest_regs.rip = cpu_vmread(GUEST_RIP);
  static uint64_t msr_exit = 0;
  uint32_t exit_reason = cpu_vmread(VM_EXIT_REASON);

  debug_server_run(exit_reason);

  uint32_t exit_instruction_length = cpu_vmread(VM_EXIT_INSTRUCTION_LEN);
  //printk("VM_EXIT_REASON: %x\n", exit_reason);
  //printk("EXIT_QUALIFICATION: %x\n", cpu_vmread(EXIT_QUALIFICATION));
  //printk("VM_EXIT_INSTRUCTION_LEN: %x\n", cpu_vmread(VM_EXIT_INSTRUCTION_LEN));
  //printk("GUEST_RIP: %X\n", cpu_vmread(GUEST_RIP));
  //printk("GUEST_CR0: %X\n", cpu_vmread(GUEST_CR0));
  //printk("GUEST_CR4: %X\n", cpu_vmread(GUEST_CR4));
  //printk("GUEST_IA32_EFER: %X\n", cpu_vmread(GUEST_IA32_EFER));
  //printk("GUEST_RFLAGS: %X\n", cpu_vmread(GUEST_RFLAGS));
  //printk("VM_INSTRUCTION_ERROR: %x\n", cpu_vmread(VM_INSTRUCTION_ERROR));
  //printk("VM_ENTRY_CONTROLS: %x\n", cpu_vmread(VM_ENTRY_CONTROLS));

  //Print(L"PCI IO %d\n", status);

  switch (exit_reason) {
    case EXIT_REASON_IO_INSTRUCTION: {
      // INFO("EXIT_REASON_IO_INSTRUCTION\n");
      uint32_t instruction = *((uint32_t *) guest_regs.rip);
      // INFO("INSTRUCTION %x\n", instruction);
      // INFO("BEFORE rax:%X rdx:%x\n", guest_regs.rax, guest_regs.rdx);
      // Decode instruction
      if        ((instruction & 0x00ff) == 0x00ec &&
          exit_instruction_length == 1) {  // in %dx, %al
        __asm__ __volatile__(
            "in %%dx, %%al"
        : "=a"(guest_regs.rax), "=d"(guest_regs.rdx) : "a"(guest_regs.rax), "d"(guest_regs.rdx));
      } else if ((instruction & 0xffff) == 0xed66 &&
          exit_instruction_length == 2) {  // in %dx, %ax
        __asm__ __volatile__(
            "in %%dx, %%ax"
        : "=a"(guest_regs.rax), "=d"(guest_regs.rdx) : "a"(guest_regs.rax), "d"(guest_regs.rdx));
      } else if ((instruction & 0x00ff) == 0x00ed &&
          exit_instruction_length == 1) {  // in %dx, %eax
        __asm__ __volatile__(
            "in %%dx, %%eax"
        : "=a"(guest_regs.rax), "=d"(guest_regs.rdx) : "a"(guest_regs.rax), "d"(guest_regs.rdx));
      } else if ((instruction & 0x00ff) == 0x00e4 &&
          exit_instruction_length == 2) {  // in $x, %al
        uint8_t dx = (instruction >> 8) & 0xff;
        __asm__ __volatile__(
            "in %%dx, %%al"
        : "=a"(guest_regs.rax), "=d"(dx) : "a"(guest_regs.rax), "d"(dx));
      } else if ((instruction & 0xffff) == 0xe566 &&
          exit_instruction_length == 3) {  // in $x, %ax
        uint8_t dx = (instruction >> 16) & 0xff;
        __asm__ __volatile__(
            "in %%dx, %%ax"
        : "=a"(guest_regs.rax), "=d"(dx) : "a"(guest_regs.rax), "d"(dx));
      } else if ((instruction & 0x00ff) == 0x00e5 &&
          exit_instruction_length == 2) {  // in $x, %aex
        uint8_t dx = (instruction >> 8) & 0xff;
        __asm__ __volatile__(
            "in %%dx, %%eax"
        : "=a"(guest_regs.rax), "=d"(dx) : "a"(guest_regs.rax), "d"(dx));
      } else if ((instruction & 0x00ff) == 0x00ee &&
          exit_instruction_length == 1) {  // out %al, %dx
        __asm__ __volatile__(
            "out %%al, %%dx"
        : "=a"(guest_regs.rax), "=d"(guest_regs.rdx) : "a"(guest_regs.rax), "d"(guest_regs.rdx));
      } else if ((instruction & 0xffff) == 0xef66 &&
          exit_instruction_length == 2) {  // out %ax, %dx
        __asm__ __volatile__(
            "out %%ax, %%dx"
        : "=a"(guest_regs.rax), "=d"(guest_regs.rdx) : "a"(guest_regs.rax), "d"(guest_regs.rdx));
      } else if ((instruction & 0x00ff) == 0x00ef &&
          exit_instruction_length == 1) {  // out %eax, %dx
        __asm__ __volatile__(
            "out %%eax, %%dx"
        : "=a"(guest_regs.rax), "=d"(guest_regs.rdx) : "a"(guest_regs.rax), "d"(guest_regs.rdx));
      } else if ((instruction & 0x00ff) == 0x00e6 &&
          exit_instruction_length == 2) {  // out %al, $x
        uint8_t dx = (instruction >> 8) & 0xff;
        __asm__ __volatile__(
            "out %%al, %%dx"
        : "=a"(guest_regs.rax), "=d"(dx) : "a"(guest_regs.rax), "d"(dx));
      } else if ((instruction & 0xffff) == 0xe766 &&
          exit_instruction_length == 3) {  // out %ax, $x
        uint8_t dx = (instruction >> 16) & 0xff;
        __asm__ __volatile__(
            "out %%ax, %%dx"
        : "=a"(guest_regs.rax), "=d"(dx) : "a"(guest_regs.rax), "d"(dx));
      } else if ((instruction & 0x00ff) == 0x00e7 &&
          exit_instruction_length == 2) {  // out %eax, $x
        uint8_t dx = (instruction >> 8) & 0xff;
        __asm__ __volatile__(
            "out %%eax, %%dx"
        : "=a"(guest_regs.rax), "=d"(dx) : "a"(guest_regs.rax), "d"(dx));
      } else {
        INFO("UNKNOWN I/O :(\n");
        while(1);
      }
      // INFO("AFTER rax:%X rdx:%x\n", guest_regs.rax, guest_regs.rdx);
      break;
    }
    case EXIT_REASON_CPUID: {
      //vmm_print_guest_regs(&guest_regs);
      __asm__ __volatile__("cpuid"
          : "=a" (guest_regs.rax), "=b" (guest_regs.rbx), "=c" (guest_regs.rcx), "=d" (guest_regs.rdx)
          :  "a" (guest_regs.rax),  "b" (guest_regs.rbx),  "c" (guest_regs.rcx),  "d" (guest_regs.rdx));
      //vmm_print_guest_regs(&guest_regs);
      break;
    }
    case EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED: {
      INFO("EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED: @%X\n", guest_regs.rip);
      cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE,  99000000); 
      break;
    } 
    case EXIT_REASON_RDMSR: {
      msr_exit++;
      INFO("RDMSR %d\n", msr_exit);
      __asm__ __volatile__("rdmsr"
          : "=a" (guest_regs.rax), "=b" (guest_regs.rbx), "=c" (guest_regs.rcx), "=d" (guest_regs.rdx)
          :  "a" (guest_regs.rax),  "b" (guest_regs.rbx),  "c" (guest_regs.rcx),  "d" (guest_regs.rdx));
      break;
    }
    case EXIT_REASON_WRMSR: {
      __asm__ __volatile__("wrmsr"
          : "=a" (guest_regs.rax), "=b" (guest_regs.rbx), "=c" (guest_regs.rcx), "=d" (guest_regs.rdx)
          :  "a" (guest_regs.rax),  "b" (guest_regs.rbx),  "c" (guest_regs.rcx),  "d" (guest_regs.rdx));
      break;
    }
    default: {
      printk("Exit reason (base 16) %X\n", exit_reason);
      printk("rax %X, rbx %X, rcx %X, rdx %X\n", guest_regs.rax, guest_regs.rbx, guest_regs.rcx, guest_regs.rdx);
      printk("cr0 %X\n", cpu_vmread(GUEST_CR0));
      //dump((void *)0, exit_instruction_length, 1, guest_regs.rip, 1);

      dump((void *)guest_regs.rip, 1, exit_instruction_length, guest_regs.rip, 1);
      while(1);
    }
  }
  if (exit_reason != EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED) {
    cpu_vmwrite(GUEST_RIP, guest_regs.rip + exit_instruction_length);
  }
  if (msr_exit == 0x42) {
    INFO("TODO ACTIVATION DU PREEMTION TIMER\n");
    cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE,  100000); 
    uint32_t pinbased_ctls = ACT_VMX_PREEMPT_TIMER;
    cpu_vmwrite(PIN_BASED_VM_EXEC_CONTROL, cpu_adjust32(pinbased_ctls, MSR_ADDRESS_IA32_VMX_PINBASED_CTLS));
  }
}
