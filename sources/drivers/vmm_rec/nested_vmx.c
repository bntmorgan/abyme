#include <efi.h>
#include <efilib.h>
#include "vmm.h"
#include "nested_vmx.h"
#include "vmcs.h"
#include "vmx.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "nested_vmx.h"
#include "paging.h"
#include "ept.h"
#include "cpuid.h"
#include "msr.h"
#include "cpu.h"
#include "level.h"
#include "io_bitmap.h"
#include "msr_bitmap.h"
#ifdef _DEBUG_SERVER
#include "debug_server/debug_server.h"
#endif

// Current level
uint8_t level;

void nested_set_vm_succeed(void);

// Shadow VMCS cache
static struct vmcs svmcs = {};

#ifdef _VMCS_SHADOWING
uint8_t vmread_bitmap [4096] __attribute__((aligned(0x1000)));
uint8_t vmwrite_bitmap[4096] __attribute__((aligned(0x1000)));
#endif

#ifdef _VMCS_SHADOWING
static void shadow_bitmap_read_set_field(uint64_t field) {
  vmread_bitmap[(field & 0x7fff) >> 3] |= (1 << ((field & 0x7fff) & 0x7));
}

void nested_vmx_shadow_bitmap_init(void) {
  static uint64_t fields[] = {NESTED_READ_ONLY_DATA_FIELDS};
  memset(&vmwrite_bitmap[0], 0, 4096);
  memset(&vmread_bitmap[0], 0, 4096);
  uint32_t i;
  for (i = 0; i < sizeof(fields) / sizeof(uint64_t); i++) {
    shadow_bitmap_read_set_field(fields[i]);
  }
}
#endif

/**
 * Event injection
 */ 

static uint8_t charged = 0;
static uint32_t error_code = 0;
static union vm_entry_interrupt_info iif;

void nested_interrupt_set(uint8_t vector, uint8_t type, uint32_t error_code) {
  if (charged) {
    INFO("Multiple event injection unsupported : losing an interrupt \n");
  }
  charged = 1;
  iif.vector = vector;
  iif.type = type;
  if (error_code > 0 && (type == VM_ENTRY_INT_TYPE_SOFT_EXCEPTION || type ==
        VM_ENTRY_INT_TYPE_HW_EXCEPTION)) {
    iif.error_code = 1;
    error_code = error_code;
  }
  iif.valid = 1;
}

void nested_interrupt_inject(void) {
  if (charged) {
    charged = 0;
    cpu_vmwrite(VM_ENTRY_INTR_INFO_FIELD, iif.raw);
    if (iif.error_code) {
      cpu_vmwrite(VM_ENTRY_EXCEPTION_ERROR_CODE, error_code);
    }
    INFO("Event Injection in 0x%x!\n", level);
  }
}

/**
 * VMX Emulation
 */

#ifdef _VMCS_SHADOWING
static void set_vmcs_link_pointer(uint8_t *shadow_vmcs) {
  // Set the VMCS link pointer for vmcs0
  cpu_vmwrite(VMCS_LINK_POINTER, ((uint64_t)shadow_vmcs) & 0xffffffff);
  cpu_vmwrite(VMCS_LINK_POINTER_HIGH, (((uint64_t)shadow_vmcs) >> 32) & 0xffffffff);
  cpu_vmwrite(SECONDARY_VM_EXEC_CONTROL, cpu_vmread(SECONDARY_VM_EXEC_CONTROL) |
      VMCS_SHADOWING);
  // Exit bitmap
  cpu_vmwrite(VMREAD_BITMAP_ADDR, (uint64_t)&vmread_bitmap & 0xffffffff);
  cpu_vmwrite(VMREAD_BITMAP_ADDR_HIGH, ((uint64_t)&vmread_bitmap >> 32) & 0xffffffff);
  cpu_vmwrite(VMWRITE_BITMAP_ADDR, (uint64_t)&vmwrite_bitmap & 0xffffffff);
  cpu_vmwrite(VMWRITE_BITMAP_ADDR_HIGH, ((uint64_t)&vmwrite_bitmap >> 32) & 0xffffffff);
  // Set the type of the shadow_vmcs
  *(uint32_t *)shadow_vmcs |= (1 << 31);
}
#endif

void nested_vmxon(uint8_t *vmxon_guest) {
  if (vm->state != NESTED_DISABLED) {
    panic("#!NESTED_VMXON not disabled\n");
  }
  // Notify the success
  nested_set_vm_succeed();

  // check if vmcs addr is aligned on 4Ko and check rev identifier
  if (((uint64_t)vmxon_guest & 0xfff) != 0){
    panic("#!NESTED_VMXON addr is not 4k aligned\n");
    // (1 << 32) ignore if our VMCS is a shadow...
  } else if (((*(uint32_t*)vmxon_guest) & ~(1 << 31)) != (*(uint32_t*)vm->vmcs_region & ~(1 << 31))) {
    panic("#!NESTED_VMXON rev identifier is not valid\n");
  }

  vm->state = NESTED_HOST_RUNNING;

  DBG("This is the VMXON dudes\n");
}

void nested_vmclear(uint8_t *shadow_vmcs) {
  cpu_vmclear(shadow_vmcs);
  // set guest carry and zero flag to 0
  nested_set_vm_succeed();
  DBG("This is the VMCLEAR dudes\n");
}

void nested_vmptrld(uint8_t *shadow_vmcs, struct registers *gr) {

  // Remember the current shadow VMCS
  rvm->shadow_ptr = shadow_vmcs;

  if (shadow_vmcs == 0x0) {
    ERROR("Shadow VMCS pointer is null\n");
  }

  nested_set_vm_succeed();

#ifdef _VMCS_SHADOWING
  set_vmcs_link_pointer(shadow_vmcs);
#endif
  // DBG("This is the VMPTRLD dudes\n");
}

void nested_vmptrst(uint8_t **shadow_vmcs, struct registers *gr) {
  // Give back the shadow ptr to the guest
  *shadow_vmcs = rvm->shadow_ptr;

  nested_set_vm_succeed();
}

#ifdef _NESTED_EPT
void nested_smap_build(uint32_t index) {
  uint64_t start, end;
  uint64_t ta = 0;
  uint8_t in = 0;
  uint8_t max_phyaddr = cpuid_get_maxphyaddr();
  // XXX already done in shadow to guest
  cpu_vmptrld(rvm->shadow_ptr);
  // Private callback for chunk detection
  int cb(uint64_t *e, uint64_t a, uint8_t s) {
    if (in == 0) {
      if ((*e & 0x7) == 0) {
        start = a;
        in = 1;
      }
    } else {
      if ((*e & 0x7) != 0) {
        end = a; 
        // Add the detected memory chunk to the smap
        INFO("chunk(0x%x, 0x%016X, 0x%016X)\n", index, start, (end -
              start) >> 12);
        ept_perm(start, ((end - start) >> 12), 0x0, index);
        in = 0;
      }
    }
    switch (s) {
      case PAGING_ENTRY_PTE:
        ta = a + 0x1000;
        break;
      case PAGING_ENTRY_PDE:
        ta = a + 0x200000;
        break;
      case PAGING_ENTRY_PDPTE:
        ta = a + 0x40000000;
        break;
      default:
        ERROR("BAD page size\n");
    }
    if (ta == ((uint64_t)1 << max_phyaddr)) {
      INFO("CHECK END maxphyaddr reached\n");
      return 0;
    } else {
      return 1;
    }
  }
  // TODO XXX
  uint64_t eptp = ((cpu_vmread(EPT_POINTER_HIGH) << 32) |
    cpu_vmread(EPT_POINTER)) & ~((uint64_t)0xfff);
  INFO("EPTP 0x%016X\n", eptp);
  if (ept_iterate(eptp, &cb)) {
    ERROR("PAGING error... 0x%x\n", paging_error);
  }
}
#endif

/**
 * Applies a fully collected shadow VMCS cache to the nvm VMCS cache
 */
void nested_guest_shadow_apply(struct vmcs *svmcs, struct vm *nvm) {
  struct vmcs *bvmcs = vmcs;
  vmcs = nvm->vmcs;
  // Guest state
  VMC(gs.es_selector, nvm->vmcs, svmcs);
  VMC(gs.cs_selector, nvm->vmcs, svmcs);
  VMC(gs.ss_selector, nvm->vmcs, svmcs);
  VMC(gs.ds_selector, nvm->vmcs, svmcs);
  VMC(gs.fs_selector, nvm->vmcs, svmcs);
  VMC(gs.gs_selector, nvm->vmcs, svmcs);
  VMC(gs.ldtr_selector, nvm->vmcs, svmcs);
  VMC(gs.tr_selector, nvm->vmcs, svmcs);
  VMC(gs.ia32_debugctl, nvm->vmcs, svmcs);
  VMC(gs.ia32_pat, nvm->vmcs, svmcs);
  VMC(gs.ia32_efer, nvm->vmcs, svmcs);
  VMC(gs.ia32_perf_global_ctrl, nvm->vmcs, svmcs);
  VMC(gs.pdptr0, nvm->vmcs, svmcs);
  VMC(gs.pdptr1, nvm->vmcs, svmcs);
  VMC(gs.pdptr2, nvm->vmcs, svmcs);
  VMC(gs.pdptr3, nvm->vmcs, svmcs);
  VMC(gs.cr0, nvm->vmcs, svmcs);
  VMC(gs.cr3, nvm->vmcs, svmcs);
  VMC(gs.cr4, nvm->vmcs, svmcs);
  VMC(gs.es_base, nvm->vmcs, svmcs);
  VMC(gs.cs_base, nvm->vmcs, svmcs);
  VMC(gs.ss_base, nvm->vmcs, svmcs);
  VMC(gs.ds_base, nvm->vmcs, svmcs);
  VMC(gs.fs_base, nvm->vmcs, svmcs);
  VMC(gs.gs_base, nvm->vmcs, svmcs);
  VMC(gs.ldtr_base, nvm->vmcs, svmcs);
  VMC(gs.tr_base, nvm->vmcs, svmcs);
  VMC(gs.gdtr_base, nvm->vmcs, svmcs);
  VMC(gs.idtr_base, nvm->vmcs, svmcs);
  VMC(gs.dr7, nvm->vmcs, svmcs);
  VMC(gs.rsp, nvm->vmcs, svmcs);
  VMC(gs.rip, nvm->vmcs, svmcs);
  VMC(gs.rflags, nvm->vmcs, svmcs);
  VMC(gs.pending_dbg_exceptions, nvm->vmcs, svmcs);
  VMC(gs.sysenter_esp, nvm->vmcs, svmcs);
  VMC(gs.sysenter_eip, nvm->vmcs, svmcs);
  VMC(gs.es_limit, nvm->vmcs, svmcs);
  VMC(gs.cs_limit, nvm->vmcs, svmcs);
  VMC(gs.ss_limit, nvm->vmcs, svmcs);
  VMC(gs.ds_limit, nvm->vmcs, svmcs);
  VMC(gs.fs_limit, nvm->vmcs, svmcs);
  VMC(gs.gs_limit, nvm->vmcs, svmcs);
  VMC(gs.ldtr_limit, nvm->vmcs, svmcs);
  VMC(gs.tr_limit, nvm->vmcs, svmcs);
  VMC(gs.gdtr_limit, nvm->vmcs, svmcs);
  VMC(gs.idtr_limit, nvm->vmcs, svmcs);
  VMC(gs.es_ar_bytes, nvm->vmcs, svmcs);
  VMC(gs.cs_ar_bytes, nvm->vmcs, svmcs);
  VMC(gs.ss_ar_bytes, nvm->vmcs, svmcs);
  VMC(gs.ds_ar_bytes, nvm->vmcs, svmcs);
  VMC(gs.fs_ar_bytes, nvm->vmcs, svmcs);
  VMC(gs.gs_ar_bytes, nvm->vmcs, svmcs);
  VMC(gs.ldtr_ar_bytes, nvm->vmcs, svmcs);
  VMC(gs.tr_ar_bytes, nvm->vmcs, svmcs);
  VMC(gs.interruptibility_info, nvm->vmcs, svmcs);
  VMC(gs.activity_state, nvm->vmcs, svmcs);
  VMC(gs.smbase, nvm->vmcs, svmcs);
  VMC(gs.sysenter_cs, nvm->vmcs, svmcs);
  VMC(gs.vmcs_link_pointer, nvm->vmcs, svmcs);
  VMC(gs.interrupt_status, nvm->vmcs, svmcs);
  // Other control fields
  VMC(ctrls.entry.controls, nvm->vmcs, svmcs);
  VMC(ctrls.entry.intr_info_field, nvm->vmcs, svmcs);
  VMC(ctrls.ex.cr0_read_shadow, nvm->vmcs, svmcs);
  VMC(ctrls.ex.cr4_read_shadow, nvm->vmcs, svmcs);
  VMC(ctrls.ex.vmread_bitmap_addr, nvm->vmcs, svmcs);
  VMC(ctrls.ex.vmwrite_bitmap_addr, nvm->vmcs, svmcs);
  VMC(ctrls.ex.secondary_vm_exec_control, nvm->vmcs, svmcs);
#ifdef _VMCS_SHADOWING
  // !!! With VMCS shadowing write exits are bypassed !!!
  // Copy the Shadowing configuration from the shadow
  uint64_t sec_ctrl = svmcs->ctrls.ex.secondary_vm_exec_control &
    VMCS_SHADOWING;
  uint64_t bit31 = *(uint32_t *)rvm->shadow_ptr & (1 << 31);
  // Activate VMCS shadowing if any
  if (sec_ctrl) {
    VMW(crtls.ex.secondary_vm_exec_control,
        vmcs->crtls.ex.secondary_vm_exec_control | VMCS_SHADOWING);
  } else {
    VMW(crtls.ex.secondary_vm_exec_control,
        vmcs->crtls.ex.secondary_vm_exec_control & ~VMCS_SHADOWING);
  }
  // Set the type of the shadow_vmcs
  if (bit31) {
    *(uint32_t *)rvm->shadow_ptr |= (1 << 31);
  } else {
    *(uint32_t *)rvm->shadow_ptr &= ~(1 << 31);
  }
#endif
  vmcs = bvmcs;
}

void nested_shadow_collect(struct vmcs *svmcs) {
  struct vmcs *bvmcs = vmcs;
  vmcs = svmcs;
  vmcs_update();
  vmcs = bvmcs;
}

void nested_shadow_to_guest(struct vm *nvm) {
  // Clone encodings with field dirtyness
  vmcs_clone(&svmcs);
  cpu_vmptrld(rvm->shadow_ptr);
  // Collect vmcs shadow fields
  nested_shadow_collect(&svmcs);
  // Apply shadow to nvm vmcs cache
  nested_guest_shadow_apply(&svmcs, nvm);
  // Finally loads nvm VMCS region
  cpu_vmptrld(nvm->vmcs_region);
}

void nested_cpu_vmlaunch(struct registers *guest_regs) {
  // Commit the VMCS
  vmcs_commit();
  /* Correct rbp */
  __asm__ __volatile__("mov %0, %%rbp" : : "m" (guest_regs->rbp));
  /* Launch the vm */
  __asm__ __volatile__("vmlaunch;"
                       /* everything after should not be executed */
                       "setc %al;"
                       "setz %dl;"
                       "mov %eax, %edi;"
                       "mov %edx, %esi;"
                       "call vmx_transition_display_error");
}

void nested_vmresume(struct registers *guest_regs) {
  // DBG("This is the VMRESUME DUDES\n");

  // Change the current vm current child
  vm_child_shadow_set(vm);
  // Copy exec controls from vmcs0
  // uint32_t pinbased_ctls = cpu_vmread(PIN_BASED_VM_EXEC_CONTROL);
  nested_load_guest();

// XXX TODO see after
//  // Handle all interrupts of the virtualized VMMs to reroute them into linux
//  if (is_top_guest()) {
//    // Inject protential previous event into the top guest
//    nested_interrupt_inject();
//  } else {
//    // pinbased_ctls |= NMI_EXITING | EXT_INTR_EXITING;
//  }
//  // cpu_vmwrite(PIN_BASED_VM_EXEC_CONTROL, pinbased_ctls);

  // Set the current level
  level = vm->level;

#ifdef _NESTED_EPT
  // Set the mapping !
  ept_set_ctx(vm->index);
  uint64_t desc2[2] = {
    0x0000000000000000,
    0x000000000000ffff | 0x0000
  };
  uint64_t type2 = 0x2;
  __asm__ __volatile__("invept %0, %1" : : "m"(desc2), "r"(type2));
#endif
}

void nested_vmlaunch(struct registers *guest_regs) {
  uint64_t preempt_timer_value = cpu_vmread(VMX_PREEMPTION_TIMER_VALUE);
  struct vm *nvm;

  // Current shadow VMCS will be really executed, we allocate a VM for it
  vm_alloc(&nvm);

  // copy ctrl fields + host fields from host configuration to the nvm
  vmcs_clone(nvm->vmcs);

#ifdef _NESTED_EPT
  nested_smap_build(nvm->index);
  // Set the mapping !
  ept_set_ctx(nvm->index);
  uint64_t desc2[2] = {
    0x0000000000000000,
    0x000000000000ffff | 0x0000
  };
  uint64_t type2 = 0x2;
  __asm__ __volatile__("invept %0, %1" : : "m"(desc2), "r"(type2));
#endif

  // Copy the shadow to the guest
  nested_shadow_to_guest(nvm);
  // Change the state of the root VM
  rvm->state = NESTED_GUEST_RUNNING;
  // Add it to the current parent as a child
  vm_child_add(vm, nvm);
  // Set the VM as the current child
  vm->child = nvm;
  // Set the child VM level
  nvm->level = vm->level + 1;
  // Set the current level
  level = nvm->level;
  // Set the corresponding shadow_vmcs
  nvm->shadow_vmcs = rvm->shadow_ptr;
  // Set the new VM as the current VM
  vm_set(nvm);
  // Write the right VPID
  VMW(ctrls.ex.virtual_processor_id, nvm->index + 1); // VPIDs starts at 0
  // update vmx preemption timer
  VMW(gs.vmx_preemption_timer_value, preempt_timer_value);
  // Setting executive I/O bitmaps
  io_bitmap_clone_a_b(&io_bitmap_a_pool[nvm->index][0],
      &io_bitmap_b_pool[nvm->index][0]);
  VMW(ctrls.ex.io_bitmap_a, (uint64_t)&io_bitmap_a_pool[nvm->index][0]);
  VMW(ctrls.ex.io_bitmap_b, (uint64_t)&io_bitmap_b_pool[nvm->index][0]);
  // Setting executive msr bitmap
  msr_bitmap_clone((uint8_t *)&msr_bitmap_pool[nvm->index]);
  VMW(ctrls.ex.msr_bitmap, (uint64_t)&msr_bitmap_pool[nvm->index]);
  // Adjust vm entry controls
  vmm_adjust_vm_entry_controls();

  // DBG("This is the VMLAUNCH dudes\n");

  nested_cpu_vmlaunch(guest_regs);
}

uint64_t nested_vmread(uint64_t field) {
  uint64_t value;

  // Reading ro_data fields is done in guest_vmcs instead of shadow_vmcs
  if (((field >> 10) & 0x3) == 0x1) { // ro_data fields
    struct vm *v;
    vm_child_shadow_get(rvm, &v);
    if (v == NULL) {
      // The shadow has not been launched !
      cpu_vmptrld(rvm->shadow_ptr);
    } else {
      cpu_vmptrld(v->vmcs_region);
    }
  } else {
    cpu_vmptrld(rvm->shadow_ptr);
  }

  value = cpu_vmread(field);

  cpu_vmptrld(vm->vmcs_region);

  nested_set_vm_succeed();

  // DBG("This is the VMREAD dudes\n");
  return value;
}

void nested_vmwrite(uint64_t field, uint64_t value) {
  cpu_vmptrld(rvm->shadow_ptr);
  cpu_vmwrite(field, value);
  cpu_vmptrld(vm->vmcs_region);

  nested_set_vm_succeed();
  // DBG("This is the VMWRITE dudes : rip(0x%016X), encoding(0x%08x, 0x%016X)\n", cpu_vmread(GUEST_RIP), field, value);
}

/**
 * Apply the changes of a fully collected running VMCS to a shadow VMCS
 */
void nested_shadow_update(struct vmcs *svmcs, struct vm *vm) {
  // Guest state
  VMC(gs.es_selector, svmcs, vm->vmcs);
  VMC(gs.cs_selector, svmcs, vm->vmcs);
  VMC(gs.ss_selector, svmcs, vm->vmcs);
  VMC(gs.ds_selector, svmcs, vm->vmcs);
  VMC(gs.fs_selector, svmcs, vm->vmcs);
  VMC(gs.gs_selector, svmcs, vm->vmcs);
  VMC(gs.ldtr_selector, svmcs, vm->vmcs);
  VMC(gs.tr_selector, svmcs, vm->vmcs);
  VMC(gs.ia32_debugctl, svmcs, vm->vmcs);
  VMC(gs.ia32_pat, svmcs, vm->vmcs);
  VMC(gs.ia32_efer, svmcs, vm->vmcs);
  VMC(gs.ia32_perf_global_ctrl, svmcs, vm->vmcs);
  VMC(gs.pdptr0, svmcs, vm->vmcs);
  VMC(gs.pdptr1, svmcs, vm->vmcs);
  VMC(gs.pdptr2, svmcs, vm->vmcs);
  VMC(gs.pdptr3, svmcs, vm->vmcs);
  VMC(gs.cr0, svmcs, vm->vmcs);
  VMC(gs.cr3, svmcs, vm->vmcs);
  VMC(gs.cr4, svmcs, vm->vmcs);
  VMC(gs.es_base, svmcs, vm->vmcs);
  VMC(gs.cs_base, svmcs, vm->vmcs);
  VMC(gs.ss_base, svmcs, vm->vmcs);
  VMC(gs.ds_base, svmcs, vm->vmcs);
  VMC(gs.fs_base, svmcs, vm->vmcs);
  VMC(gs.gs_base, svmcs, vm->vmcs);
  VMC(gs.ldtr_base, svmcs, vm->vmcs);
  VMC(gs.tr_base, svmcs, vm->vmcs);
  VMC(gs.gdtr_base, svmcs, vm->vmcs);
  VMC(gs.idtr_base, svmcs, vm->vmcs);
  VMC(gs.dr7, svmcs, vm->vmcs);
  VMC(gs.rsp, svmcs, vm->vmcs);
  VMC(gs.rip, svmcs, vm->vmcs);
  VMC(gs.rflags, svmcs, vm->vmcs);
  VMC(gs.pending_dbg_exceptions, svmcs, vm->vmcs);
  VMC(gs.sysenter_esp, svmcs, vm->vmcs);
  VMC(gs.sysenter_eip, svmcs, vm->vmcs);
  VMC(gs.es_limit, svmcs, vm->vmcs);
  VMC(gs.cs_limit, svmcs, vm->vmcs);
  VMC(gs.ss_limit, svmcs, vm->vmcs);
  VMC(gs.ds_limit, svmcs, vm->vmcs);
  VMC(gs.fs_limit, svmcs, vm->vmcs);
  VMC(gs.gs_limit, svmcs, vm->vmcs);
  VMC(gs.ldtr_limit, svmcs, vm->vmcs);
  VMC(gs.tr_limit, svmcs, vm->vmcs);
  VMC(gs.gdtr_limit, svmcs, vm->vmcs);
  VMC(gs.idtr_limit, svmcs, vm->vmcs);
  VMC(gs.es_ar_bytes, svmcs, vm->vmcs);
  VMC(gs.cs_ar_bytes, svmcs, vm->vmcs);
  VMC(gs.ss_ar_bytes, svmcs, vm->vmcs);
  VMC(gs.ds_ar_bytes, svmcs, vm->vmcs);
  VMC(gs.fs_ar_bytes, svmcs, vm->vmcs);
  VMC(gs.gs_ar_bytes, svmcs, vm->vmcs);
  VMC(gs.ldtr_ar_bytes, svmcs, vm->vmcs);
  VMC(gs.tr_ar_bytes, svmcs, vm->vmcs);
  VMC(gs.interruptibility_info, svmcs, vm->vmcs);
  VMC(gs.activity_state, svmcs, vm->vmcs);
  VMC(gs.smbase, svmcs, vm->vmcs);
  VMC(gs.sysenter_cs, svmcs, vm->vmcs);
  VMC(gs.vmcs_link_pointer, svmcs, vm->vmcs);
  VMC(gs.interrupt_status, svmcs, vm->vmcs);
  // Other control fields
  VMC(ctrls.entry.controls, svmcs, vm->vmcs);
}

void nested_host_shadow_apply(struct vmcs *svmcs, struct vm *vm) {
  // Guest state
  VMC2(gs.es_selector, hs.es_selector, vm->vmcs, svmcs);
  VMC2(gs.cs_selector, hs.cs_selector, vm->vmcs, svmcs);
  VMC2(gs.ss_selector, hs.ss_selector, vm->vmcs, svmcs);
  VMC2(gs.ds_selector, hs.ds_selector, vm->vmcs, svmcs);
  VMC2(gs.fs_selector, hs.fs_selector, vm->vmcs, svmcs);
  VMC2(gs.gs_selector, hs.gs_selector, vm->vmcs, svmcs);
  VMC2(gs.tr_selector, hs.tr_selector, vm->vmcs, svmcs);
  VMC2(gs.ia32_pat, hs.ia32_pat, vm->vmcs, svmcs);
  VMC2(gs.ia32_efer, hs.ia32_efer, vm->vmcs, svmcs);
  VMC2(gs.ia32_perf_global_ctrl, hs.ia32_perf_global_ctrl, vm->vmcs, svmcs);
  VMC2(gs.cr0, hs.cr0, vm->vmcs, svmcs);
  VMC2(gs.cr3, hs.cr3, vm->vmcs, svmcs);
  VMC2(gs.cr4, hs.cr4, vm->vmcs, svmcs);
  VMC2(gs.fs_base, hs.fs_base, vm->vmcs, svmcs);
  VMC2(gs.gs_base, hs.gs_base, vm->vmcs, svmcs);
  VMC2(gs.tr_base, hs.tr_base, vm->vmcs, svmcs);
  VMC2(gs.gdtr_base, hs.gdtr_base, vm->vmcs, svmcs);
  VMC2(gs.idtr_base, hs.idtr_base, vm->vmcs, svmcs);
  VMC2(gs.sysenter_esp, hs.ia32_sysenter_esp, vm->vmcs, svmcs);
  VMC2(gs.sysenter_eip, hs.ia32_sysenter_eip, vm->vmcs, svmcs);
  VMC2(gs.sysenter_cs, hs.ia32_sysenter_cs, vm->vmcs, svmcs);
  VMC2(gs.rsp, hs.rsp, vm->vmcs, svmcs);
  VMC2(gs.rip, hs.rip, vm->vmcs, svmcs);
}

void nested_load_host(void) {
  uint64_t preempt_timer_value;

  // DBG("HOST LOADING DUDES\n");

  // Update the shadow VMCS with the new guest state
  
  // Save preemption timer
  VMR2(gs.vmx_preemption_timer_value, preempt_timer_value);

  // Init a VMCS encodings
  vmcs_encoding_init(&svmcs);
  // Collect every filds of the current VMCS
  vmcs_update();
  // copy all fields updated by vm_exit from guest_vmcs to shadow_vmcs
  nested_shadow_update(&svmcs, vm);
  // Set the shadow as the current vmcs
  vmcs = &svmcs;
  cpu_vmptrld(rvm->shadow_ptr);
  vmcs_commit();

  // Copy the host part from the shadow VMCS and set it as the guest
  //
  // Reset the shadow vmcs
  vmcs_encoding_init(&svmcs);
  // Collect every shadow vmcs fields
  vmcs_update();
  // Copy host fields from the shadow VMCS to the guest part of the VMCS
  nested_host_shadow_apply(&svmcs, rvm);
  // load host VMCS
  cpu_vmptrld(rvm->vmcs_region);
  // Set the root VM as the current running VM
  vm_set(rvm);

  // restore host fields from shadow_vmcs to vmcs0
#ifdef _NESTED_EPT
  // Restore ept mapping
  ept_set_ctx(rvm->index);
  uint64_t desc2[2] = {
    0x0000000000000000,
    0x000000000000ffff | 0x0000
  };
  uint64_t type2 = 0x2;
  __asm__ __volatile__("invept %0, %1" : : "m"(desc2), "r"(type2));
#endif

  // update vmx preemption timer
  VMW(gs.vmx_preemption_timer_value, preempt_timer_value);
  // Write the right VPID
  VMW(ctrls.ex.virtual_processor_id, rvm->index + 1); // VPIDs starts at 0

  vm->state = NESTED_HOST_RUNNING;
  level = rvm->level;

#ifdef _DEBUG_SERVER
  // handle Monitor trap flag
  debug_server_mtf();
#endif
}

void nested_load_guest(void) {
  uint64_t preempt_timer_value = cpu_vmread(VMX_PREEMPTION_TIMER_VALUE);
  uint8_t *io_bitmap_a, *io_bitmap_b, *msr_bitmap;

  nested_shadow_to_guest(vm->child);

  // update vmx preemption timer
  cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE, preempt_timer_value);

  rvm->state = NESTED_GUEST_RUNNING;

  // Set the current child VM as the current running VM
  vm_set(vm->child);

  // XXX Shadow vmcs is already loaded
  // Update executive io bitmap
  io_bitmap_a = (uint8_t*)svmcs.ctrls.ex.io_bitmap_a;
  io_bitmap_b = (uint8_t*)svmcs.ctrls.ex.io_bitmap_b;
  io_bitmap_or(&io_bitmap_a_pool[vm->index][0], &io_bitmap_b_pool[vm->index][0],
      io_bitmap_a, io_bitmap_b);
  // Update executive io bitmap
  msr_bitmap = (uint8_t*)svmcs.ctrls.ex.msr_bitmap;
  msr_bitmap_or((uint8_t *)&msr_bitmap_pool[vm->index], msr_bitmap);

#ifdef _DEBUG_SERVER
  // handle Monitor trap flag
  debug_server_mtf();
#endif
}

void nested_set_vm_succeed(void) {
  union rflags rf;
  VMR(gs.rflags);
  rf.raw = vmcs->gs.rflags;
  rf.cf = rf.pf = rf.af = rf.zf = rf.sf = rf.of = 0;
  VMW(gs.rflags, rf.raw);
}
