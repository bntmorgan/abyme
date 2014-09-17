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

uint8_t vmm_stack[VMM_STACK_SIZE];

static uint8_t send_debug[NB_EXIT_REASONS];

static uint64_t io_count = 0;
static uint64_t cr3_count = 0;

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

static inline int get_cpu_mode(void);
static void vmm_panic(uint64_t code, uint64_t extra, struct registers *guest_regs);
static inline void increment_rip(uint8_t cpu_mode, struct registers *guest_regs);
static inline uint8_t is_MTRR(uint64_t msr_addr);

void vmm_init(void) {
  /* Init exit reasons for which we need to send a debug message */
  memset(&send_debug[0], 1, NB_EXIT_REASONS);
  send_debug[EXIT_REASON_CPUID] = 0;
  send_debug[EXIT_REASON_IO_INSTRUCTION] = 0;
  send_debug[EXIT_REASON_WRMSR] = 0;
  send_debug[EXIT_REASON_RDMSR] = 0;
  send_debug[EXIT_REASON_XSETBV] = 0;
  send_debug[EXIT_REASON_CR_ACCESS] = 0;
  send_debug[EXIT_REASON_INVVPID] = 0;
  send_debug[EXIT_REASON_VMRESUME] = 0;
}

void vmm_handle_vm_exit(struct registers guest_regs) {
  guest_regs.rsp = cpu_vmread(GUEST_RSP);
  guest_regs.rip = cpu_vmread(GUEST_RIP);

  uint32_t exit_reason = cpu_vmread(VM_EXIT_REASON);
  uint64_t exit_qualification = cpu_vmread(EXIT_QUALIFICATION);
  uint8_t cpu_mode = get_cpu_mode();

  // check VMX abort
  uint32_t vmx_abort = *(uint32_t*)&vmcs0[1];
  if (vmx_abort) {
    printk("VMX abort detected : %d\n", vmx_abort);
    vmm_panic(0,0,&guest_regs);
  }

#ifdef _DEBUG_SERVER
  if (send_debug[exit_reason]) {
    message_vmexit ms = {
      MESSAGE_VMEXIT,
      debug_server_get_core(),
      exit_reason
    };
    debug_server_send(&ms, sizeof(ms));
    debug_server_run(&guest_regs);
  }
#endif

  //
  // VMX Specific VMexits that we override
  //
  if (exit_reason == EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED) {
    vmcs_set_vmx_preemption_timer_value(VMCS_DEFAULT_PREEMPTION_TIMER_MICROSEC);
    return;
  } else if (exit_reason == EXIT_REASON_MONITOR_TRAP_FLAG) {
    return;
  }

  switch (exit_reason) {
    //
    // Things we should emulate/protect
    //
    case EXIT_REASON_XSETBV: {
      if (cpu_mode == MODE_LONG) {
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
      if (cpu_mode == MODE_REAL || cpl <= iopl) {
        // REP prefixed || String I/O
        if ((exit_qualification & 0x20) || (exit_qualification & 0x10)) {
          vmm_panic(VMM_PANIC_IO, 0, &guest_regs);
        }
        uint8_t direction = exit_qualification & 8;
        uint8_t size = exit_qualification & 7;
        uint8_t string = exit_qualification & (1<<4);
        uint8_t rep = exit_qualification & (1<<5);
        uint16_t port = (exit_qualification >> 16) & 0xffff;

        // Unsupported
        if (rep || string) {
          vmm_panic(VMM_PANIC_IO, 4, &guest_regs);
        }

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
      /*if (guest_regs.rax == 0x88888888) {
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
      } else {*/
        __asm__ __volatile__("cpuid"
            : "=a" (guest_regs.rax), "=b" (guest_regs.rbx), "=c" (guest_regs.rcx), "=d" (guest_regs.rdx)
            :  "a" (guest_regs.rax),  "b" (guest_regs.rbx),  "c" (guest_regs.rcx),  "d" (guest_regs.rdx));
      /*}*/
      break;
    }
    case EXIT_REASON_RDMSR: {
      if (guest_regs.rcx == MSR_ADDRESS_IA32_EFER) {
        if (cpu_mode == MODE_LONG) {
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
      if (guest_regs.rcx == MSR_ADDRESS_IA32_EFER) {
        cpu_vmwrite(GUEST_IA32_EFER, guest_regs.rax & 0xffffffff);
        cpu_vmwrite(GUEST_IA32_EFER_HIGH, guest_regs.rdx & 0xffffffff);
      } else {
        if (is_MTRR(guest_regs.rcx)) {
          __asm__ __volatile__("wrmsr"
            : : "a" (guest_regs.rax), "b" (guest_regs.rbx), "c" (guest_regs.rcx), "d" (guest_regs.rdx));
          // Recompute the cache ranges
          uint8_t need_recompute_ept = mtrr_update_ranges();
          // Recompute ept tables
          if (need_recompute_ept) {
            ept_create_tables();
          }
        } else {
          vmm_panic(VMM_PANIC_WRMSR, 0, &guest_regs);
        }
      }
      break;
    }
    case EXIT_REASON_CR_ACCESS: {
      uint8_t o = (exit_qualification >> 8) & 0xf;
      uint8_t n = (exit_qualification >> 0) & 0xf;
      uint8_t a = (exit_qualification >> 4) & 0x3;
      // Mov to CRX
      if (a != 0) {
        vmm_panic(VMM_PANIC_CR_ACCESS, 0, &guest_regs);
      }
      uint64_t value = ((uint64_t*)&guest_regs)[o];
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
        // désactivation de l'expérience
        // debug_server_log_cr3_add(&guest_regs, cpu_vmread(GUEST_CR3));
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
    //
    // Debug
    //
    case EXIT_REASON_EPT_VIOLATION: {
      uint64_t guest_linear_addr = cpu_vmread(GUEST_LINEAR_ADDRESS);
      INFO("EPT violation, Qualification : %X, addr : %X\n", exit_qualification, guest_linear_addr);
      break;
    }
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

  increment_rip(cpu_mode, &guest_regs);
}

static inline int get_cpu_mode(void) {
  uint64_t cr0 = cpu_vmread(GUEST_CR0);
  uint64_t ia32_efer = cpu_vmread(GUEST_IA32_EFER);
  if (!(cr0 & (1 << 0))) {
    INFO("On est en mode réel\n");
    return MODE_REAL;
  } else if (!(ia32_efer & (1 << 10))) {
    INFO("On est en mode protégé\n");
    return MODE_PROTECTED;
  } else {
    return MODE_LONG;
  }
}

static inline uint8_t is_MTRR(uint64_t msr_addr) {
  return (/* Variable MTRRs */
          ((msr_addr >= MSR_ADDRESS_IA32_MTRR_PHYSBASE0) &&
           (msr_addr < MSR_ADDRESS_IA32_MTRR_PHYSBASE0 + 2*mtrr_get_nb_variable_mtrr()))
        ||/* Fixed MTRRs */
          (msr_addr == MSR_ADDRESS_IA32_MTRR_DEF_TYPE     ||
           msr_addr == MSR_ADDRESS_IA32_MTRR_FIX64K_00000 ||
           msr_addr == MSR_ADDRESS_IA32_MTRR_FIX16K_80000 ||
           msr_addr == MSR_ADDRESS_IA32_MTRR_FIX16K_A0000 ||
           msr_addr == MSR_ADDRESS_IA32_MTRR_FIX4K_C0000  ||
           msr_addr == MSR_ADDRESS_IA32_MTRR_FIX4K_C8000  ||
           msr_addr == MSR_ADDRESS_IA32_MTRR_FIX4K_D0000  ||
           msr_addr == MSR_ADDRESS_IA32_MTRR_FIX4K_D8000  ||
           msr_addr == MSR_ADDRESS_IA32_MTRR_FIX4K_E0000  ||
           msr_addr == MSR_ADDRESS_IA32_MTRR_FIX4K_E8000  ||
           msr_addr == MSR_ADDRESS_IA32_MTRR_FIX4K_F0000  ||
           msr_addr == MSR_ADDRESS_IA32_MTRR_FIX4K_F8000));
}

static inline void increment_rip(uint8_t cpu_mode, struct registers *guest_regs) {
  uint32_t exit_instruction_length = cpu_vmread(VM_EXIT_INSTRUCTION_LEN);

  switch (cpu_mode) {
    case MODE_REAL: {
      uint16_t tmp = (uint16_t) guest_regs->rip;
      tmp = tmp + (uint16_t) exit_instruction_length;
      cpu_vmwrite(GUEST_RIP, (guest_regs->rip & 0xffffffffffff0000) | tmp);
      break;
    }
    case MODE_PROTECTED: {
      uint32_t tmp = (uint32_t) guest_regs->rip;
      tmp = tmp + (uint32_t) exit_instruction_length;
      cpu_vmwrite(GUEST_RIP, (guest_regs->rip & 0xffffffff00000000) | tmp);
      break;
    }
    case MODE_LONG: {
      uint64_t tmp = (uint64_t) guest_regs->rip;
      tmp = tmp + (uint64_t) exit_instruction_length;
      cpu_vmwrite(GUEST_RIP, (guest_regs->rip & 0x0000000000000000) | tmp);
      break;
    }
    default :
      vmm_panic(VMM_PANIC_UNKNOWN_CPU_MODE, 0, guest_regs);
  }
}

static void vmm_panic(uint64_t code, uint64_t extra, struct registers *guest_regs) {
#ifdef _DEBUG_SERVER
  message_vmm_panic m = {
    MESSAGE_VMM_PANIC,
    debug_server_get_core(),
    code,
    extra
  };
  debug_server_send(&m, sizeof(m));
  if (guest_regs != NULL) {
    debug_server_run(guest_regs);
  } else {
    while(1);
  }
#endif
}
