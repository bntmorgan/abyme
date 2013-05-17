#include "vmm.h"
#include "stdio.h"
#include "cpu.h"
#include "msr.h"
#include "string.h"
#include <efi.h>
#include <efilib.h>

#include "systab.h"
#include "vmcs.h"
#include "pci.h"
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
  uint32_t exit_qualification = cpu_vmread(EXIT_QUALIFICATION);

  if (exit_reason != EXIT_REASON_IO_INSTRUCTION) {
    debug_server_run(exit_reason, &guest_regs);
  }

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

  static uint8_t catch = 2;
  static uint32_t eax = 0;

  switch (exit_reason) {
    case EXIT_REASON_IO_INSTRUCTION: {
      uint32_t ins = *((uint32_t *) guest_regs.rip);
      uint8_t *ins_byte = (uint8_t *) &ins;
      memset(&ins_byte[exit_instruction_length], 0x90, 4 - exit_instruction_length);
      exit_qualification++;
      uint8_t size;

      if (catch == 2) {
        eax = pci_make_addr(PCI_MAKE_ID(
              eth->pci_addr.bus,
              eth->pci_addr.device,
              eth->pci_addr.function));
      }
      switch (exit_qualification & 0x3) {
        case 0:
          size = 1;
          break;
        case 1:
          size = 2;
          break;
        case 3:
          size = 4;
          break;
      }
      // 0 out, 1 IN
      uint8_t direction = (exit_qualification >> 2) & 0x1;
      uint16_t port = (exit_qualification >> 16) & 0xffff;
      __asm__ __volatile__(
          "mov %%ecx, 1f(%%rip)  ;"
          "1: nop; nop; nop; nop ;"
        : "=a" (guest_regs.rax), "=d" (guest_regs.rdx)
        : "a" (guest_regs.rax), "c" (ins), "d" (guest_regs.rdx));
      if (direction == 0 && port == PCI_CONFIG_ADDR) {
        if ((guest_regs.rax & 0xffffff00) == eax) {
          catch = 1;
        } else {
          catch = 0;
        }
      } else if (direction == 1 && port == PCI_CONFIG_DATA && catch) {
        memset(&guest_regs.rax, 0xff, size);
      }
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
      // printk("Exit reason (base 16) %X\n", exit_reason);
      // printk("rax %X, rbx %X, rcx %X, rdx %X\n", guest_regs.rax, guest_regs.rbx, guest_regs.rcx, guest_regs.rdx);
      // printk("cr0 %X\n", cpu_vmread(GUEST_CR0));
      //dump((void *)0, exit_instruction_length, 1, guest_regs.rip, 1);
      // uint64_t guest_linear_address = cpu_vmread(GUEST_LINEAR_ADDRESS);
      // INFO("GUEST LINEAR %X\n", guest_linear_address);
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
