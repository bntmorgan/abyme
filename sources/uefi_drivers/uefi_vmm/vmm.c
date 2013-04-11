#include "vmm.h"
#include "stdio.h"
#include "hardware/cpu.h"
#include "hardware/msr.h"
#include <efi.h>
#include <efilib.h>
#include "systab.h"

void vmm_set_guest_rip(uint64_t guest_rip, uint32_t exit_instruction_length) {
  //uint64_t guest_cr0 = cpu_vmread(GUEST_CR0);
  //if ((guest_cr0 & 0x1) == 0x0) {
  //  cpu_vmwrite(GUEST_RIP, (guest_rip + exit_instruction_length) % 0x10000);
  //} else {
    cpu_vmwrite(GUEST_RIP, guest_rip + exit_instruction_length);
  //}
}

uint64_t vmm_get_guest_rip(void) {
  uint64_t guest_rip = cpu_vmread(GUEST_RIP);
  uint64_t guest_cr0 = cpu_vmread(GUEST_CR0);
  if ((guest_cr0 & 0x1) == 0x0) {
    //uint64_t guest_cs = cpu_vmread(GUEST_CS_SELECTOR);
    /* TODO: if a20, don't wrap! */
    return guest_rip;
    //return ((guest_cs << 4) + guest_rip) & 0xfffff;
  } else {
    return guest_rip;
  }
}

void vmm_handle_vm_exit(gpr64_t guest_gpr) {
  guest_gpr.rsp = cpu_vmread(GUEST_RSP);
  guest_gpr.rip = vmm_get_guest_rip();
  static uint64_t msr_exit = 0;

  ////INFO("Vm exit\n");
  //uint64_t guest_rip = vmm_get_guest_rip();
  uint32_t exit_reason = cpu_vmread(VM_EXIT_REASON);
  //uint32_t exit_qualification = cpu_vmread(EXIT_QUALIFICATION);
  //  printk("VMEXIT REASON: %x\n", VM_EXIT_REASON);
  ////printk("VMEXIT REASON: %x\n", exit_reason);
 //// printk("VMEXIT QUALIFICATION: %x\n", cpu_vmread(EXIT_QUALIFICATION));
 //// printk("GUEST RIP: %X\n", cpu_vmread(GUEST_RIP));
  //  printk("GUEST CR0: %x\n", cpu_vmread(GUEST_CR0));
  // printk("GUEST CR4: %x\n", cpu_vmread(GUEST_CR4));
  //  printk("GUEST_IA32_EFER: %x\n", cpu_vmread(GUEST_IA32_EFER));
 //// printk("GUEST_RFLAGS: %x\n", cpu_vmread(GUEST_RFLAGS));
  ////printk("VM_INSTRUCTION_ERROR: %x\n", cpu_vmread(VM_INSTRUCTION_ERROR));
  //  printk("VM_ENTRY_CONTROLS: %x\n", cpu_vmread(VM_ENTRY_CONTROLS));
  uint32_t exit_instruction_length = cpu_vmread(VM_EXIT_INSTRUCTION_LEN);
  /*printk("VMEXIT REASON: %x\n", exit_reason);
  printk("VMEXIT QUALIFICATION: %x\n", cpu_vmread(EXIT_QUALIFICATION));
  printk("GUEST RIP: %X\n", cpu_vmread(GUEST_RIP));
  printk("GUEST CR0: %x\n", cpu_vmread(GUEST_CR0));
  printk("GUEST CR4: %x\n", cpu_vmread(GUEST_CR4));
  printk("GUEST CS: %x\n", cpu_vmread(GUEST_CS_SELECTOR));
  printk("GUEST_IA32_EFER: %x\n", cpu_vmread(GUEST_IA32_EFER));
  printk("GUEST_RFLAGS: %x\n", cpu_vmread(GUEST_RFLAGS));
  printk("VM_INSTRUCTION_ERROR: %x\n", cpu_vmread(VM_INSTRUCTION_ERROR));
  printk("VM_ENTRY_CONTROLS: %x\n", cpu_vmread(VM_ENTRY_CONTROLS));*/
  
  switch (exit_reason) {
    case EXIT_REASON_CPUID: {
      /*INFO("handling CPUID (rax = %x)\n", guest_gpr.rax);
      INFO("handling CPUID (rbx = %x)\n", guest_gpr.rbx);
      INFO("handling CPUID (rcx = %x)\n", guest_gpr.rcx);
      INFO("handling CPUID (rdx = %x)\n", guest_gpr.rdx);*/
      //uint64_t command = guest_gpr.rax;
      __asm__ __volatile__("cpuid" : "=a" (guest_gpr.rax),
          "=b" (guest_gpr.rbx), "=c" (guest_gpr.rcx),
          "=d" (guest_gpr.rdx) : "a" (guest_gpr.rax), "b" (guest_gpr.rbx), "c" (guest_gpr.rcx), "d" (guest_gpr.rdx));
      /*INFO("handling CPUID (rax = %x)\n", guest_gpr.rax);
      INFO("handling CPUID (rbx = %x)\n", guest_gpr.rbx);
      INFO("handling CPUID (rcx = %x)\n", guest_gpr.rcx);
      INFO("handling CPUID (rdx = %x)\n", guest_gpr.rdx);*/
      /*if (command == 0) {
  INFO("Vm exit\n");
  uint32_t exit_reason = cpu_vmread(VM_EXIT_REASON);
  uint32_t exit_instruction_length = cpu_vmread(VM_EXIT_INSTRUCTION_LEN);
  printk("VMEXIT REASON: %x\n", exit_reason);
  printk("VMEXIT QUALIFICATION: %x\n", cpu_vmread(EXIT_QUALIFICATION));
  printk("GUEST RIP: %x\n", cpu_vmread(GUEST_RIP));
  printk("GUEST_RFLAGS: %x\n", cpu_vmread(GUEST_RFLAGS));
  printk("VM_INSTRUCTION_ERROR: %x\n", cpu_vmread(VM_INSTRUCTION_ERROR));
  if (exit_reason != EXIT_REASON_MONITOR_TRAP_FLAG) {
    vmm_set_guest_rip(guest_gpr.rip, exit_instruction_length);
  }
  switch (exit_reason) {
    case EXIT_REASON_CPUID: {
      //INFO("handling CPUID (rax = %x)\n", guest_gpr.rax);
      __asm__ __volatile__("cpuid" : "=a" (guest_gpr.rax),
          "=b" (guest_gpr.rbx), "=c" (guest_gpr.rcx),
          "=d" (guest_gpr.rdx) : "a" (guest_gpr.rax));*/
      break;
    }
    case EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED: {
      INFO("TIMER EXPIRED : RIP %X\n", guest_gpr.rip);
      while (1);
      cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE,  99000000); 
      break;
    } 
    case EXIT_REASON_RDMSR: {
      msr_exit++;
      INFO("RDMSR : rcx %X [total : %X]\n", guest_gpr.rcx, msr_exit);
      __asm__ __volatile__("rdmsr" : "=a" (guest_gpr.rax),
          "=b" (guest_gpr.rbx), "=c" (guest_gpr.rcx),
          "=d" (guest_gpr.rdx) : "a" (guest_gpr.rax), "b" (guest_gpr.rbx), "c" (guest_gpr.rcx), "d" (guest_gpr.rdx));
      break;
    }
    case EXIT_REASON_WRMSR: {
      INFO("WRMSR : rcx %X\n", guest_gpr.rcx);
      INFO("a %X, b %X, c %X, d %x\n", guest_gpr.rax, guest_gpr.rbx, guest_gpr.rcx, guest_gpr.rdx);
      __asm__ __volatile__("wrmsr" : "=a" (guest_gpr.rax),
          "=b" (guest_gpr.rbx), "=c" (guest_gpr.rcx),
          "=d" (guest_gpr.rdx) : "a" (guest_gpr.rax), "b" (guest_gpr.rbx), "c" (guest_gpr.rcx), "d" (guest_gpr.rdx));
      INFO("a %X, b %X, c %X, d %x\n", guest_gpr.rax, guest_gpr.rbx, guest_gpr.rcx, guest_gpr.rdx);
      uint64_t key = 0;
      while (key != 0xd0000) {
        uefi_call_wrapper(systab->ConIn->ReadKeyStroke, 2, systab->ConIn, &key);
      }
      break;
    }
    default: {
      INFO("UNDEF\n");
      INFO("Exit reason (base 10) %X\n", exit_reason);
      while(1);
    }
  }
  if (exit_reason != EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED) {
    vmm_set_guest_rip(guest_gpr.rip, exit_instruction_length);
  }


  if (msr_exit == 0x42) {
    INFO("ACTIVATION DU PREEMTION TIMER\n");
    cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE,  100000); 
    uint32_t pinbased_ctls = ACT_VMX_PREEMPT_TIMER;
    cpu_vmwrite(PIN_BASED_VM_EXEC_CONTROL, cpu_adjust32(pinbased_ctls, MSR_ADDRESS_IA32_VMX_PINBASED_CTLS));
  }
  // vmm_set_guest_rip(guest_gpr.rip, exit_instruction_length);
}
