#include "vmm.h"
#include "stdio.h"
#include "cpu.h"
#include "msr.h"
#include <efi.h>
#include <efilib.h>

#include "systab.h"
#include "vmcs.h"
#include "debug.h"

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
  
  //
  // Tests
  //
#include <efi.h>
#include <efilib.h>
#include "efi/efiapi_1_1.h"
#include "efi/efi_1_1.h"

#define EFI_PCI_IO_PROTOCOL_GUID \
    { 0x4cf5b200, 0x68b8, 0x4ca5, {0x9e, 0xec, 0xb2, 0x3e, 0x3f, 0x50, 0x2, 0x9a } }
        EFI_STATUS status;

  EFI_GUID pci_io_guid = EFI_PCI_IO_PROTOCOL_GUID;
  EFI_PCI_IO_PROTOCOL *PciIo;
  status = uefi_call_wrapper(GET_EFI_BOOT_SERVICES_1_1(systab->BootServices)->LocateProtocol,
                             3,
                             &pci_io_guid,
                             NULL,
                             (void **)&PciIo);

  //Print(L"PCI IO %d\n", status);
  if (status == 0) {
    uint8_t Character = 'c';
    uint32_t Row = 0, Column = 0;
    uint32_t VideoAddress = 0xB8000 + (Row * 80 + Column) * 2;
    uint16_t VideoCharacter = 0x0700 | Character;
    uint32_t i;
    for (i = 0; i < 0x2000; i++) {
      Row = i;
      Column = i;
      status = uefi_call_wrapper(PciIo->Mem.Write, 6, PciIo, EfiPciIoWidthUint16, 
          EFI_PCI_IO_PASS_THROUGH_BAR, VideoAddress, 1, &VideoCharacter);
    }

    uint32_t Value = 0x03;
    status = uefi_call_wrapper(PciIo->Io.Write, 6, PciIo, EfiPciIoWidthUint8,
        EFI_PCI_IO_PASS_THROUGH_BAR, 0x80, 1, &Value);

    *((uint8_t *) (0xb8000 + 2)) = VideoCharacter;

  } else {
    Print(L"Error while retreiving the asshole\n");
  }

  switch (exit_reason) {
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
