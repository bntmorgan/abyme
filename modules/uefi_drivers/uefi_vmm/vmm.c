#include "vmm.h"
#include "vmx.h"
#include "stdio.h"
#include "cpu.h"
#include "msr.h"
#include "string.h"
#include <efi.h>
#include <efilib.h>

#include "vmcs.h"
#include "pci.h"
#include "mtrr.h"
#include "ept.h"
#include "debug.h"
#ifdef _DEBUG_SERVER
#include "debug_server/debug_server.h"
#endif
#include "mtrr.h"

void vmm_print_guest_regs(struct registers *guest_regs) {
  INFO("rax=%X\n", guest_regs->rax);
  INFO("rbx=%X\n", guest_regs->rbx);
  INFO("rcx=%X\n", guest_regs->rcx);
  INFO("rdx=%X\n", guest_regs->rdx);
}

void vmm_WAIT(void) {
  uint64_t key = 0;
  while (key != 0xd0000) {
    uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &key);
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
  VMM_PANIC_UNKNOWN_CPU_MODE,
  VMM_PANIC_IO,
  VMM_PANIC_XSETBV
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

void vmm_panic(uint64_t code, uint64_t extra, struct registers *guest_regs) {
#ifdef _DEBUG_SERVER
  message_vmm_panic m = {
    MESSAGE_VMM_PANIC,
    debug_server_get_core(),
    code,
    extra
  };
  debug_server_send(&m, sizeof(m));
  if (guest_regs) {
    debug_server_run(guest_regs);
  } else {
    while(1);
  }
#endif
}

uint64_t io_count = 0;
uint64_t cr3_count = 0;

void vmm_handle_vm_exit(struct registers guest_regs) {
  guest_regs.rsp = cpu_vmread(GUEST_RSP);
  guest_regs.rip = cpu_vmread(GUEST_RIP);
  // static uint64_t msr_exit = 0;
  uint32_t exit_reason = cpu_vmread(VM_EXIT_REASON);
  uint32_t exit_qualification = cpu_vmread(EXIT_QUALIFICATION);
  uint8_t mode = vmm_get_cpu_mode(); 

#ifdef _DEBUG_SERVER
  // if (exit_reason == EXIT_REASON_IO_INSTRUCTION) {
  // if (exit_reason == EXIT_REASON_VMCALL) {
  if (exit_reason != EXIT_REASON_CPUID && exit_reason != EXIT_REASON_IO_INSTRUCTION && exit_reason != EXIT_REASON_WRMSR && exit_reason != EXIT_REASON_CR_ACCESS) {
    message_vmexit ms = {
      MESSAGE_VMEXIT,
      debug_server_get_core(),
      exit_reason
    };
    debug_server_send(&ms, sizeof(ms));
    debug_server_run(&guest_regs);
  }
#endif

  uint32_t exit_instruction_length = cpu_vmread(VM_EXIT_INSTRUCTION_LEN);

  switch (exit_reason) {
    case EXIT_REASON_XSETBV: {
      if (vmm_get_cpu_mode() == MODE_LONG) {
        __asm__ __volatile__("xsetbv" : : "a"(guest_regs.rax), "c"(guest_regs.rcx), "d"(guest_regs.rdx));
      } else {
        vmm_panic(VMM_PANIC_XSETBV, 0, &guest_regs);
      }
      break;
    }
    case EXIT_REASON_IO_INSTRUCTION: {
      io_count++;
      // Checking the privileges
      uint8_t cpl = (cpu_vmread(GUEST_CS_AR_BYTES) >> 5) & 3;
      uint8_t iopl = (cpu_vmread(GUEST_RFLAGS) >> 12) & 3;
      if (vmm_get_cpu_mode() == MODE_REAL || cpl <= iopl) {
        // REP prefixed || String I/O
        if ((exit_qualification & 0x20) || (exit_qualification & 0x10)) {
          vmm_panic(VMM_PANIC_IO, 0, &guest_regs);
        }
        uint8_t direction = exit_qualification & 8;
        uint8_t size = exit_qualification & 7;
        uint16_t port = (exit_qualification >> 16) & 0xffff;
        // out
        uint32_t v = guest_regs.rax;
        if (direction == 0) {
          if (pci_no_protect_out(port, v)) {
            if (size == 0) {
              __asm__ __volatile__("out %%al, %%dx" : : "a"(v), "d"(port));
            } else if (size == 1) {
              __asm__ __volatile__("out %%ax, %%dx" : : "a"(v), "d"(port));
            } else if (size == 3) {
              __asm__ __volatile__("out %%eax, %%dx" : : "a"(v), "d"(port));
            } else {
              vmm_panic(VMM_PANIC_IO, 1, &guest_regs);
            }
          }
        // in
        } else {
          if (pci_no_protect_in(port)) {
            if (size == 0) {
              __asm__ __volatile__("in %%dx, %%al" : "=a"(v) : "d"(port));
              guest_regs.rax = (guest_regs.rax & 0xffffffffffffff00) | (v & 0x000000ff);
            } else if (size == 1) {
              __asm__ __volatile__("in %%dx, %%ax" : "=a"(v) : "d"(port));
              guest_regs.rax = (guest_regs.rax & 0xffffffffffff0000) | (v & 0x0000ffff);
            } else if (size == 3) {
              __asm__ __volatile__("in %%dx, %%eax" : "=a"(v) : "d"(port));
              guest_regs.rax = (guest_regs.rax & 0xffffffff00000000) | (v & 0xffffffff);
            } else {
              vmm_panic(VMM_PANIC_IO, port, &guest_regs);
            }
          } else {
            if (size == 0) {
              guest_regs.rax = guest_regs.rax | 0x000000ff;
            } else if (size == 1) {
              guest_regs.rax = guest_regs.rax | 0x0000ffff;
            } else if (size == 3) {
              guest_regs.rax = guest_regs.rax | 0xffffffff;
            } else {
              vmm_panic(VMM_PANIC_IO, 2, &guest_regs);
            }
          }
        }
      // Unsufficient privileges
      } else {
        vmm_panic(VMM_PANIC_IO, 3, &guest_regs);
      }
      break;
    }
    case EXIT_REASON_CPUID: {
      //vmm_print_guest_regs(&guest_regs);
      if (guest_regs.rax == 0x88888888) {
        guest_regs.rax = 0xC001C001C001C001;
        guest_regs.rbx = io_count;
        guest_regs.rcx = cr3_count;
      } else if (guest_regs.rax == 0x99999999) {
        vmm_panic(VMM_PANIC_RDMSR, 1234, &guest_regs);
      } else if (guest_regs.rax == 0x0) {
        __asm__ __volatile__("cpuid"
            : "=a" (guest_regs.rax), "=b" (guest_regs.rbx), "=c" (guest_regs.rcx), "=d" (guest_regs.rdx)
            :  "a" (guest_regs.rax),  "b" (guest_regs.rbx),  "c" (guest_regs.rcx),  "d" (guest_regs.rdx));
        char *gilles = "30000%MAKINA";
        guest_regs.rbx = *((uint32_t *)gilles);
        guest_regs.rdx = *((uint32_t *)gilles + 1);
        guest_regs.rcx = *((uint32_t *)gilles + 2);
      } else {
        __asm__ __volatile__("cpuid"
            : "=a" (guest_regs.rax), "=b" (guest_regs.rbx), "=c" (guest_regs.rcx), "=d" (guest_regs.rdx)
            :  "a" (guest_regs.rax),  "b" (guest_regs.rbx),  "c" (guest_regs.rcx),  "d" (guest_regs.rdx));
        //vmm_print_guest_regs(&guest_regs);
      }
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
        if (guest_regs.rcx > 0xc0001fff || (guest_regs.rcx > 0x1fff && guest_regs.rcx < 0xc0000000)) {
          // Tells the vm that the msr doesn't exist
          uint32_t it_info_field =    (0x1 << 11)     // push error code
                                    | (0x3 << 8)      // hardware exception
                                    | 0xd ;           // GP fault
          cpu_vmwrite(VM_ENTRY_INTR_INFO_FIELD, it_info_field);
        } else {
          __asm__ __volatile__("rdmsr"
            : "=a" (guest_regs.rax), "=b" (guest_regs.rbx), "=c" (guest_regs.rcx), "=d" (guest_regs.rdx)
            :  "a" (guest_regs.rax),  "b" (guest_regs.rbx),  "c" (guest_regs.rcx),  "d" (guest_regs.rdx));
        }
      }
      break;
    }
    case EXIT_REASON_WRMSR: {
      // Check for variable mtrr msr
      uint64_t msr_base_address = MSR_ADDRESS_IA32_MTRR_PHYBASE0;
      uint64_t msr_mask_address = MSR_ADDRESS_IA32_MTRR_PHYBASE0 + 1;
      uint8_t is_var_mtrr = 0;
      uint32_t i;
      // mtrr_cap is a mtrr.c extern
      for (i = 0; i < mtrr_cap.msr.vcnt; i++) {
        if (guest_regs.rcx == msr_base_address || guest_regs.rcx == msr_mask_address) {
          is_var_mtrr = 1;
        }
        msr_base_address += 2;
        msr_mask_address += 2;
      }
      if (guest_regs.rcx == MSR_ADDRESS_IA32_EFER) {
        cpu_vmwrite(GUEST_IA32_EFER, guest_regs.rax & 0xffffffff);
        cpu_vmwrite(GUEST_IA32_EFER_HIGH, guest_regs.rdx & 0xffffffff);
      } else if (
          is_var_mtrr ||
          guest_regs.rcx == MSR_ADDRESS_IA32_MTRRCAP ||
          guest_regs.rcx == MSR_ADDRESS_A32_MTRR_DEF_TYPE ||
          guest_regs.rcx == MSR_ADDRESS_IA32_MTRR_FIX64K_00000 ||
          guest_regs.rcx == MSR_ADDRESS_IA32_MTRR_FIX16K_80000 ||
          guest_regs.rcx == MSR_ADDRESS_IA32_MTRR_FIX16K_A0000 ||
          guest_regs.rcx == MSR_ADDRESS_IA32_MTRR_FIX4K_C0000 ||
          guest_regs.rcx == MSR_ADDRESS_IA32_MTRR_FIX4K_C8000 ||
          guest_regs.rcx == MSR_ADDRESS_IA32_MTRR_FIX4K_D0000 ||
          guest_regs.rcx == MSR_ADDRESS_IA32_MTRR_FIX4K_D8000 ||
          guest_regs.rcx == MSR_ADDRESS_IA32_MTRR_FIX4K_E0000 ||
          guest_regs.rcx == MSR_ADDRESS_IA32_MTRR_FIX4K_E8000 ||
          guest_regs.rcx == MSR_ADDRESS_IA32_MTRR_FIX4K_F0000 ||
          guest_regs.rcx == MSR_ADDRESS_IA32_MTRR_FIX4K_F8000 ) {
        __asm__ __volatile__("wrmsr"
          : : "a" (guest_regs.rax), "b" (guest_regs.rbx), "c" (guest_regs.rcx), "d" (guest_regs.rdx));
        // Recompute the cache ranges
        mtrr_create_ranges();
        // Recompute ept tables
        ept_create_tables();
      } else {
        vmm_panic(VMM_PANIC_WRMSR, 0, &guest_regs);
      }
      break;
    }
    case EXIT_REASON_CR_ACCESS: {
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
      // Mov to CRX
      if (a != 0) {
        vmm_panic(VMM_PANIC_CR_ACCESS, 0, &guest_regs);
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
            vmm_panic(VMM_PANIC_CR_ACCESS, 0, &guest_regs);
          }
          // Write LMA
          cpu_vmwrite(GUEST_IA32_EFER, value_IA32_EFER | (1 << 10));
          cpu_vmwrite(VM_ENTRY_CONTROLS, vm_entry_controls | (1 << 9));
        } else if (((previous_cr0 >> 31) & 1) != ((value >> 31) & 1)) {
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
      } else if (n == 3) {
        cr3_count++;
#ifdef _DEBUG_SERVER
        debug_server_log_cr3_add(&guest_regs, cpu_vmread(GUEST_CR3));
#endif
        uint8_t desc[16] = { cpu_vmread(VIRTUAL_PROCESSOR_ID), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        uint64_t un = 1;
        __asm__ __volatile__("invvpid %1, %%rax" : : "a"(un), "m"(*desc) : "memory");
        cpu_vmwrite(GUEST_CR3, value);
      } else {
        vmm_panic(VMM_PANIC_CR_ACCESS, 0, &guest_regs);
      }
      break;
    }
    case EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED: {
      uint32_t preemption_timer_value = 0xffffff;
      cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE, preemption_timer_value);
      // Don't increment RIP
      return;
      break;
    }
    case EXIT_REASON_MONITOR_TRAP_FLAG:
      // Don't increment RIP
      return;
      break;
    default: {
#ifdef _DEBUG_SERVER
      message_vmexit ms = {
        MESSAGE_UNHANDLED_VMEXIT,
        debug_server_get_core(),
        exit_reason
      };
      debug_server_send(&ms, sizeof(ms));
      debug_server_run(&guest_regs);
#endif
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
      break;
    }
    case MODE_LONG: {
      uint64_t tmp = (uint64_t) guest_regs.rip;
      tmp = tmp + (uint64_t) exit_instruction_length;
      cpu_vmwrite(GUEST_RIP, (guest_regs.rip & 0x0000000000000000) | tmp);
      break;
    }
    default :
      vmm_panic(VMM_PANIC_UNKNOWN_CPU_MODE, 0, &guest_regs);
  }
}

void msr_bitmap_changes(void) {
  //msr_bitmap_get_ptr();
}
