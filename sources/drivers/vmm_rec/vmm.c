#include "vmm.h"
#include "vmx.h"
#include "stdio.h"
#include "cpu.h"
#include "msr.h"
#include "string.h"
#include <efi.h>
#include <efilib.h>
#include "efiw.h"

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
#include "paging.h"
#include "apic.h"
#include "hook.h"
#include "msr_bitmap.h"
#include "io_bitmap.h"
#include "error.h"

// XXX
#include "dmar.h"

/**
 * TSC DEALINE virtualization
 */

static uint64_t msr_tsc_deadline_adjusted = 0;
static uint64_t msr_tsc_deadline_original = 0;

/**
 * VMs pool
 */
struct vm *vm_pool;
uint8_t vm_allocated[VM_NB];

/**
 * Current VM running
 */
struct vm *vm;

/**
 * Root VM
 */
struct vm *rvm;

void vm_set(struct vm *v) {
  vm = v;
  vmcs = v->vmcs;
}

void vm_set_root(struct vm *v) {
  rvm = v;
}

void vm_get(struct vm **v) {
  *v = vm;
}

void vm_print(struct vm *v) {
  INFO("VM : Index(0x%x), child(@0x%x)\n", v->index, v->child);
}

uint8_t *vmm_stack;

static uint64_t io_count = 0;
static uint64_t cr3_count = 0;

static uint64_t tsc_vmexit = 0;
int64_t tsc_l1_offset = 0;

int get_paging_mode(void) {
  VMR(gs.cr0);
  VMR(gs.cr4);
  VMR(gs.ia32_efer);
  if (!vmcs->gs.cr0.pg) {
    return PAGING_DISABLED;
  } else if (!vmcs->gs.cr4.pae && !vmcs->gs.ia32_efer.lma) {
    return PAGING_P32BIT;
  } else if (!vmcs->gs.ia32_efer.lma) {
    return PAGING_PAE;
  } else if (vmcs->gs.ia32_efer.lma) {
    return PAGING_IA32E;
  } else {
    ERROR("Error getting paging mode\n");
  }
  return 0;
}

int get_cpu_mode(void) {
  VMR(gs.cr0);
  VMR(gs.ia32_efer);
  VMR(gs.rflags);
  if (vmcs->gs.rflags.vm == 1) {
    return MODE_VIRTUAL_8086;
  } else if (!vmcs->gs.cr0.pe) {
    return MODE_REAL;
  } else if (!vmcs->gs.ia32_efer.lma) {
    return MODE_PROTECTED;
  } else {
    return MODE_LONG;
  }
}

static uint8_t mtf = 0;

uint8_t ismtf(void) {
  return mtf;
}

static inline void* get_instr_param_ptr(struct registers *guest_regs);
static inline void increment_rip(uint8_t cpu_mode, struct registers *guest_regs);
static inline uint8_t is_MTRR(uint64_t msr_addr);
void vmm_vms_init(void);

void vmm_init(void) {
  vmm_stack = efi_allocate_pages(VMM_STACK_SIZE >> 12);

  INFO("Setup state : rip(0x%016x), rsp(0x%016x), rbp(0x%016x),\n"
      "    protected_begin(0x%016X), protected_end(0x%016X), "
      "vmm stack(@0x%016X)\n",
      setup_state->vm_RIP, setup_state->vm_RSP, setup_state->vm_RBP,
      setup_state->protected_begin, setup_state->protected_end,
      (uint64_t)&vmm_stack[VMM_STACK_SIZE]);
  vmm_vms_init();
}

/**
 * Return the index of the allocated VM
 */
void vm_alloc(struct vm **v) {
  uint32_t i;
  for (i = 0; i < VM_NB; i++) {
    if (vm_allocated[i] == 0) {
      vm_allocated[i] = 1;
      *v = &vm_pool[i];
      // Reset the VM
      memset(*v, 0, sizeof(struct vm));
      (*v)->index = i;
      (*v)->vmcs_region = &vmcs_region_pool[i][0];
      INFO("REGION 0x%016X\n", (*v)->vmcs_region);
      (*v)->vmcs = &vmcs_cache_pool[i];
      return;
    }
  }
  ERROR("Failed to allocate a VM, pool is fully used\n");
}

/**
 * Free a VM
 */
void vm_free(struct vm *v) {
  uint32_t i;
  if (v == 0) {
    ERROR("Bad VM pointer\n");
  }
  for (i = 0; i < VM_NB; i++) {
    if (&vm_pool[i] == v) {
      if (vm_allocated[i] == 1) {
        ERROR("VM wasn't allocated\n");
      }
      vm_allocated[i] = 0;
      return;
    }
  }
  ERROR("Failed to free a VM : not found\n");
}

// Don't dealocate the VM0
void vm_free_all(void) {
  uint32_t i;
  for (i = 1; i < VM_NB; i++) {
    vm_allocated[i] = 0;
  }
}

void vm_iterate(int (*cb) (struct vm *)) {
  uint32_t i;
  for (i = 0; i < VM_NB; i++) {
    if (vm_allocated[i] == 1) {
      if (!cb(&vm_pool[i])) {
        break;
      }
    }
  }
}

/**
 * Handle child VMs
 */
void vm_child_add(struct vm *pv, struct vm *cv) {
  uint32_t i;
  for (i = 0; i < VM_NB; i++) {
    if (pv->childs[i] == NULL) {
      pv->childs[i] = cv; 
      return;
    }
  }
  ERROR("Max child VM number reached\n");
}

void vm_child_del(struct vm *pv, struct vm *cv) {
  uint32_t i;
  for (i = 0; i < VM_NB; i++) {
    if (pv->childs[i] == cv) {
      pv->childs[i] = NULL; 
      return;
    }
  }
  ERROR("Child VM not found\n");
}

/**
 * Find a child curresponding to the current shadow_ptr
 * and set it as the current child
 */
void vm_child_shadow_set(struct vm *v) {
  uint32_t i;
  for (i = 0; i < VM_NB; i++) {
    if (v->childs[i]->shadow_vmcs == v->shadow_ptr) {
      v->child = v->childs[i];
      return;
    }
  }
  ERROR("Current shadow VMCS is not found nor running\n");
}

/**
 * Find a child curresponding to the current shadow_ptr
 */
void vm_child_shadow_get(struct vm *pv, struct vm **cv) {
  // DBG("shadow_ptr(@0x%016X), index(0x%x)\n", pv->shadow_ptr, pv->index); 
  uint32_t i;
  for (i = 0; i < VM_NB; i++) {
    if (pv->childs[i] != NULL) {
      // DBG("child : index(0x%x), shadow_vmcs(@0x%016X), vmcs_region(0x%016X)\n",
          // pv->childs[i]->index, pv->childs[i]->shadow_vmcs,
          // pv->childs[i]->vmcs_region); 
      if (pv->childs[i]->shadow_vmcs == pv->shadow_ptr) {
        *cv = pv->childs[i];
        return;
      }
    }
  }
  *cv = NULL;
}

void vmm_vms_init(void) {
  vm_pool = efi_allocate_pool(sizeof(struct vm) * VM_NB);
  WARN("This VMM can handle up to 0x%08x VMs\n", VM_NB);
  // Initialize VMs pool
  memset(&vm_pool[0], 0, VM_NB * sizeof(struct vm));
  // Initialize allocation table
  memset(&vm_allocated[0], 0, VM_NB);
}

/**
 * Compute if we haev to redirect the VMExit to the l1 host
 */
int vmm_host_redirect(uint32_t exit_reason, uint32_t exit_qualification, struct
    registers *guest_regs) {
  switch(exit_reason) {
    case EXIT_REASON_WRMSR:
      return msr_bitmap_write_host_redirect(guest_regs->rcx);
    case EXIT_REASON_RDMSR:
      return msr_bitmap_read_host_redirect(guest_regs->rcx);
    case EXIT_REASON_IO_INSTRUCTION:
      return io_bitmap_host_redirect((exit_qualification >> 16) & 0xffff);
  }
  return 1;
}

void vmm_handle_vm_exit(struct registers guest_regs) {
  uint8_t hook_override = 0;

  // XXX OLOL OwNED!
  // vmcs_encoding_init_all();

  tsc_vmexit = cpu_rdtsc();

  VMR2(gs.rsp, guest_regs.rsp);
  VMR2(gs.rip, guest_regs.rip);

  VMR(info.reason);
  uint32_t exit_reason = vmcs->info.reason.raw;
  VMR(info.qualification);
  uint64_t exit_qualification = vmcs->info.qualification.raw;
  uint8_t cpu_mode = get_cpu_mode();
  uint8_t** instr_param_ptr;

  // check VMX abort
  uint32_t vmx_abort = ((uint32_t*)vm->vmcs_region)[1];
  if (vmx_abort) {
    ERROR("VMX abort detected : %d\n", vmx_abort);
  }

  // Check VM Entry failure
  if (vmcs->info.reason.vm_entry_failure) {
    vmcs_force_update();
    vmcs_dump(vmcs);
    // TODO decode the exit reason which gives the reason of the failure
    ERROR("VM entry failure\n");
  }

  // Boot hook
  if (hook_boot[exit_reason] != 0) {
    hook_override = (hook_boot[exit_reason])(&guest_regs);
  }

#ifdef _DEBUG_SERVER
  if (debug_server) {
    debug_server_vmexit(vm->index, exit_reason, &guest_regs);
  }
#endif

  // If we override we return to the VM
  if (hook_override) {
    if (hook_override == HOOK_OVERRIDE_STAY) {
      INFO("Boot override, no rip increment !\n");
      return;
    }
    increment_rip(cpu_mode, &guest_regs);
    return;
  }

  //
  // VMX Specific VMexits that we override
  //
  if (exit_reason == EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED) {
    vmcs_set_vmx_preemption_timer_value(vmcs,
        VMCS_DEFAULT_PREEMPTION_TIMER_MICROSEC);
    return;
// XXX Optimisation rappel de la NIM et EXternal interrupt
//  } else if (exit_reason == EXIT_REASON_EXTERNAL_INTERRUPT) {
//    VMR(info.intr_info);
//    union vm_exit_interrupt_info iif = {.raw = vmcs->info.intr_info.raw};
//    VMR(info.intr_error_code);
//    uint32_t error_code = vmcs->info.intr_error_code.raw;
//    INFO("External interrupt from 0x%x : \n  info(0x%x)\n  error_code(0x%x)\n",
//        vm->index, iif.raw, error_code);
//    if (iif.vector == 0xef) { // Local APIC timer
//      if (apic_get_mode() == APIC_MODE_X2APIC) {
//        union apic_timer_register timer_ctrl = 
//            {.raw = msr_read(MSR_ADDRESS_IA32_X2APIC_LVT_TIMER)};
//        uint64_t timer_init =
//          msr_read(MSR_ADDRESS_IA32_X2APIC_LVT_INITIAL_COUNT);
//        uint64_t timer_curr =
//          msr_read(MSR_ADDRESS_IA32_X2APIC_LVT_CURRENT_COUNT);
//        INFO("Timer:\n  ctrl(0x%016X)\n  init(0x%016X)\n  curr(0x%016X)\n",
//            timer_ctrl.raw, timer_init, timer_curr);
//        if (timer_ctrl.mode == APIC_TIMER_MODE_ONE_TSC_DEADLINE) {
//          uint64_t reload = msr_read(MSR_ADDRESS_IA32_TSC_DEADLINE);
//          uint64_t tsc = cpu_rdtsc();
//          INFO("TSC deadline mode :\n  goal(0x%016X)\n  tsc(0x%016X)\n",
//              reload, tsc);
//        }
//      }
//    }
    // nested_interrupt_set(iif.vector, iif.type, error_code);
    // nested_interrupt_inject();
//  } else if (exit_reason == EXIT_REASON_EXCEPTION_OR_NMI) {
//    INFO("NMI interrupt from 0x%x!\n", vm->index);
//    VMR(info.intr_info);
//    union vm_exit_interrupt_info iif = {.raw = vmcs->info.intr_info.raw};
//    VMR(info.intr_error_code);
//    uint32_t error_code = vmcs->info.intr_error_code.raw;
//    nested_interrupt_set(iif.vector, iif.type, error_code);
//    return;
  } else if (exit_reason == EXIT_REASON_MONITOR_TRAP_FLAG) {
    return;
  }

  // EPT violation hadling
  if (exit_reason == EXIT_REASON_EPT_VIOLATION) {
    VMR(info.guest_physical_address);
    uint64_t guest_physical_addr = vmcs->info.guest_physical_address.raw;
    VMR(ctrls.ex.ept_pointer);
    uint64_t eptp = vmcs->ctrls.ex.ept_pointer.raw;
    uint64_t *e;
    uint64_t a;
    uint8_t s;
    if(ept_walk(eptp, guest_physical_addr, &e, &a, &s)) {
      if (paging_error != PAGING_WALK_NOT_PRESENT) {
        INFO("EPT VIOLATION : ERROR walking address 0x%016X\n",
            guest_physical_addr);
      }
    }
    INFO("EPT VIOLATION from 0x%x: Guest physical 0x%016X\n",
        ept_get_ctx(), guest_physical_addr);
    // XXX
    INFO("protected_end 0x%016X\n", setup_state->protected_end);
    // Own memory space check
    if (guest_physical_addr >= setup_state->protected_begin &&
        guest_physical_addr < setup_state->protected_end) {
      INFO("EPT VIOLATION FOR ME!\n");
      // increment_rip(cpu_mode, &guest_regs);
      ERROR("Unsupported EPT_VIOLATION\n");
      return;
    }
  }

  if (rvm->state == NESTED_GUEST_RUNNING &&
      vmm_host_redirect(exit_reason, exit_qualification, &guest_regs)) {
    // Forward to L1 host
    nested_load_host();
    return;
  }

  // Pre hook
  if (hook_pre[exit_reason] != 0) {
    hook_override = (hook_pre[exit_reason])(&guest_regs);
  }

  // If we override we return to the VM
  if (hook_override) {
    increment_rip(cpu_mode, &guest_regs);
    return;
  }


  switch (exit_reason) {
    //
    // VMX Operations
    //
    case EXIT_REASON_RDPMC:
    case EXIT_REASON_RDTSC:
    case EXIT_REASON_RSM:
    case EXIT_REASON_MOV_DR:
    case EXIT_REASON_INVALID_GUEST_STATE:
    case EXIT_REASON_MSR_LOADING_FAILED:
    case EXIT_REASON_MWAIT:
    case EXIT_REASON_MONITOR_TRAP_FLAG:
    case EXIT_REASON_MONITOR:
    case EXIT_REASON_PAUSE:
    case EXIT_REASON_MCE_DURING_VM_ENTRY:
    case EXIT_REASON_TPR_BELOW_THRESHOLD:
    case EXIT_REASON_APIC_ACCESS:
    case EXIT_REASON_VIRTUALIZED_EOI:
    case EXIT_REASON_ACCESS_GDTR_OR_IDTR:
    case EXIT_REASON_ACCESS_LDTR_OR_TR:
    case EXIT_REASON_EPT_VIOLATION:
    case EXIT_REASON_INVEPT:
    case EXIT_REASON_RDTSCP:
    case EXIT_REASON_WBINVD:
    case EXIT_REASON_APIC_WRITE:
    case EXIT_REASON_RDRAND:
    case EXIT_REASON_INVPCID:
    case EXIT_REASON_VMFUNC:
    case EXIT_REASON_RDSEED:
    case EXIT_REASON_XSAVES:
    case EXIT_REASON_XRSTORS:
    case EXIT_REASON_INVLPG:
    case EXIT_REASON_INVD:
    case EXIT_REASON_HLT:
    case EXIT_REASON_GETSEC:
    case EXIT_REASON_TASK_SWITCH:
    case EXIT_REASON_NMI_WINDOW:
    case EXIT_REASON_INTR_WINDOW:
    case EXIT_REASON_OTHER_SMI:
    case EXIT_REASON_IO_SMI:
    case EXIT_REASON_SIPI:
    case EXIT_REASON_INIT_SIGNAL:
    case EXIT_REASON_EXTERNAL_INTERRUPT:
    case EXIT_REASON_EXCEPTION_OR_NMI:
      break;
    case EXIT_REASON_VMXOFF:
      nested_vmxoff(&guest_regs);
      break;
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
    case EXIT_REASON_VMPTRST:
      instr_param_ptr = get_instr_param_ptr(&guest_regs);
      nested_vmptrst(instr_param_ptr, &guest_regs);
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
      VMR(info.vmx_instruction_info);
      uint64_t type = ((uint64_t*)&guest_regs)
        [(vmcs->info.vmx_instruction_info.raw >> 28) & 0xF];
      // We need to increment the VPID
      ((uint16_t*)desc)[0]++;
      __asm__ __volatile__("invvpid %0, %1" : : "m"(*desc), "r"(type) );
      break;
    }
    case EXIT_REASON_VMREAD:
    case EXIT_REASON_VMWRITE: {
      VMR(info.vmx_instruction_info);
      uint8_t operand_in_register = (vmcs->info.vmx_instruction_info.raw >> 10) & 0x1;
      uint64_t field = ((uint64_t*)&guest_regs)
        [(vmcs->info.vmx_instruction_info.raw >> 28) & 0xF];
      uint64_t* value_ptr = NULL;

      if (operand_in_register) {
        value_ptr = &((uint64_t*)&guest_regs)
          [(vmcs->info.vmx_instruction_info.raw >> 3) & 0xF];
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
        ERROR("Guest using xsetbv outsite long mode!\n");
      }
      break;
    }
    case EXIT_REASON_IO_INSTRUCTION: {
      io_count++;
      // Checking the privileges
      VMR(gs.cs_ar_bytes);
      uint8_t cpl = (vmcs->gs.cs_ar_bytes.raw >> 5) & 3;
      VMR(gs.rflags);
      uint8_t iopl = (vmcs->gs.rflags.raw >> 12) & 3;
      if (cpu_mode == MODE_REAL || cpl <= iopl) {
        uint8_t direction = exit_qualification & 8;
        uint8_t size = exit_qualification & 7;
        uint8_t string = exit_qualification & (1<<4);
        uint8_t rep = exit_qualification & (1<<5);
        uint16_t port = (exit_qualification >> 16) & 0xffff;

        // REP prefixed || String I/O Unsupported
        if (rep || string) {
          ERROR("I/O instruction with rep prefix unsupported\n");
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
              ERROR("I/O size decoding error\n");
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
              ERROR("I/O size decoding error\n");
            }
          } else {
            INFO("NIC I/O config space block\n");
            if (size == 0) {
              guest_regs.rax = guest_regs.rax | 0x000000ff;
            } else if (size == 1) {
              guest_regs.rax = guest_regs.rax | 0x0000ffff;
            } else if (size == 3) {
              guest_regs.rax = guest_regs.rax | 0xffffffff;
            } else {
              ERROR("I/O size decoding error\n");
            }
          }
        }
      // Unsufficient privileges
      } else {
        ERROR("Unsufficient privileges\n");
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
      // Adjust it with the current TSC offset
      if (guest_regs.rcx == MSR_ADDRESS_IA32_TSC_DEADLINE) {
        guest_regs.rax = (guest_regs.rax & (0xffffffff00000000)) | ((msr_tsc_deadline_original >> 0) & 0xffffffff);
        guest_regs.rdx = (guest_regs.rdx & (0xffffffff00000000)) | ((msr_tsc_deadline_original >> 32) & 0xffffffff);
      } else if (guest_regs.rcx == MSR_ADDRESS_IA32_EFER) {
        if (cpu_mode == MODE_LONG) {
          guest_regs.rdx = 0;
          guest_regs.rax = 0;
        }
        VMR(gs.ia32_efer);
        guest_regs.rax = (guest_regs.rax & (0xffffffff00000000)) | (vmcs->gs.ia32_efer.raw & 0xffffffff);
        guest_regs.rdx = (guest_regs.rdx & (0xffffffff00000000)) | ((vmcs->gs.ia32_efer.raw >> 32) & 0xffffffff);
      } else if (guest_regs.rcx > 0xc0001fff || (guest_regs.rcx > 0x1fff && guest_regs.rcx < 0xc0000000)) {
        // Tells the vm that the msr doesn't exist
        uint32_t it_info_field =    (0x1 << 11)     // push error code
                                  | (0x3 << 8)      // hardware exception
                                  | 0xd ;           // GP fault
        VMW(info.intr_info, it_info_field);
      } else {
        __asm__ __volatile__("rdmsr"
          : "=a" (guest_regs.rax), "=b" (guest_regs.rbx), "=c" (guest_regs.rcx), "=d" (guest_regs.rdx)
          :  "a" (guest_regs.rax),  "b" (guest_regs.rbx),  "c" (guest_regs.rcx),  "d" (guest_regs.rdx));
      }
#ifdef _NO_GUEST_EPT
      if (guest_regs.rcx == MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS2) {
        guest_regs.rax &= ~((uint32_t)1 << 1) & 0xffffffff;
        guest_regs.rdx &= ~((uint32_t)1 << 1) & 0xffffffff;
      }
#endif
      break;
    }
    case EXIT_REASON_WRMSR: {
      // Adjust it with the current TSC offset
      if (guest_regs.rcx == MSR_ADDRESS_IA32_TSC_DEADLINE) {
        msr_tsc_deadline_original = guest_regs.rax | (guest_regs.rdx << 32);
      } else if (guest_regs.rcx == MSR_ADDRESS_IA32_EFER) {
        VMW(gs.ia32_efer, ((guest_regs.rdx << 32) & 0xffffffff00000000) |
            (guest_regs.rax & 0xffffffff));
      } else if (is_MTRR(guest_regs.rcx)) {
          __asm__ __volatile__("wrmsr"
            : : "a" (guest_regs.rax), "b" (guest_regs.rbx), "c" (guest_regs.rcx), "d" (guest_regs.rdx));
          // Recompute the cache ranges
          uint8_t need_recompute_ept = mtrr_update_ranges();
          // Recompute ept tables
          if (need_recompute_ept) {
            ept_cache();
          }
      } else if (guest_regs.rcx == MSR_ADDRESS_IA32_APIC_BASE) {
          __asm__ __volatile__("wrmsr"
            : : "a" (guest_regs.rax), "b" (guest_regs.rbx), 
            "c" (guest_regs.rcx), "d" (guest_regs.rdx));
          INFO("Writing in apic base msr!\n"); 
          apic_setup();
      } else {
        msr_bitmap_dump((struct msr_bitmap*)vmcs->ctrls.ex.msr_bitmap.raw);
        ERROR("MSR write panic rcx(0x%08x) <= rdx(0x%08x)\n", guest_regs.rcx,
            guest_regs.rdx);
      }
      break;
    }
    case EXIT_REASON_CR_ACCESS: {
      uint8_t cr_num      = (exit_qualification >> 0) & 0xf;
      uint8_t access_type = (exit_qualification >> 4) & 0x3;
      uint8_t reg_num     = (exit_qualification >> 8) & 0xf;

      if (access_type != 0) {
        ERROR("Unsupported : access type != mov to CR\n");
      }

      uint64_t value = ((uint64_t*)&guest_regs)[reg_num];

      if (cr_num == 0) {
        VMW(ctrls.ex.cr0_read_shadow, value);
        return;
      } else if (cr_num == 4) {
        VMW(ctrls.ex.cr4_read_shadow, value);
        return;
      } else if (cr_num == 3) {
        cr3_count++;
#ifdef _DEBUG_SERVER
        // experience CR3
        if (debug_server) {
          VMR(gs.cr3);
          debug_server_log_cr3_add(&guest_regs, vmcs->gs.cr3.raw);
        }
#endif
        VMW(gs.cr3, value);
        // We need to invalidate TLBs, see doc INTEL vol 3C chap 28.3.3.3
        VMR(ctrls.ex.virtual_processor_id);
        uint8_t invvpid_desc[16] = {vmcs->ctrls.ex.virtual_processor_id.raw};
        __asm__ __volatile__("invvpid %1, %0" : : "r"((uint64_t)3), "m"(*invvpid_desc));
      } else {
        ERROR("Invalid control register number for cr access\n");
      }
      break;
    }
    //
    // Debug
    //
    case EXIT_REASON_EPT_MISCONFIG: {
      INFO("ept misconfiguration\n");
      break;
    }
    case EXIT_REASON_VMCALL:
      printk("rax = %016X\n",guest_regs.rax);
      break;
    case EXIT_REASON_TRIPLE_FAULT:
      break;
    default: {
      ERROR("Unsupported VMEXIT : 0x%08x\n", exit_reason);
    }
  }

  increment_rip(cpu_mode, &guest_regs);

  // Post hook
  if (hook_post[exit_reason] != 0) {
    (hook_post[exit_reason])(&guest_regs);
  }
}

/**
 * Called right after vmm_handle_vm_exit()
 */
void vmm_adjust_tsc(void) {
  int64_t tsc_delta = tsc_vmexit - cpu_rdtsc(); // < 0 value
  int cb(struct vm *v) {
    v->tsc_offset += tsc_delta;
    return 1;
  }
  // Apply new TSC offset to every active VM
  vm_iterate(cb);
  // Finally write the current tsc offset
  if (vm == rvm) {
    VMW(ctrls.ex.tsc_offset, vm->tsc_offset);
  } else {
    // Write in addition the TSC offset wanted by l1
    VMW(ctrls.ex.tsc_offset, vm->tsc_offset + tsc_l1_offset);
  }
  // Adjust TSC DEADLINE
  if (msr_tsc_deadline_original > 0) {
    msr_tsc_deadline_adjusted = msr_tsc_deadline_original - rvm->tsc_offset;
    msr_write(MSR_ADDRESS_IA32_TSC_DEADLINE, msr_tsc_deadline_adjusted);
//    INFO("Write to TSC DEADLINE DUDES original 0x%016X, adjusted 0x%016X, tsc is 0x%016X, offset is 0x%016X!!!\n",
//        msr_tsc_deadline_original, msr_tsc_deadline_adjusted, cpu_rdtsc(),
//        rvm->tsc_offset);
    if (msr_tsc_deadline_adjusted < cpu_rdtsc()) {
      // WARN("DEADLINE HAS PASSED YOLOSSE\n");
    }
  }
  // INFO("TSC offset 0x%016X\n", vm->tsc_offset);
}

/**
 * Called right after vmm_handle_vm_exit()
 */
void vmm_adjust_paging(void) {
  // Paging adjustments
  int paging_mode = get_paging_mode();
  if (paging_mode == PAGING_PAE) {
    uint64_t *cr3;
    cr3 = (uint64_t *)(uint64_t)(vmcs->gs.cr3.page_directory_base << 12);
    // TODO XXX check the cr3 we cannot trust this address
    VMW(gs.pdpte0, cr3[0]);
    VMW(gs.pdpte1, cr3[1]);
    VMW(gs.pdpte2, cr3[2]);
    VMW(gs.pdpte3, cr3[3]);
  } else {
    VMW(gs.pdpte0, 0);
    VMW(gs.pdpte1, 0);
    VMW(gs.pdpte2, 0);
    VMW(gs.pdpte3, 0);
  }
}

/**
 * Called right after vmm_handle_vm_exit()
 */
void vmm_vmcs_flush(void) {
  // Flush the current vmcs
  vmcs_commit();
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
  VMR(info.vmx_instruction_info);
  uint32_t vmx_instr_info = vmcs->info.vmx_instruction_info.raw;
  uint8_t scaling = vmx_instr_info & 0x03;
  uint8_t index_reg_disabled = (vmx_instr_info >> 22) & 0x01;
  uint8_t base_reg_disabled = (vmx_instr_info >> 27) & 0x01;
  uint8_t index_reg = (vmx_instr_info >> 18) & 0x0F;
  uint8_t base_reg = (vmx_instr_info >> 23) & 0x0F;
  VMR(info.qualification);
  int64_t instruction_displacement_field = vmcs->info.qualification.raw;

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

  // XXX page a l'arrache
  uint64_t *e;
  uint64_t a;
  uint8_t s;
  VMR(gs.cr3);
  if (paging_walk(vmcs->gs.cr3.raw, addr_ptr, &e, &a, &s)) {
    ERROR("Page walk error\n");
  }

  return (void*)a;
}

static inline void increment_rip(uint8_t cpu_mode, struct registers *guest_regs) {
  VMR(info.instruction_len);
  uint32_t exit_instruction_length = vmcs->info.instruction_len.raw;
  // LEVEL(2, "MODE %d, ILength %d\n", cpu_mode, exit_instruction_length);

  switch (cpu_mode) {
    case MODE_REAL: {
      uint16_t tmp = (uint16_t) guest_regs->rip;
      tmp = tmp + (uint16_t) exit_instruction_length;
      VMW(gs.rip, (guest_regs->rip & 0xffffffffffff0000) | tmp);
      break;
    }
    // XXX red alert !!!!
    case MODE_VIRTUAL_8086:
      WARN("Incrementing virtual 8086 rip ... unsupported now, defaulting to protected mode\n");
    case MODE_PROTECTED: {
      uint32_t tmp = (uint32_t) guest_regs->rip;
      tmp = tmp + (uint32_t) exit_instruction_length;
      VMW(gs.rip, (guest_regs->rip & 0xffffffff00000000) | tmp);
      break;
    }
    case MODE_LONG: {
      uint64_t tmp = (uint64_t) guest_regs->rip;
      tmp = tmp + (uint64_t) exit_instruction_length;
      // LEVEL(2, "TMP 0x%016X\n", tmp);
      VMW(gs.rip, (guest_regs->rip & 0x0000000000000000) | tmp);
      // VMR(gs.rip);
      // LEVEL(2, "Post VMCS Guest rip%016X\n", vmcs->gs.rip);
      break;
    }
    default :
      ERROR("Unknown CPU mode\n");
  }
}

void vmm_adjust_execution_controls(void) {
#ifdef _DEBUG_SERVER
  // handle Monitor trap flag
  vmm_adjust_mtf();
#endif
}

void vmm_adjust_vm_entry_controls(void) {
  uint8_t cpu_mode = get_cpu_mode();
  VMR(ctrls.entry.controls);
  uint64_t vm_entry_controls = vmcs->ctrls.entry.controls.raw;
  if (cpu_mode == MODE_LONG) {
    // Guest is in IA32e mode
    // INFO("Setting IA32E_MODE_GUEST\n");
    VMW(ctrls.entry.controls, vm_entry_controls |
        (uint64_t)IA32E_MODE_GUEST);
  } else {
    // Guest is not in IA32e mode
    VMW(ctrls.entry.controls, vm_entry_controls & ~(uint64_t)IA32E_MODE_GUEST);
  }
}

void vmm_adjust_mtf(void) {
  // monitor trap flag handling
  VMR(ctrls.ex.cpu_based_vm_exec_control);
  if (ismtf()) {
    VMW(ctrls.ex.cpu_based_vm_exec_control,
        vmcs->ctrls.ex.cpu_based_vm_exec_control.raw | MONITOR_TRAP_FLAG);
  } else {
    VMW(ctrls.ex.cpu_based_vm_exec_control,
        vmcs->ctrls.ex.cpu_based_vm_exec_control.raw &
        ~(uint32_t)MONITOR_TRAP_FLAG);
  }
}

void vmm_mtf_set(void) {
  INFO("WE SET THE MOTHERFUCKING MTF !\n");
  mtf = 1;
}

void vmm_mtf_unset(void) {
  INFO("WE UNSET THE MOTHERFUCKING MTF !\n");
  mtf = 0;
}
