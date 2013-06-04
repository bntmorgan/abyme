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

enum CPU_MODE {
  MODE_REAL,
  MODE_PROTECTED,
  MODE_LONG
};

enum VMM_PANIC {
  VMM_PANIC_RDMSR,
  VMM_PANIC_WRMSR,
  VMM_PANIC_CR_ACCESS,
  VMM_PANIC_UNKNOWN_CPU_MODE
};

int vmm_get_cpu_mode() {
  uint64_t cr0 = cpu_vmread(GUEST_CR0);
  uint64_t ia32_efer = cpu_vmread(GUEST_IA32_EFER);
  if (!(cr0 & (1 << 0))) {
    return MODE_REAL;
  } else if (!(ia32_efer & (1 << 10))) {
    return MODE_PROTECTED;
  } else {
    return MODE_LONG;
  }
}

void vmm_panic(uint64_t code) {
  message_vmm_panic m = {
    MESSAGE_VMM_PANIC,
    debug_server_get_core(),
    code
  };
  debug_server_send(&m, sizeof(m));
}

void vmm_handle_vm_exit(struct registers guest_regs) {
  guest_regs.rsp = cpu_vmread(GUEST_RSP);
  guest_regs.rip = cpu_vmread(GUEST_RIP);
  // static uint64_t msr_exit = 0;
  uint32_t exit_reason = cpu_vmread(VM_EXIT_REASON);
  uint32_t exit_qualification = cpu_vmread(EXIT_QUALIFICATION);
  uint8_t mode = vmm_get_cpu_mode(); 

  //if (exit_reason != EXIT_REASON_IO_INSTRUCTION) {
  if (exit_reason == EXIT_REASON_VMCALL) {
    message_vmexit ms = {
      MESSAGE_VMEXIT,
      debug_server_get_core(),
      exit_reason
    };
    debug_server_send(&ms, sizeof(ms));
    debug_server_run(&guest_regs);
  }

  uint32_t exit_instruction_length = cpu_vmread(VM_EXIT_INSTRUCTION_LEN);

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
    case EXIT_REASON_RDMSR: {
      if (guest_regs.rcx == MSR_ADDRESS_IA32_EFER) {
        if (mode == MODE_LONG) {
          guest_regs.rdx = 0;
          guest_regs.rax = 0;
        }
        guest_regs.rax = (guest_regs.rax & (0xffffffff00000000)) | (cpu_vmread(GUEST_IA32_EFER) & 0xffffffff);
        guest_regs.rdx = (guest_regs.rdx & (0xffffffff00000000)) | (cpu_vmread(GUEST_IA32_EFER_HIGH) & 0xffffffff);
      } else {
        vmm_panic(VMM_PANIC_RDMSR);
      }
      /*__asm__ __volatile__("rdmsr"
          : "=a" (guest_regs.rax), "=b" (guest_regs.rbx), "=c" (guest_regs.rcx), "=d" (guest_regs.rdx)
          :  "a" (guest_regs.rax),  "b" (guest_regs.rbx),  "c" (guest_regs.rcx),  "d" (guest_regs.rdx));*/
      break;
    }
    case EXIT_REASON_WRMSR: {
      if (guest_regs.rcx == MSR_ADDRESS_IA32_EFER) {
        cpu_vmwrite(GUEST_IA32_EFER, guest_regs.rax & 0xffffffff);
        cpu_vmwrite(GUEST_IA32_EFER_HIGH, guest_regs.rdx & 0xffffffff);
      } else {
        vmm_panic(VMM_PANIC_WRMSR);
      }
      break;
    }
    case EXIT_REASON_CR_ACCESS : {
      uint8_t o = (exit_qualification >> 8) & 0xf;
      uint8_t n = (exit_qualification >> 0) & 0xf;
      uint8_t a = (exit_qualification >> 4) & 0x3;
      uint8_t offset[16] = {
        (uint8_t) (uint64_t) &(((struct registers *) 0)->rax),
        (uint8_t) (uint64_t) &(((struct registers *) 0)->rcx),
        (uint8_t) (uint64_t) &(((struct registers *) 0)->rdx),
        (uint8_t) (uint64_t) &(((struct registers *) 0)->rbx),
        (uint8_t) (uint64_t) &(((struct registers *) 0)->rsp),
        (uint8_t) (uint64_t) &(((struct registers *) 0)->rbp),
        (uint8_t) (uint64_t) &(((struct registers *) 0)->rsi),
        (uint8_t) (uint64_t) &(((struct registers *) 0)->rdi),
        (uint8_t) (uint64_t) &(((struct registers *) 0)->r8),
        (uint8_t) (uint64_t) &(((struct registers *) 0)->r9),
        (uint8_t) (uint64_t) &(((struct registers *) 0)->r10),
        (uint8_t) (uint64_t) &(((struct registers *) 0)->r11),
        (uint8_t) (uint64_t) &(((struct registers *) 0)->r12),
        (uint8_t) (uint64_t) &(((struct registers *) 0)->r13),
        (uint8_t) (uint64_t) &(((struct registers *) 0)->r14),
        (uint8_t) (uint64_t) &(((struct registers *) 0)->r15)
      };
      if (a != 0) {
        vmm_panic(VMM_PANIC_CR_ACCESS);
      }
      uint64_t value = *((uint64_t *)(((uint8_t *)&guest_regs) + offset[o]));
      // printk("Value %016X offset %d\n", value, o);
      if (n == 0) {
        uint64_t previous_cr0 = cpu_vmread(GUEST_CR0);
        uint64_t value_IA32_EFER = cpu_vmread(GUEST_IA32_EFER);
        uint64_t vm_entry_controls = cpu_vmread(VM_ENTRY_CONTROLS);
        // Guest is attempting entering into long mode by activating
        // paging, we need to write IA32_EFER.LMA to one !
        if (((previous_cr0 >> 31) & 1) == 0 && ((value >> 31) & 1) && ((value_IA32_EFER >> 8) & 1)) {
          // We still are in long mode but without paging ???
          if ((value_IA32_EFER >> 10) & 1) {
            vmm_panic(VMM_PANIC_CR_ACCESS);
          }
          // Write LMA
          cpu_vmwrite(GUEST_IA32_EFER, value_IA32_EFER | (1 << 10));
          cpu_vmwrite(VM_ENTRY_CONTROLS, vm_entry_controls | (1 << 9));
        } else if (((previous_cr0 >> 31) & 1) != ((value >> 31) & 1)) {
          //vmm_panic(2);
          cpu_vmwrite(GUEST_IA32_EFER, value_IA32_EFER & ~(1 << 10));
          cpu_vmwrite(VM_ENTRY_CONTROLS, vm_entry_controls & ~(1 << 9));
        }
        // printk("CR0 %016X, SHAD CR0 %016X\n", cpu_vmread(GUEST_CR0), cpu_vmread(CR0_READ_SHADOW));
        // cpu_vmwrite(GUEST_CR0, cpu_adjust64(value, MSR_ADDRESS_VMX_CR0_FIXED0, MSR_ADDRESS_VMX_CR0_FIXED1));
        // cpu_vmwrite(GUEST_CR0, (value | 0x20));
        cpu_vmwrite(GUEST_CR0, (value & ((msr_read(MSR_ADDRESS_VMX_CR0_FIXED1)) | (0xe0000001))) | (msr_read(MSR_ADDRESS_VMX_CR0_FIXED0)  & ~(0xe0000001)));
        cpu_vmwrite(CR0_READ_SHADOW, value);
        // printk("CR0 %016X, SHAD CR0 %016X\n", cpu_vmread(GUEST_CR0), cpu_vmread(CR0_READ_SHADOW));
      } else if (n == 4) {
        // printk("CR4 %016X, SHAD CR4 %016X\n", cpu_vmread(GUEST_CR4), cpu_vmread(CR4_READ_SHADOW));
        cpu_vmwrite(GUEST_CR4, cpu_adjust64(value, MSR_ADDRESS_VMX_CR4_FIXED0, MSR_ADDRESS_VMX_CR4_FIXED1));
        cpu_vmwrite(CR4_READ_SHADOW, value);
        // printk("CR4 %016X, SHAD CR4 %016X\n", cpu_vmread(GUEST_CR4), cpu_vmread(CR4_READ_SHADOW));
      } else {
        vmm_panic(VMM_PANIC_CR_ACCESS);
      }
      break;
    }
    case EXIT_REASON_MONITOR_TRAP_FLAG:
      // Don't increment RIP
      return;
    default: {
      message_vmexit ms = {
        MESSAGE_UNHANDLED_VMEXIT,
        debug_server_get_core(),
        exit_reason
      };
      debug_server_send(&ms, sizeof(ms));
      debug_server_run(&guest_regs);
    }
  }
  switch (mode) {
    case MODE_REAL: {
      uint16_t tmp = (uint16_t) guest_regs.rip;
      tmp = tmp + (uint16_t) exit_instruction_length;
      cpu_vmwrite(GUEST_RIP, (guest_regs.rip & 0xffffffffffff0000) | tmp);
      break;
    }
    case MODE_PROTECTED: {
      uint32_t tmp = (uint32_t) guest_regs.rip;
      tmp = tmp + (uint32_t) exit_instruction_length;
      cpu_vmwrite(GUEST_RIP, (guest_regs.rip & 0xffffffff00000000) | tmp);
      //cpu_vmwrite(GUEST_RIP, guest_regs.rip + (uint32_t)exit_instruction_length);
      break;
    }
    case MODE_LONG : {
      uint64_t tmp = (uint64_t) guest_regs.rip;
      tmp = tmp + (uint64_t) exit_instruction_length;
      cpu_vmwrite(GUEST_RIP, (guest_regs.rip & 0x0000000000000000) | tmp);
      //cpu_vmwrite(GUEST_RIP, guest_regs.rip + (uint64_t)exit_instruction_length);
      break;
    }
    default : 
      vmm_panic(VMM_PANIC_UNKNOWN_CPU_MODE);
  }
}
