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
#include "nested_vmx.h"
#include "level.h"

uint8_t vmm_stack[VMM_STACK_SIZE];

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
static inline void* get_instr_param_ptr(struct registers *guest_regs);
static void vmm_panic(uint8_t core, uint64_t code, uint64_t extra, struct
    registers *guest_regs);
static inline void increment_rip(uint8_t cpu_mode, struct registers *guest_regs);
static inline uint8_t is_MTRR(uint64_t msr_addr);

void vmm_init(void) { }

void vmm_handle_vm_exit(struct registers guest_regs) {

//   if (ns.state == NESTED_GUEST_RUNNING) {
//     INFO("Guest running\n");
//   } else if (ns.state == NESTED_HOST_RUNNING) {
//     INFO("Host running\n");
//   } else {
//     INFO("Not nested\n");
//   }

  guest_regs.rsp = cpu_vmread(GUEST_RSP);
  guest_regs.rip = cpu_vmread(GUEST_RIP);

  uint32_t exit_reason = cpu_vmread(VM_EXIT_REASON);
  uint64_t exit_qualification = cpu_vmread(EXIT_QUALIFICATION);
  uint8_t cpu_mode = get_cpu_mode();
  uint8_t** instr_param_ptr;

  // check VMX abort
  uint32_t vmx_abort = (ns.state != NESTED_GUEST_RUNNING)
                        ? ((uint32_t*)vmcs0)[1]
                        : ((uint32_t*)guest_vmcs[0])[1];
  if (vmx_abort) {
    printk("VMX abort detected : %d\n", vmx_abort);
    vmm_panic(ns.state, 0, 0, &guest_regs);
  }

#ifdef _DEBUG_SERVER
  // if (debug_server) {
  if (debug_server/* && ns.nested_level == 3*/) {
    debug_server_vmexit(ns.nested_level, exit_reason, &guest_regs);
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


  if (ns.state == NESTED_GUEST_RUNNING) {
    //if (debug_server_level == 1) {
      // static uint32_t lol = 0;
      // if (lol < 2) {
      //  LEVEL(1, "state %d, shadow_idx %d, nested_level %d, grip 0x%016X\n", ns.state,
      //      ns.shadow_idx, ns.nested_level, guest_regs.rip);
      //  lol++;
      // } else {
      // ERROR("Here we are\n");
      // }
    //}
    // Forward to L2 host
    nested_load_host();
    return;
  }

  switch (exit_reason) {
    //
    // VMX Operations
    //
    case EXIT_REASON_VMXON:
      instr_param_ptr = get_instr_param_ptr(&guest_regs);
      nested_vmxon(*instr_param_ptr);
      break;
    case EXIT_REASON_VMCLEAR:
      instr_param_ptr = get_instr_param_ptr(&guest_regs);
      nested_vmclear(*instr_param_ptr);
      break;
    case EXIT_REASON_VMPTRLD:
      instr_param_ptr = get_instr_param_ptr(&guest_regs);
      nested_vmptrld(*instr_param_ptr, &guest_regs);
      break;
    case EXIT_REASON_VMLAUNCH:
      nested_vmlaunch(&guest_regs);
      break;
    case EXIT_REASON_VMRESUME:
      // return to L2 guest
      nested_vmresume(&guest_regs);
      return;
      break;
    case EXIT_REASON_INVVPID: {
      uint8_t* desc = get_instr_param_ptr(&guest_regs);
      uint64_t type = ((uint64_t*)&guest_regs)[(cpu_vmread(VMX_INSTRUCTION_INFO) >> 28) & 0xF];
      // INFO("INVVPID VPID(0x%04x), addr(0x%016X), type(0x%016X)\n", ((uint64_t*)desc)[1],
      //    ((uint16_t*)desc)[0], type);
      // We need to increment the VPID
      ((uint16_t*)desc)[0]++;
      // INFO("INVVPID VPID(0x%04x)\n", ((uint16_t*)desc)[0]);
      __asm__ __volatile__("invvpid %0, %1" : : "m"(*desc), "r"(type) );
      break;
    }
    case EXIT_REASON_VMREAD:
    case EXIT_REASON_VMWRITE: {
      uint8_t operand_in_register = (cpu_vmread(VMX_INSTRUCTION_INFO) >> 10) & 0x1;
      uint64_t field = ((uint64_t*)&guest_regs)[(cpu_vmread(VMX_INSTRUCTION_INFO) >> 28) & 0xF];
      uint64_t* value_ptr = NULL;

      if (operand_in_register) {
        value_ptr = &((uint64_t*)&guest_regs)[(cpu_vmread(VMX_INSTRUCTION_INFO) >> 3) & 0xF];
      } else {
        value_ptr = get_instr_param_ptr(&guest_regs);
      }

      if (exit_reason == EXIT_REASON_VMREAD) {
        *value_ptr = nested_vmread(field);
      } else {
        nested_vmwrite(field, *value_ptr);
      }

      break;
    }
    //
    // Things we should emulate/protect
    //
    case EXIT_REASON_XSETBV: {
      if (cpu_mode == MODE_LONG) {
        __asm__ __volatile__("xsetbv" : : "a"(guest_regs.rax), "c"(guest_regs.rcx), "d"(guest_regs.rdx));
      } else {
        vmm_panic(ns.state, VMM_PANIC_XSETBV, 0, &guest_regs);
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
          vmm_panic(ns.state, VMM_PANIC_IO, 0, &guest_regs);
        }
        uint8_t direction = exit_qualification & 8;
        uint8_t size = exit_qualification & 7;
        uint8_t string = exit_qualification & (1<<4);
        uint8_t rep = exit_qualification & (1<<5);
        uint16_t port = (exit_qualification >> 16) & 0xffff;

        // Unsupported
        if (rep || string) {
          vmm_panic(ns.state, VMM_PANIC_IO, 4, &guest_regs);
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
              vmm_panic(ns.state, VMM_PANIC_IO, 1, &guest_regs);
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
              vmm_panic(ns.state, VMM_PANIC_IO, port, &guest_regs);
            }
          } else {
            if (size == 0) {
              guest_regs.rax = guest_regs.rax | 0x000000ff;
            } else if (size == 1) {
              guest_regs.rax = guest_regs.rax | 0x0000ffff;
            } else if (size == 3) {
              guest_regs.rax = guest_regs.rax | 0xffffffff;
            } else {
              vmm_panic(ns.state, VMM_PANIC_IO, 2, &guest_regs);
            }
          }
        }
      // Unsufficient privileges
      } else {
        vmm_panic(ns.state, VMM_PANIC_IO, 3, &guest_regs);
      }
      break;
    }
    case EXIT_REASON_CPUID: {
      if (guest_regs.rax == 0x77777777) {
        guest_regs.rbx =
          (uint64_t)'l' << 0 | (uint64_t)'1' << 8 | (uint64_t)'i' << 16 |
          (uint64_t)'s' << 24 | (uint64_t)'h' << 32 | (uint64_t)'e' << 40 |
          (uint64_t)'r' << 48 | (uint64_t)'e' << 56;
        guest_regs.rcx =
          (uint64_t)'l' << 0 | (uint64_t)'0' << 8 | (uint64_t)'i' << 16 |
          (uint64_t)'s' << 24 | (uint64_t)'h' << 32 | (uint64_t)'e' << 40 |
          (uint64_t)'r' << 48 | (uint64_t)'e' << 56;
      } else if (guest_regs.rax == 0x88888888) {
        guest_regs.rax = 0xC001C001C001C001;
        guest_regs.rbx = io_count;
        guest_regs.rcx = cr3_count;
      } else if (guest_regs.rax == 0x99999999) {
        vmm_panic(ns.state, VMM_PANIC_RDMSR, 1234, &guest_regs);
      /*} else if (guest_regs.rax == 0x0) {
        __asm__ __volatile__("cpuid"
            : "=a" (guest_regs.rax), "=b" (guest_regs.rbx), "=c" (guest_regs.rcx), "=d" (guest_regs.rdx)
            :  "a" (guest_regs.rax),  "b" (guest_regs.rbx),  "c" (guest_regs.rcx),  "d" (guest_regs.rdx));
        char *gilles = "30000%MAKINA";
        guest_regs.rbx = *((uint32_t *)gilles);
        guest_regs.rdx = *((uint32_t *)gilles + 1);
        guest_regs.rcx = *((uint32_t *)gilles + 2);*/
      } else if (guest_regs.rax == 0x5) {
        // On intel platform, mwait is used instead of halt for cpu idle
        // and mwait instruction is able to change processor c-state.
        // However VMX-preemption timer doesn't work when cpu is in c-state > 2
        // so we need to disable support for theses c-states.
        // One more thing, mwait c-states don't match real (ACPI) c-states.
        // For "nehalem" cpu and onwards ("haswell" included) the mappings are :
        //    mwait (c-state).(sub-c-state)   ->    acpi c-state
        //          C0.0                      ->    C0
        //          C1.0                      ->    C1
        //          C1.1                      ->    C1E
        //          C2.0                      ->    C3
        //                    ...
        // references :
        //  - doc INTEL vol 3B chap 14.6 (mwait extensions for power management)
        //  - linux kernel 3.12 sources : drivers/idle/intel_idle.c (mappings)
        __asm__ __volatile__("cpuid"
            : "=a" (guest_regs.rax), "=b" (guest_regs.rbx), "=c" (guest_regs.rcx), "=d" (guest_regs.rdx)
            :  "a" (guest_regs.rax),  "b" (guest_regs.rbx),  "c" (guest_regs.rcx),  "d" (guest_regs.rdx));
        guest_regs.rdx &= 0xff;
      } else {
        __asm__ __volatile__("cpuid"
            : "=a" (guest_regs.rax), "=b" (guest_regs.rbx), "=c" (guest_regs.rcx), "=d" (guest_regs.rdx)
            :  "a" (guest_regs.rax),  "b" (guest_regs.rbx),  "c" (guest_regs.rcx),  "d" (guest_regs.rdx));
      }
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
      } else if (guest_regs.rcx > 0xc0001fff || (guest_regs.rcx > 0x1fff && guest_regs.rcx < 0xc0000000)) {
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
      break;
    }
    case EXIT_REASON_WRMSR: {
      if (guest_regs.rcx == MSR_ADDRESS_IA32_EFER) {
        cpu_vmwrite(GUEST_IA32_EFER, guest_regs.rax & 0xffffffff);
        cpu_vmwrite(GUEST_IA32_EFER_HIGH, guest_regs.rdx & 0xffffffff);
      } else if (is_MTRR(guest_regs.rcx)) {
          __asm__ __volatile__("wrmsr"
            : : "a" (guest_regs.rax), "b" (guest_regs.rbx), "c" (guest_regs.rcx), "d" (guest_regs.rdx));
          // Recompute the cache ranges
          uint8_t need_recompute_ept = mtrr_update_ranges();
          // Recompute ept tables
          if (need_recompute_ept) {
            ept_cache();
          }
      } else {
        vmm_panic(ns.state, VMM_PANIC_WRMSR, 0, &guest_regs);
      }
      break;
    }
    case EXIT_REASON_CR_ACCESS: {
      uint8_t cr_num      = (exit_qualification >> 0) & 0xf;
      uint8_t access_type = (exit_qualification >> 4) & 0x3;
      uint8_t reg_num     = (exit_qualification >> 8) & 0xf;

      if (access_type != 0) {
        INFO("Unsupported : access type != mov to CR\n");
        vmm_panic(ns.state, VMM_PANIC_CR_ACCESS, 0, &guest_regs);
      }

      uint64_t value = ((uint64_t*)&guest_regs)[reg_num];

      if (cr_num == 0) {
        cpu_vmwrite(CR0_READ_SHADOW, value);
        return;
      } else if (cr_num == 4) {
        cpu_vmwrite(CR4_READ_SHADOW, value);
        return;
      } else if (cr_num == 3) {
        cr3_count++;
#ifdef _DEBUG_SERVER
        // if (debug_server) {
        //   désactivation de l'expérience
        //   debug_server_log_cr3_add(&guest_regs, cpu_vmread(GUEST_CR3));
        // }
#endif
        cpu_vmwrite(GUEST_CR3, value);
        // We need to invalidate TLBs, see doc INTEL vol 3C chap 28.3.3.3
        uint8_t invvpid_desc[16] = { cpu_vmread(VIRTUAL_PROCESSOR_ID) };
        __asm__ __volatile__("invvpid %1, %0" : : "r"((uint64_t)3), "m"(*invvpid_desc));
      } else {
        vmm_panic(ns.state, VMM_PANIC_CR_ACCESS, 0, &guest_regs);
      }
      break;
    }
    //
    // Debug
    //
    case EXIT_REASON_EPT_VIOLATION: 
    case EXIT_REASON_EPT_MISCONFIG: {
      uint64_t guest_linear_addr = cpu_vmread(GUEST_LINEAR_ADDRESS);
      INFO("EPT violation, Qualification : %X, addr : %X\n", exit_qualification, guest_linear_addr);
      break;
    }
    case EXIT_REASON_VMCALL:
      printk("rax = %016X\n",guest_regs.rax);
      break;
    case EXIT_REASON_TRIPLE_FAULT:
      vmm_panic(ns.state, 0,0,&guest_regs);
      break;
    default: {
#ifdef _DEBUG_SERVER
      if (debug_server) {
        debug_server_vmexit(ns.nested_level, exit_reason, &guest_regs);
      }
#endif
    }
  }

  increment_rip(cpu_mode, &guest_regs);
}

static inline int get_cpu_mode(void) {
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

static inline void* get_instr_param_ptr(struct registers *guest_regs) {
  uint64_t addr_ptr = 0;
  uint8_t i;
  uint32_t vmx_instr_info = cpu_vmread(VMX_INSTRUCTION_INFO);
  uint8_t scaling = vmx_instr_info & 0x03;
  uint8_t index_reg_disabled = (vmx_instr_info >> 22) & 0x01;
  uint8_t base_reg_disabled = (vmx_instr_info >> 27) & 0x01;
  uint8_t index_reg = (vmx_instr_info >> 18) & 0x0F;
  uint8_t base_reg = (vmx_instr_info >> 23) & 0x0F;
  int64_t instruction_displacement_field = cpu_vmread(EXIT_QUALIFICATION);

  if (!index_reg_disabled) {
    addr_ptr = ((uint64_t*)guest_regs)[index_reg];
    for (i = 0; i < scaling; i++) {
      addr_ptr *= 2;
    }
  }
  if (!base_reg_disabled) {
    addr_ptr += ((uint64_t*)guest_regs)[base_reg];
  }
  addr_ptr += instruction_displacement_field;

  return (void*)addr_ptr;
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
      vmm_panic(ns.state, VMM_PANIC_UNKNOWN_CPU_MODE, 0, guest_regs);
  }
}

static void vmm_panic(uint8_t core, uint64_t code, uint64_t extra, struct
    registers *guest_regs) {
#ifdef _DEBUG_SERVER
  if (debug_server) {
    debug_server_panic(core, code, extra, guest_regs);
  }
#endif
}
