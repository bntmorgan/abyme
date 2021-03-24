/*
Copyright (C) 2021  Benoît Morgan

This file is part of abyme

abyme is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

abyme is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with abyme.  If not, see <http://www.gnu.org/licenses/>.
*/

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
#include "error.h"

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

void nested_vmxoff(struct registers *gr) {
  INFO("This is a VMXOFF\n");
  // Change the state of the root VM
  rvm->state = NESTED_DISABLED;
  // Deallocate VMs
  vm_free_all();
  memset(rvm->childs, 0, sizeof(struct vm *) * VM_NB);
  rvm->child = NULL;
}

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

  rvm->state = NESTED_HOST_RUNNING;

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
    ERROR_N_REBOOT("Shadow VMCS pointer is null\n");
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
// We need to read the shadow vmcs before running this function
void nested_smap_build(uint32_t index) {
  uint64_t start, end;
  uint64_t ta = 0;
  uint8_t in = 0;
  uint8_t max_phyaddr = cpuid_get_maxphyaddr();
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
        ERROR_N_REBOOT("BAD page size\n");
    }
    if (ta == ((uint64_t)1 << max_phyaddr)) {
      INFO("CHECK END maxphyaddr reached\n");
      return 0;
    } else {
      return 1;
    }
  }
  uint64_t eptp = svmcs.ctrls.ex.ept_pointer.raw & ~((uint64_t)0xfff);
  INFO("EPTP 0x%016X, idx 0x%x\n", eptp, index);
  if (ept_iterate(eptp, &cb)) {
    ERROR_N_REBOOT("PAGING error... 0x%x\n", paging_error);
  }
}
#endif

/**
 * Applies a fully collected shadow VMCS cache to the nvm VMCS cache
 */
void nested_guest_shadow_apply(struct vm *nvm) {
  struct vmcs *bvmcs = vmcs;
  vmcs = nvm->vmcs;
  // Guest state
  VMC(gs.es_selector, nvm->vmcs, &svmcs);
  VMC(gs.cs_selector, nvm->vmcs, &svmcs);
  VMC(gs.ss_selector, nvm->vmcs, &svmcs);
  VMC(gs.ds_selector, nvm->vmcs, &svmcs);
  VMC(gs.fs_selector, nvm->vmcs, &svmcs);
  VMC(gs.gs_selector, nvm->vmcs, &svmcs);
  VMC(gs.ldtr_selector, nvm->vmcs, &svmcs);
  VMC(gs.tr_selector, nvm->vmcs, &svmcs);
  VMC(gs.ia32_debugctl, nvm->vmcs, &svmcs);
  VMC(gs.ia32_pat, nvm->vmcs, &svmcs);
  VMC(gs.ia32_efer, nvm->vmcs, &svmcs);
  VMC(gs.ia32_perf_global_ctrl, nvm->vmcs, &svmcs);
  VMC(gs.pdpte0, nvm->vmcs, &svmcs);
  VMC(gs.pdpte1, nvm->vmcs, &svmcs);
  VMC(gs.pdpte2, nvm->vmcs, &svmcs);
  VMC(gs.pdpte3, nvm->vmcs, &svmcs);
  VMC(gs.cr0, nvm->vmcs, &svmcs);
  VMC(gs.cr3, nvm->vmcs, &svmcs);
  VMC(gs.cr4, nvm->vmcs, &svmcs);
  VMC(gs.es_base, nvm->vmcs, &svmcs);
  VMC(gs.cs_base, nvm->vmcs, &svmcs);
  VMC(gs.ss_base, nvm->vmcs, &svmcs);
  VMC(gs.ds_base, nvm->vmcs, &svmcs);
  VMC(gs.fs_base, nvm->vmcs, &svmcs);
  VMC(gs.gs_base, nvm->vmcs, &svmcs);
  VMC(gs.ldtr_base, nvm->vmcs, &svmcs);
  VMC(gs.tr_base, nvm->vmcs, &svmcs);
  VMC(gs.gdtr_base, nvm->vmcs, &svmcs);
  VMC(gs.idtr_base, nvm->vmcs, &svmcs);
  VMC(gs.dr7, nvm->vmcs, &svmcs);
  VMC(gs.rsp, nvm->vmcs, &svmcs);
  VMC(gs.rip, nvm->vmcs, &svmcs);
  VMC(gs.rflags, nvm->vmcs, &svmcs);
  VMC(gs.pending_dbg_exceptions, nvm->vmcs, &svmcs);
  VMC(gs.sysenter_esp, nvm->vmcs, &svmcs);
  VMC(gs.sysenter_eip, nvm->vmcs, &svmcs);
  VMC(gs.es_limit, nvm->vmcs, &svmcs);
  VMC(gs.cs_limit, nvm->vmcs, &svmcs);
  VMC(gs.ss_limit, nvm->vmcs, &svmcs);
  VMC(gs.ds_limit, nvm->vmcs, &svmcs);
  VMC(gs.fs_limit, nvm->vmcs, &svmcs);
  VMC(gs.gs_limit, nvm->vmcs, &svmcs);
  VMC(gs.ldtr_limit, nvm->vmcs, &svmcs);
  VMC(gs.tr_limit, nvm->vmcs, &svmcs);
  VMC(gs.gdtr_limit, nvm->vmcs, &svmcs);
  VMC(gs.idtr_limit, nvm->vmcs, &svmcs);
  VMC(gs.es_ar_bytes, nvm->vmcs, &svmcs);
  VMC(gs.cs_ar_bytes, nvm->vmcs, &svmcs);
  VMC(gs.ss_ar_bytes, nvm->vmcs, &svmcs);
  VMC(gs.ds_ar_bytes, nvm->vmcs, &svmcs);
  VMC(gs.fs_ar_bytes, nvm->vmcs, &svmcs);
  VMC(gs.gs_ar_bytes, nvm->vmcs, &svmcs);
  VMC(gs.ldtr_ar_bytes, nvm->vmcs, &svmcs);
  VMC(gs.tr_ar_bytes, nvm->vmcs, &svmcs);
  VMC(gs.interruptibility_info, nvm->vmcs, &svmcs);
  VMC(gs.activity_state, nvm->vmcs, &svmcs);
  VMC(gs.smbase, nvm->vmcs, &svmcs);
  VMC(gs.ia32_sysenter_cs, nvm->vmcs, &svmcs);
  VMC(gs.vmcs_link_pointer, nvm->vmcs, &svmcs);
  VMC(gs.interrupt_status, nvm->vmcs, &svmcs);
  vmcs = bvmcs;
}

void nested_shadow_collect(void) {
  struct vmcs *bvmcs = vmcs;
  vmcs = &svmcs;
  vmcs_force_update();
  vmcs = bvmcs;
}

/**
 * Or the controls needed by the virtualized hypervisor with host config
 */
void nested_ctrls_shadow_apply(struct vm *vm) {
  union pin_based spnb, hpnb, gpnb;
  union proc_based spb, hpb, gpb;
  union proc_based_2 spb2, hpb2, gpb2;
  uint32_t exception_bitmap;
  uint32_t cr0_guest_host_mask, cr4_guest_host_mask;
  uint32_t entry_controls, exit_controls;

  // Ajust pin based controls
  spnb.raw = svmcs.ctrls.ex.pin_based_vm_exec_control.raw;
  hpnb.raw = hc->ctrls.ex.pin_based_vm_exec_control.raw;
  gpnb.raw = hpnb.raw | spnb.raw;
  VMW3(vm->vmcs, ctrls.ex.pin_based_vm_exec_control, gpnb.raw);

  // Ajust cpu_based_vm_exec_control
  spb.raw = svmcs.ctrls.ex.cpu_based_vm_exec_control.raw;
  hpb.raw = hc->ctrls.ex.cpu_based_vm_exec_control.raw;
  gpb.raw = hpb.raw | spb.raw;
  VMW3(vm->vmcs, ctrls.ex.cpu_based_vm_exec_control, gpb.raw);

  // Ajust secondary controls
  spb2.raw = svmcs.ctrls.ex.secondary_vm_exec_control.raw;
  hpb2.raw = hc->ctrls.ex.secondary_vm_exec_control.raw;
  gpb2.raw = hpb2.raw | spb2.raw;

  // Adjust some mandatory fiedls
  gpb2.unrestricted_guest = spb2.unrestricted_guest;
  VMW3(vm->vmcs, ctrls.ex.secondary_vm_exec_control, gpb2.raw);

  // Adjust cr0/cr4 guest/host mask
  cr0_guest_host_mask = hc->ctrls.ex.cr0_guest_host_mask.raw |
    svmcs.ctrls.ex.cr0_guest_host_mask.raw;
  VMW3(vm->vmcs, ctrls.ex.cr0_guest_host_mask, cr0_guest_host_mask);
  cr4_guest_host_mask = hc->ctrls.ex.cr4_guest_host_mask.raw |
    svmcs.ctrls.ex.cr4_guest_host_mask.raw;
  VMW3(vm->vmcs, ctrls.ex.cr4_guest_host_mask, cr4_guest_host_mask);

  // copy cr0/cr4 read shadow
  VMC(ctrls.ex.cr0_read_shadow, vm->vmcs, &svmcs);
  VMC(ctrls.ex.cr4_read_shadow, vm->vmcs, &svmcs);

  // Tells the vmm to add this to tsc offset
  // TODO see if this is necessary with more than one VMM virtualized
  tsc_l1_offset = svmcs.ctrls.ex.tsc_offset.raw;

  // copy vapic page address : we don't virtualize it for the moment
  VMC(ctrls.ex.virtual_apic_page_addr, vm->vmcs, &svmcs);

  // copy tsc offset
  VMC(ctrls.ex.tsc_offset, vm->vmcs, &svmcs);

  // Exception bitmap
  exception_bitmap = hc->ctrls.ex.exception_bitmap.raw |
    svmcs.ctrls.ex.exception_bitmap.raw;
  VMW3(vm->vmcs, ctrls.ex.exception_bitmap, exception_bitmap);

  // Page fault masks
  VMC(ctrls.ex.page_fault_error_code_mask, vm->vmcs, &svmcs);
  VMC(ctrls.ex.page_fault_error_code_match, vm->vmcs, &svmcs);
  VMC(ctrls.ex.virt_excep_info_addr, vm->vmcs, &svmcs);

  // VMExit
  VMC(ctrls.exit.msr_load_addr, vm->vmcs, &svmcs);
  VMC(ctrls.exit.msr_store_addr, vm->vmcs, &svmcs);
  VMC(ctrls.exit.msr_store_count, vm->vmcs, &svmcs);
  VMC(ctrls.exit.msr_load_count, vm->vmcs, &svmcs);

  // VMEntry
  VMC(ctrls.entry.msr_load_addr, vm->vmcs, &svmcs);
  VMC(ctrls.entry.msr_load_count, vm->vmcs, &svmcs);
  VMC(ctrls.entry.intr_info_field, vm->vmcs, &svmcs);
  VMC(ctrls.entry.exception_error_code, vm->vmcs, &svmcs);
  VMC(ctrls.entry.instruction_len, vm->vmcs, &svmcs);

  // VMExit controls or VMEntry controls
  exit_controls = svmcs.ctrls.exit.controls.raw | hc->ctrls.exit.controls.raw;
  VMW3(vm->vmcs, ctrls.exit.controls, exit_controls);
  entry_controls = svmcs.ctrls.entry.controls.raw |
    hc->ctrls.entry.controls.raw;
  VMW3(vm->vmcs, ctrls.entry.controls, entry_controls);

  // Update executive io bitmap
  io_bitmap_a_shadow = (uint8_t*)svmcs.ctrls.ex.io_bitmap_a.raw;
  io_bitmap_b_shadow = (uint8_t*)svmcs.ctrls.ex.io_bitmap_b.raw;
  io_bitmap_or(&io_bitmap_a_pool[vm->index][0], &io_bitmap_b_pool[vm->index][0],
      io_bitmap_a_shadow, io_bitmap_b_shadow);
  // Update executive msr bitmap
  msr_bitmap_shadow = (struct msr_bitmap*)svmcs.ctrls.ex.msr_bitmap.raw;
  msr_bitmap_or((uint8_t*)&msr_bitmap_pool[vm->index],
      (uint8_t*)msr_bitmap_shadow);

  // Other control fields
  VMC(ctrls.entry.intr_info_field, vm->vmcs, &svmcs);
  VMC(ctrls.ex.vmread_bitmap_addr, vm->vmcs, &svmcs);
  VMC(ctrls.ex.vmwrite_bitmap_addr, vm->vmcs, &svmcs);

#ifdef _VMCS_SHADOWING
  // !!! With VMCS shadowing write exits are bypassed !!!
  // Copy the Shadowing configuration from the shadow
  uint64_t sec_ctrl = svmcs.ctrls.ex.secondary_vm_exec_control.raw &
    VMCS_SHADOWING;
  uint64_t bit31 = *(uint32_t *)rvm->shadow_ptr & (1 << 31);
  // Activate VMCS shadowing if any
  if (sec_ctrl) {
    VMW3(vm->vmcs, ctrls.ex.secondary_vm_exec_control,
        vmcs->ctrls.ex.secondary_vm_exec_control.raw | VMCS_SHADOWING);
  } else {
    VMW3(vm->vmcs, ctrls.ex.secondary_vm_exec_control,
        vmcs->ctrls.ex.secondary_vm_exec_control.raw & ~VMCS_SHADOWING);
  }
  // Set the type of the shadow_vmcs
  if (bit31) {
    *(uint32_t *)rvm->shadow_ptr |= (1 << 31);
  } else {
    *(uint32_t *)rvm->shadow_ptr &= ~(1 << 31);
  }
#endif
}

void nested_shadow_to_guest(struct vm *nvm) {
  // Clone encodings with field dirtyness
  vmcs_clone(&svmcs); // XXX why dude ?
  cpu_vmptrld(rvm->shadow_ptr);
  // Collect vmcs shadow fields
  nested_shadow_collect();
  // Apply shadow to nvm vmcs cache
  nested_guest_shadow_apply(nvm);
  // Apply shadow to nvm execution controls
  nested_ctrls_shadow_apply(nvm);
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

  // Adjust vm entry controls TODO : refactor
  vmm_adjust_vm_entry_controls();

#ifdef _NESTED_EPT
  // Set the mapping !
  ept_set_ctx(vm->index);
  uint64_t desc[2] = {
    0x0000000000000000,
    0x000000000000ffff | 0x0000
  };
  uint64_t type = 0x2;
  __asm__ __volatile__("invept %0, %1" : : "m"(desc), "r"(type));
#endif

  cpu_invvpid(0x1, vm->index + 1, 0);
}

void nested_vmlaunch(struct registers *guest_regs) {
  uint64_t preempt_timer_value = cpu_vmread(VMX_PREEMPTION_TIMER_VALUE);
  struct vm *nvm;

  INFO("This is a VMLAUNCH\n");

  // Current shadow VMCS will be really executed, we allocate a VM for it
  vm_alloc(&nvm);
  // Clear the VMCS
  cpu_vmclear(nvm->vmcs_region);
  // copy ctrl fields + host fields from host configuration to the nvm
  vmcs_clone(nvm->vmcs);

  // Copy the shadow to the guest
  nested_shadow_to_guest(nvm);
  // Change the state of the root VM
  rvm->state = NESTED_GUEST_RUNNING;
  // Add it to the current parent as a child
  vm_child_add(vm, nvm);
  // Set the VM as the current child
  vm->child = nvm;
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
  // Adjust vm entry controls TODO : refactor
  vmm_adjust_vm_entry_controls();

#ifdef _NESTED_EPT
  // SMAP Build only if EPT is enabled
  if (svmcs.ctrls.ex.secondary_vm_exec_control.enable_ept) {
    nested_smap_build(nvm->index);
  }
  // Set the mapping !
  ept_set_ctx(nvm->index);
  uint64_t desc2[2] = {
    0x0000000000000000,
    0x000000000000ffff | 0x0000
  };
  uint64_t type2 = 0x2;
  __asm__ __volatile__("invept %0, %1" : : "m"(desc2), "r"(type2));
#endif

  nested_cpu_vmlaunch(guest_regs);
}

uint64_t nested_vmread(uint64_t field) {
  uint64_t value;

  // Reading ro_data fields is done in executive vmcs  instead of shadow_vmcs
  if (((field >> 10) & 0x3) == 0x1) { // ro_data fields
    struct vm *v;
    vm_child_shadow_get(rvm, &v);
    if (v == NULL) {
      // The shadow has not been launched !
      cpu_vmptrld(rvm->shadow_ptr);
      // XXX
      // INFO("Reading the executive VMCS\n");
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
  VMC(gs.pdpte0, svmcs, vm->vmcs);
  VMC(gs.pdpte1, svmcs, vm->vmcs);
  VMC(gs.pdpte2, svmcs, vm->vmcs);
  VMC(gs.pdpte3, svmcs, vm->vmcs);
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
  VMC(gs.ia32_sysenter_cs, svmcs, vm->vmcs);
  VMC(gs.vmcs_link_pointer, svmcs, vm->vmcs);
  VMC(gs.interrupt_status, svmcs, vm->vmcs);
}

void nested_host_shadow_apply(struct vm *vm) {
  // Guest state
  VMC2(gs.es_selector, hs.es_selector, vm->vmcs, &svmcs);
  VMC2(gs.cs_selector, hs.cs_selector, vm->vmcs, &svmcs);
  VMC2(gs.ss_selector, hs.ss_selector, vm->vmcs, &svmcs);
  VMC2(gs.ds_selector, hs.ds_selector, vm->vmcs, &svmcs);
  VMC2(gs.fs_selector, hs.fs_selector, vm->vmcs, &svmcs);
  VMC2(gs.gs_selector, hs.gs_selector, vm->vmcs, &svmcs);
  VMC2(gs.tr_selector, hs.tr_selector, vm->vmcs, &svmcs);
  VMC2(gs.ia32_pat, hs.ia32_pat, vm->vmcs, &svmcs);
  VMC2(gs.ia32_efer, hs.ia32_efer, vm->vmcs, &svmcs);
  VMC2(gs.ia32_perf_global_ctrl, hs.ia32_perf_global_ctrl, vm->vmcs, &svmcs);
  VMC2(gs.cr0, hs.cr0, vm->vmcs, &svmcs);
  VMC2(gs.cr3, hs.cr3, vm->vmcs, &svmcs);
  VMC2(gs.cr4, hs.cr4, vm->vmcs, &svmcs);
  VMC2(gs.fs_base, hs.fs_base, vm->vmcs, &svmcs);
  VMC2(gs.gs_base, hs.gs_base, vm->vmcs, &svmcs);
  VMC2(gs.tr_base, hs.tr_base, vm->vmcs, &svmcs);
  VMC2(gs.gdtr_base, hs.gdtr_base, vm->vmcs, &svmcs);
  VMC2(gs.idtr_base, hs.idtr_base, vm->vmcs, &svmcs);
  VMC2(gs.sysenter_esp, hs.ia32_sysenter_esp, vm->vmcs, &svmcs);
  VMC2(gs.sysenter_eip, hs.ia32_sysenter_eip, vm->vmcs, &svmcs);
  VMC2(gs.ia32_sysenter_cs, hs.ia32_sysenter_cs, vm->vmcs, &svmcs);
  VMC2(gs.rsp, hs.rsp, vm->vmcs, &svmcs);
  VMC2(gs.rip, hs.rip, vm->vmcs, &svmcs);

  // Ajust certain Guest State fields according to Chapter 27.5 Volume 3 of
  // Intel programming manual

  VMW3(vm->vmcs, gs.dr7, 0x400);
  VMW3(vm->vmcs, gs.ia32_debugctl, 0);
  VMW3(vm->vmcs, gs.ia32_sysenter_cs, 0);

  VMW3(vm->vmcs, gs.cs_base, 0);
  VMW3(vm->vmcs, gs.ss_base, 0);
  VMW3(vm->vmcs, gs.ds_base, 0);

  VMW3(vm->vmcs, gs.cs_limit, 0xffffffff);
  VMW3(vm->vmcs, gs.ss_limit, 0xffffffff);
  VMW3(vm->vmcs, gs.ds_limit, 0xffffffff);
  VMW3(vm->vmcs, gs.es_limit, 0xffffffff);
  VMW3(vm->vmcs, gs.fs_limit, 0xffffffff);
  VMW3(vm->vmcs, gs.gs_limit, 0xffffffff);
  VMW3(vm->vmcs, gs.tr_limit, 0x00000067);

  // Sgement type, DPL, P

  // Dirty it
  VMD2(vm->vmcs, gs.cs_ar_bytes);
  VMD2(vm->vmcs, gs.ss_ar_bytes);
  VMD2(vm->vmcs, gs.ds_ar_bytes);
  VMD2(vm->vmcs, gs.es_ar_bytes);
  VMD2(vm->vmcs, gs.fs_ar_bytes);
  VMD2(vm->vmcs, gs.gs_ar_bytes);
  VMD2(vm->vmcs, gs.tr_ar_bytes);
  VMD2(vm->vmcs, gs.ldtr_ar_bytes);

  vm->vmcs->gs.cs_ar_bytes.type = 11;
  vm->vmcs->gs.ss_ar_bytes.type = 3;
  vm->vmcs->gs.ds_ar_bytes.type = 3;
  vm->vmcs->gs.es_ar_bytes.type = 3;
  vm->vmcs->gs.fs_ar_bytes.type = 3;
  vm->vmcs->gs.gs_ar_bytes.type = 3;
  vm->vmcs->gs.tr_ar_bytes.type = 11;

  vm->vmcs->gs.cs_ar_bytes.s = 1;
  vm->vmcs->gs.ss_ar_bytes.s = 1;
  vm->vmcs->gs.ds_ar_bytes.s = 1;
  vm->vmcs->gs.es_ar_bytes.s = 1;
  vm->vmcs->gs.fs_ar_bytes.s = 1;
  vm->vmcs->gs.gs_ar_bytes.s = 1;
  vm->vmcs->gs.tr_ar_bytes.s = 0;

  vm->vmcs->gs.cs_ar_bytes.dpl = 0;
  vm->vmcs->gs.ss_ar_bytes.dpl = 0;
  vm->vmcs->gs.ds_ar_bytes.dpl = 0;
  vm->vmcs->gs.es_ar_bytes.dpl = 0;
  vm->vmcs->gs.fs_ar_bytes.dpl = 0;
  vm->vmcs->gs.gs_ar_bytes.dpl = 0;
  vm->vmcs->gs.tr_ar_bytes.dpl = 0;

  vm->vmcs->gs.cs_ar_bytes.p = 1;
  vm->vmcs->gs.ss_ar_bytes.p = 1;
  vm->vmcs->gs.ds_ar_bytes.p = 1;
  vm->vmcs->gs.es_ar_bytes.p = 1;
  vm->vmcs->gs.fs_ar_bytes.p = 1;
  vm->vmcs->gs.gs_ar_bytes.p = 1;
  vm->vmcs->gs.tr_ar_bytes.p = 1;

  // "host address-space size" VM-exit control value
  vm->vmcs->gs.cs_ar_bytes.l =
    svmcs.ctrls.exit.controls.host_address_space_size;

  // ! "host address-space size" VM-exit control value
  vm->vmcs->gs.cs_ar_bytes.d =
    ~svmcs.ctrls.exit.controls.host_address_space_size;
  vm->vmcs->gs.ss_ar_bytes.d = 1;
  vm->vmcs->gs.ds_ar_bytes.d = 1;
  vm->vmcs->gs.es_ar_bytes.d = 1;
  vm->vmcs->gs.fs_ar_bytes.d = 1;
  vm->vmcs->gs.gs_ar_bytes.d = 1;
  vm->vmcs->gs.tr_ar_bytes.d = 0;

  vm->vmcs->gs.cs_ar_bytes.g = 1;
  vm->vmcs->gs.ss_ar_bytes.g = 1;
  vm->vmcs->gs.ds_ar_bytes.g = 1;
  vm->vmcs->gs.es_ar_bytes.g = 1;
  vm->vmcs->gs.fs_ar_bytes.g = 1;
  vm->vmcs->gs.gs_ar_bytes.g = 1;
  vm->vmcs->gs.tr_ar_bytes.g = 0;

  VMD2(vm->vmcs, gs.ldtr_selector);
  vm->vmcs->gs.ldtr_ar_bytes.raw = 0;
  vm->vmcs->gs.ldtr_ar_bytes.unusable = 1;

  VMW3(vm->vmcs, gs.gdtr_limit, 0xffff);
  VMW3(vm->vmcs, gs.idtr_limit, 0xffff);

  VMW3(vm->vmcs, gs.pending_dbg_exceptions, 0);

  VMW3(vm->vmcs, gs.rflags, 0x2);

  // XXX no info on that
  VMW3(vm->vmcs, gs.interruptibility_info, 0);
  VMW3(vm->vmcs, gs.interrupt_status, 0);
  VMW3(vm->vmcs, gs.activity_state, 0);
  VMW3(vm->vmcs, gs.smbase, 0);
}

void nested_load_host(void) {
  uint64_t preempt_timer_value;

  // Update the shadow VMCS with the new guest state

  // Save preemption timer
  VMR2(gs.vmx_preemption_timer_value, preempt_timer_value);

  // Init a VMCS encodings
  vmcs_encoding_init(&svmcs);
  // Collect every filds of the current VMCS
  vmcs_force_update();
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
  vmcs_force_update();
  // Copy host fields from the shadow VMCS to the guest part of the VMCS
  nested_host_shadow_apply(rvm);
  // load host VMCS
  cpu_vmptrld(rvm->vmcs_region);
  // Set the root VM as the current running VM
  vm_set(rvm);

  // restore host fields from shadow_vmcs to vmcs0
#ifdef _NESTED_EPT
  // Restore ept mapping
  ept_set_ctx(rvm->index);
  uint64_t desc[2] = {
    0x0000000000000000,
    0x000000000000ffff | 0x0000
  };
  uint64_t type = 0x2;
  __asm__ __volatile__("invept %0, %1" : : "m"(desc), "r"(type));
#endif
  // DEACTIVATE IT the doesn't need to be invalidated
  // cpu_invvpid(0x1, vm->index + 1, 0);

  nested_set_vm_succeed();

  // update vmx preemption timer
  VMW(gs.vmx_preemption_timer_value, preempt_timer_value);
  // Write the right VPID
  VMW(ctrls.ex.virtual_processor_id, rvm->index + 1); // VPIDs starts at 0

  rvm->state = NESTED_HOST_RUNNING;

  // XXX YOLO
  // INFO("HOST LOADED DUDES\n");
  // vmcs_commit();
  // vmcs_force_update();
  // INFO("\n\n   YOLO shadow\n\n");
  // vmcs_dump_hs(&svmcs);
  // INFO("\n\n   YOLO executive\n\n");
  // vmcs_dump_gs(vmcs);

#ifdef _DEBUG_SERVER
//   debug_server_send_debug_all();
//   set_mtf();
#endif
}

void nested_load_guest(void) {
  uint64_t preempt_timer_value = cpu_vmread(VMX_PREEMPTION_TIMER_VALUE);

  // DBG("GUEST LOADING DUDES\n");

  nested_shadow_to_guest(vm->child);

  // update vmx preemption timer
  cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE, preempt_timer_value);

  rvm->state = NESTED_GUEST_RUNNING;

  // Set the current child VM as the current running VM
  vm_set(vm->child);
}

void nested_set_vm_succeed(void) {
  union rflags rf;
  VMR(gs.rflags);
  rf.raw = vmcs->gs.rflags.raw;
  rf.cf = rf.pf = rf.af = rf.zf = rf.sf = rf.of = 0;
  VMW(gs.rflags, rf.raw);
}
