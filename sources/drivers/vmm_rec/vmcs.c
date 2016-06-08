#include "vmm.h"
#include "vmx.h"
#include "gdt.h"
#include "idt.h"
#include "mtrr.h"
#include "ept.h"
#include "vmcs.h"
#include "msr_bitmap.h"
#include "io_bitmap.h"
#include "paging.h"
#include "efiw.h"

#include "string.h"
#include "stdio.h"

#include "cpu.h"
#include "msr.h"

uint8_t *vmxon;

/**
 * Host vmcs configuration reference
 */
struct vmcs *hc;

/**
 * Current VMCS
 */
struct vmcs *vmcs;

/**
 * VMCS cache pool
 */
struct vmcs *vmcs_cache_pool;

/**
 * VMCS region pool
 */
uint8_t (*vmcs_region_pool)[0x1000];

static uint16_t tsc_freq_MHz;
static uint8_t tsc_divider;

/**
 * Clone host configuration VMCS
 */
void vmcs_clone(struct vmcs *v) {
  memcpy(v, hc, sizeof(struct vmcs));
}

void vmcs_create_vmcs_regions(void) {
  uint32_t i;
  memset((uint8_t *) &vmxon[0], 0, 0x1000);
  *((uint32_t *) &vmxon[0]) = hc->revision_id;
  for (i = 0; i < VM_NB; i++) {
    *((uint32_t *) &vmcs_region_pool[i][0]) = hc->revision_id;
  }
}

/**
 * Specific field dumping
 */

void print_exit_controls(union exit_controls *c) {
  printk("    save_debug_controls: 0x%x\n", c->save_debug_controls);
  printk("    host_address_space_size: 0x%x\n", c->host_address_space_size);
  printk("    load_ia32_perf_global_ctrl: 0x%x\n", c->load_ia32_perf_global_ctrl);
  printk("    acknoledge_interrupt_on_exit: 0x%x\n", c->acknoledge_interrupt_on_exit);
  printk("    save_ia32_pat: 0x%x\n", c->save_ia32_pat);
  printk("    load_ia32_pat: 0x%x\n", c->load_ia32_pat);
  printk("    save_ia32_efer: 0x%x\n", c->save_ia32_efer);
  printk("    load_ia32_efer: 0x%x\n", c->load_ia32_efer);
  printk("    save_vmx_preemption_timer_value: 0x%x\n", c->save_vmx_preemption_timer_value);
}

void print_entry_controls(union entry_controls *c) {
  printk("    load_debug_controls: 0x%x\n", c->load_debug_controls);
  printk("    ia32_mode_guest: 0x%x\n", c->ia32_mode_guest);
  printk("    entry_to_smm: 0x%x\n", c->entry_to_smm);
  printk("    deactivate_dual_monitor_treatment: 0x%x\n", c->deactivate_dual_monitor_treatment);
  printk("    load_ia32_perf_global_ctrl: 0x%x\n", c->load_ia32_perf_global_ctrl);
  printk("    load_ia32_pat: 0x%x\n", c->load_ia32_pat);
  printk("    load_ia32_efer: 0x%x\n", c->load_ia32_efer);
}

void print_pin_based(union pin_based *c) {
  printk("    external_interrupt_exiting: 0x%x\n", c->external_interrupt_exiting);
  printk("    nmi_exiting: 0x%x\n", c->nmi_exiting);
  printk("    virtual_nmi: 0x%x\n", c->virtual_nmi);
  printk("    activate_vmx_preemption_timer: 0x%x\n", c->activate_vmx_preemption_timer);
  printk("    process_posted_interrupts: 0x%x\n", c->process_posted_interrupts);
}

void print_proc_based(union proc_based *c) {
  printk("    interrupt_window_exiting: 0x%x\n", c->interrupt_window_exiting);
  printk("    use_tsc_offsetting: 0x%x\n", c->use_tsc_offsetting);
  printk("    hlt_exiting: 0x%x\n", c->hlt_exiting);
  printk("    invlpg_exiting: 0x%x\n", c->invlpg_exiting);
  printk("    mwait_exiting: 0x%x\n", c->mwait_exiting);
  printk("    rdpmc_exiting: 0x%x\n", c->rdpmc_exiting);
  printk("    rdtsc_exiting: 0x%x\n", c->rdtsc_exiting);
  printk("    cr3_load_exiting: 0x%x\n", c->cr3_load_exiting);
  printk("    cr3_store_exiting: 0x%x\n", c->cr3_store_exiting);
  printk("    cr8_load_exiting: 0x%x\n", c->cr8_load_exiting);
  printk("    cr8_store_exiting: 0x%x\n", c->cr8_store_exiting);
  printk("    use_tpr_shadow: 0x%x\n", c->use_tpr_shadow);
  printk("    nmi_window_exiting: 0x%x\n", c->nmi_window_exiting);
  printk("    mov_dr_exiting: 0x%x\n", c->mov_dr_exiting);
  printk("    unconditional_io_exiting: 0x%x\n", c->unconditional_io_exiting);
  printk("    use_io_bitmaps: 0x%x\n", c->use_io_bitmaps);
  printk("    monitor_trap_flag: 0x%x\n", c->monitor_trap_flag);
  printk("    use_msr_bitmaps: 0x%x\n", c->use_msr_bitmaps);
  printk("    monitor_exiting: 0x%x\n", c->monitor_exiting);
  printk("    pause_exiting: 0x%x\n", c->pause_exiting);
  printk("    activate_secondary_controls: 0x%x\n", c->activate_secondary_controls);
}

void print_proc_based_2(union proc_based_2 *c) {
  printk("    virtualize_apic_access: 0x%x\n", c->virtualize_apic_access);
  printk("    enable_ept: 0x%x\n", c->enable_ept);
  printk("    descriptor_table_exiting: 0x%x\n", c->descriptor_table_exiting);
  printk("    enable_rdtscp: 0x%x\n", c->enable_rdtscp);
  printk("    virtualize_x2apic_mode: 0x%x\n", c->virtualize_x2apic_mode);
  printk("    enable_vpid: 0x%x\n", c->enable_vpid);
  printk("    wbinvd_exiting: 0x%x\n", c->wbinvd_exiting);
  printk("    unrestricted_guest: 0x%x\n", c->unrestricted_guest);
  printk("    apic_register_virtualization: 0x%x\n", c->apic_register_virtualization);
  printk("    virtual_interrupt_delivery: 0x%x\n", c->virtual_interrupt_delivery);
  printk("    pause_loop_exiting: 0x%x\n", c->pause_loop_exiting);
  printk("    rdrand_exiting: 0x%x\n", c->rdrand_exiting);
  printk("    enable_invpcid: 0x%x\n", c->enable_invpcid);
  printk("    enable_vm_functions: 0x%x\n", c->enable_vm_functions);
  printk("    vmcs_shadowing: 0x%x\n", c->vmcs_shadowing);
  printk("    rdseed_exiting: 0x%x\n", c->rdseed_exiting);
  printk("    ept_violation_ve: 0x%x\n", c->ept_violation_ve);
  printk("    enable_xsaves_xrstors: 0x%x\n", c->enable_xsaves_xrstors);
}

void vmcs_dump_ctrls(struct vmcs *v) {
  printk("Execution controls\n");
  VMP(v, ctrls.ex.virtual_processor_id);
  VMP(v, ctrls.ex.posted_int_notif_vector);
  VMP(v, ctrls.ex.eptp_index);
  VMP(v, ctrls.ex.io_bitmap_a);
  VMP(v, ctrls.ex.io_bitmap_b);
  VMP(v, ctrls.ex.msr_bitmap);
  VMP(v, ctrls.ex.executive_vmcs_pointer);
  VMP(v, ctrls.ex.tsc_offset);
  VMP(v, ctrls.ex.virtual_apic_page_addr);
  VMP(v, ctrls.ex.apic_access_addr);
  VMP(v, ctrls.ex.posted_intr_desc_addr);
  VMP(v, ctrls.ex.vm_function_controls);
  VMP(v, ctrls.ex.ept_pointer);
  VMP(v, ctrls.ex.eoi_exit_bitmap_0);
  VMP(v, ctrls.ex.eoi_exit_bitmap_1);
  VMP(v, ctrls.ex.eoi_exit_bitmap_2);
  VMP(v, ctrls.ex.eoi_exit_bitmap_3);
  VMP(v, ctrls.ex.eptp_list_addr);
  VMP(v, ctrls.ex.vmread_bitmap_addr);
  VMP(v, ctrls.ex.vmwrite_bitmap_addr);
  VMP(v, ctrls.ex.virt_excep_info_addr);
  VMP(v, ctrls.ex.xss_exiting_bitmap);
  VMP(v, ctrls.ex.pin_based_vm_exec_control);
  print_pin_based(&v->ctrls.ex.pin_based_vm_exec_control);
  VMP(v, ctrls.ex.cpu_based_vm_exec_control);
  print_proc_based(&v->ctrls.ex.cpu_based_vm_exec_control);
  VMP(v, ctrls.ex.exception_bitmap);
  VMP(v, ctrls.ex.page_fault_error_code_mask);
  VMP(v, ctrls.ex.page_fault_error_code_match);
  VMP(v, ctrls.ex.cr3_target_count);
  VMP(v, ctrls.ex.tpr_threshold);
  VMP(v, ctrls.ex.secondary_vm_exec_control);
  print_proc_based_2(&v->ctrls.ex.secondary_vm_exec_control);
  VMP(v, ctrls.ex.ple_gap);
  VMP(v, ctrls.ex.ple_window);
  VMP(v, ctrls.ex.cr0_guest_host_mask);
  VMP(v, ctrls.ex.cr4_guest_host_mask);
  VMP(v, ctrls.ex.cr0_read_shadow);
  VMP(v, ctrls.ex.cr4_read_shadow);
  VMP(v, ctrls.ex.cr3_target_value0);
  VMP(v, ctrls.ex.cr3_target_value1);
  VMP(v, ctrls.ex.cr3_target_value2);
  VMP(v, ctrls.ex.cr3_target_value3);
  printk("Vm exit controls\n");
  VMP(v, ctrls.exit.msr_store_addr);
  VMP(v, ctrls.exit.msr_load_addr);
  VMP(v, ctrls.exit.controls);
  print_exit_controls(&v->ctrls.exit.controls);
  VMP(v, ctrls.exit.msr_store_count);
  VMP(v, ctrls.exit.msr_load_count);
  printk("VM entry controls\n");
  VMP(v, ctrls.entry.msr_load_addr);
  VMP(v, ctrls.entry.controls);
  print_entry_controls(&v->ctrls.entry.controls);
  VMP(v, ctrls.entry.msr_load_count);
  VMP(v, ctrls.entry.intr_info_field);
  VMP(v, ctrls.entry.exception_error_code);
  VMP(v, ctrls.entry.instruction_len);
}

void vmcs_dump_gs(struct vmcs *v) {
  printk("Guest state\n");
  VMP(v, gs.es_selector);
  VMP(v, gs.cs_selector);
  VMP(v, gs.ss_selector);
  VMP(v, gs.ds_selector);
  VMP(v, gs.fs_selector);
  VMP(v, gs.gs_selector);
  VMP(v, gs.ldtr_selector);
  VMP(v, gs.tr_selector);
  VMP(v, gs.ia32_debugctl);
  VMP(v, gs.ia32_pat);
  VMP(v, gs.ia32_efer);
  VMP(v, gs.ia32_perf_global_ctrl);
  VMP(v, gs.pdpte0);
  VMP(v, gs.pdpte1);
  VMP(v, gs.pdpte2);
  VMP(v, gs.pdpte3);
  VMP(v, gs.cr0);
  VMP(v, gs.cr3);
  VMP(v, gs.cr4);
  VMP(v, gs.es_base);
  VMP(v, gs.cs_base);
  VMP(v, gs.ss_base);
  VMP(v, gs.ds_base);
  VMP(v, gs.fs_base);
  VMP(v, gs.gs_base);
  VMP(v, gs.ldtr_base);
  VMP(v, gs.tr_base);
  VMP(v, gs.gdtr_base);
  VMP(v, gs.idtr_base);
  VMP(v, gs.dr7);
  VMP(v, gs.rsp);
  VMP(v, gs.rip);
  VMP(v, gs.rflags);
  VMP(v, gs.pending_dbg_exceptions);
  VMP(v, gs.sysenter_esp);
  VMP(v, gs.sysenter_eip);
  VMP(v, gs.es_limit);
  VMP(v, gs.cs_limit);
  VMP(v, gs.ss_limit);
  VMP(v, gs.ds_limit);
  VMP(v, gs.fs_limit);
  VMP(v, gs.gs_limit);
  VMP(v, gs.ldtr_limit);
  VMP(v, gs.tr_limit);
  VMP(v, gs.gdtr_limit);
  VMP(v, gs.idtr_limit);
  VMP(v, gs.es_ar_bytes);
  VMP(v, gs.cs_ar_bytes);
  VMP(v, gs.ss_ar_bytes);
  VMP(v, gs.ds_ar_bytes);
  VMP(v, gs.fs_ar_bytes);
  VMP(v, gs.gs_ar_bytes);
  VMP(v, gs.ldtr_ar_bytes);
  VMP(v, gs.tr_ar_bytes);
  VMP(v, gs.interruptibility_info);
  VMP(v, gs.activity_state);
  VMP(v, gs.smbase);
  VMP(v, gs.ia32_sysenter_cs);
  VMP(v, gs.vmcs_link_pointer);
  VMP(v, gs.interrupt_status);
  VMP(v, gs.vmx_preemption_timer_value);
}

void vmcs_dump_hs(struct vmcs *v) {
  printk("Host state\n");
  VMP(v, hs.es_selector);
  VMP(v, hs.cs_selector);
  VMP(v, hs.ss_selector);
  VMP(v, hs.ds_selector);
  VMP(v, hs.fs_selector);
  VMP(v, hs.gs_selector);
  VMP(v, hs.tr_selector);
  VMP(v, hs.ia32_pat);
  VMP(v, hs.ia32_efer);
  VMP(v, hs.ia32_perf_global_ctrl);
  VMP(v, hs.ia32_sysenter_cs);
  VMP(v, hs.cr0);
  VMP(v, hs.cr3);
  VMP(v, hs.cr4);
  VMP(v, hs.fs_base);
  VMP(v, hs.gs_base);
  VMP(v, hs.tr_base);
  VMP(v, hs.gdtr_base);
  VMP(v, hs.idtr_base);
  VMP(v, hs.ia32_sysenter_esp);
  VMP(v, hs.ia32_sysenter_eip);
  VMP(v, hs.rsp);
  VMP(v, hs.rip);
}

void vmcs_dump_info(struct vmcs *v) {
  printk("VM exit info\n");
  VMP(v, info.guest_physical_address);
  VMP(v, info.vm_instruction_error);
  VMP(v, info.reason);
  VMP(v, info.intr_info);
  VMP(v, info.intr_error_code);
  VMP(v, info.idt_vectoring_info_field);
  VMP(v, info.idt_vectoring_error_code);
  VMP(v, info.instruction_len);
  VMP(v, info.vmx_instruction_info);
  VMP(v, info.qualification);
  VMP(v, info.io_rcx);
  VMP(v, info.io_rsi);
  VMP(v, info.io_rdi);
  VMP(v, info.io_rip);
  VMP(v, info.guest_linear_address);
}

void vmcs_dump(struct vmcs *v) {
  INFO("VMCS dump(0x%016X)\n", v);
  void vmcs_dump_ctrls(struct vmcs *v);
  void vmcs_dump_gs(struct vmcs *v);
  void vmcs_dump_hs(struct vmcs *v);
  void vmcs_dump_info(struct vmcs *v);
}

/**
 * Forced VMREAD every fields of the VMCS
 */
void vmcs_force_update(void) {
  // Execution controls
  VMRF(ctrls.ex.virtual_processor_id);
  VMRF(ctrls.ex.posted_int_notif_vector);
  VMRF(ctrls.ex.eptp_index);
  VMRF(ctrls.ex.io_bitmap_a);
  VMRF(ctrls.ex.io_bitmap_b);
  VMRF(ctrls.ex.msr_bitmap);
  VMRF(ctrls.ex.executive_vmcs_pointer);
  VMRF(ctrls.ex.tsc_offset);
  VMRF(ctrls.ex.virtual_apic_page_addr);
  VMRF(ctrls.ex.apic_access_addr);
  VMRF(ctrls.ex.posted_intr_desc_addr);
  VMRF(ctrls.ex.vm_function_controls);
  VMRF(ctrls.ex.ept_pointer);
  VMRF(ctrls.ex.eoi_exit_bitmap_0);
  VMRF(ctrls.ex.eoi_exit_bitmap_1);
  VMRF(ctrls.ex.eoi_exit_bitmap_2);
  VMRF(ctrls.ex.eoi_exit_bitmap_3);
  VMRF(ctrls.ex.eptp_list_addr);
  VMRF(ctrls.ex.vmread_bitmap_addr);
  VMRF(ctrls.ex.vmwrite_bitmap_addr);
  VMRF(ctrls.ex.virt_excep_info_addr);
  VMRF(ctrls.ex.xss_exiting_bitmap);
  VMRF(ctrls.ex.pin_based_vm_exec_control);
  VMRF(ctrls.ex.cpu_based_vm_exec_control);
  VMRF(ctrls.ex.exception_bitmap);
  VMRF(ctrls.ex.page_fault_error_code_mask);
  VMRF(ctrls.ex.page_fault_error_code_match);
  VMRF(ctrls.ex.cr3_target_count);
  VMRF(ctrls.ex.tpr_threshold);
  VMRF(ctrls.ex.secondary_vm_exec_control);
  VMRF(ctrls.ex.ple_gap);
  VMRF(ctrls.ex.ple_window);
  VMRF(ctrls.ex.cr0_guest_host_mask);
  VMRF(ctrls.ex.cr4_guest_host_mask);
  VMRF(ctrls.ex.cr0_read_shadow);
  VMRF(ctrls.ex.cr4_read_shadow);
  VMRF(ctrls.ex.cr3_target_value0);
  VMRF(ctrls.ex.cr3_target_value1);
  VMRF(ctrls.ex.cr3_target_value2);
  VMRF(ctrls.ex.cr3_target_value3);
  // Guest state
  VMRF(gs.es_selector);
  VMRF(gs.cs_selector);
  VMRF(gs.ss_selector);
  VMRF(gs.ds_selector);
  VMRF(gs.fs_selector);
  VMRF(gs.gs_selector);
  VMRF(gs.ldtr_selector);
  VMRF(gs.tr_selector);
  VMRF(gs.ia32_debugctl);
  VMRF(gs.ia32_pat);
  VMRF(gs.ia32_efer);
  VMRF(gs.ia32_perf_global_ctrl);
  VMRF(gs.pdpte0);
  VMRF(gs.pdpte1);
  VMRF(gs.pdpte2);
  VMRF(gs.pdpte3);
  VMRF(gs.cr0);
  VMRF(gs.cr3);
  VMRF(gs.cr4);
  VMRF(gs.es_base);
  VMRF(gs.cs_base);
  VMRF(gs.ss_base);
  VMRF(gs.ds_base);
  VMRF(gs.fs_base);
  VMRF(gs.gs_base);
  VMRF(gs.ldtr_base);
  VMRF(gs.tr_base);
  VMRF(gs.gdtr_base);
  VMRF(gs.idtr_base);
  VMRF(gs.dr7);
  VMRF(gs.rsp);
  VMRF(gs.rip);
  VMRF(gs.rflags);
  VMRF(gs.pending_dbg_exceptions);
  VMRF(gs.sysenter_esp);
  VMRF(gs.sysenter_eip);
  VMRF(gs.es_limit);
  VMRF(gs.cs_limit);
  VMRF(gs.ss_limit);
  VMRF(gs.ds_limit);
  VMRF(gs.fs_limit);
  VMRF(gs.gs_limit);
  VMRF(gs.ldtr_limit);
  VMRF(gs.tr_limit);
  VMRF(gs.gdtr_limit);
  VMRF(gs.idtr_limit);
  VMRF(gs.es_ar_bytes);
  VMRF(gs.cs_ar_bytes);
  VMRF(gs.ss_ar_bytes);
  VMRF(gs.ds_ar_bytes);
  VMRF(gs.fs_ar_bytes);
  VMRF(gs.gs_ar_bytes);
  VMRF(gs.ldtr_ar_bytes);
  VMRF(gs.tr_ar_bytes);
  VMRF(gs.interruptibility_info);
  VMRF(gs.activity_state);
  VMRF(gs.smbase);
  VMRF(gs.ia32_sysenter_cs);
  VMRF(gs.vmcs_link_pointer);
  VMRF(gs.interrupt_status);
  VMRF(gs.vmx_preemption_timer_value);
  // Host state
  VMRF(hs.es_selector);
  VMRF(hs.cs_selector);
  VMRF(hs.ss_selector);
  VMRF(hs.ds_selector);
  VMRF(hs.fs_selector);
  VMRF(hs.gs_selector);
  VMRF(hs.tr_selector);
  VMRF(hs.ia32_pat);
  VMRF(hs.ia32_efer);
  VMRF(hs.ia32_perf_global_ctrl);
  VMRF(hs.ia32_sysenter_cs);
  VMRF(hs.cr0);
  VMRF(hs.cr3);
  VMRF(hs.cr4);
  VMRF(hs.fs_base);
  VMRF(hs.gs_base);
  VMRF(hs.tr_base);
  VMRF(hs.gdtr_base);
  VMRF(hs.idtr_base);
  VMRF(hs.ia32_sysenter_esp);
  VMRF(hs.ia32_sysenter_eip);
  VMRF(hs.rsp);
  VMRF(hs.rip);
  // Vm exit controls
  VMRF(ctrls.exit.msr_store_addr);
  VMRF(ctrls.exit.msr_load_addr);
  VMRF(ctrls.exit.controls);
  VMRF(ctrls.exit.msr_store_count);
  VMRF(ctrls.exit.msr_load_count);
  // VM entry controls
  VMRF(ctrls.entry.msr_load_addr);
  VMRF(ctrls.entry.controls);
  VMRF(ctrls.entry.msr_load_count);
  VMRF(ctrls.entry.intr_info_field);
  VMRF(ctrls.entry.exception_error_code);
  VMRF(ctrls.entry.instruction_len);
  // VM exit info
  VMRF(info.guest_physical_address);
  VMRF(info.vm_instruction_error);
  VMRF(info.reason);
  VMRF(info.intr_info);
  VMRF(info.intr_error_code);
  VMRF(info.idt_vectoring_info_field);
  VMRF(info.idt_vectoring_error_code);
  VMRF(info.instruction_len);
  VMRF(info.vmx_instruction_info);
  VMRF(info.qualification);
  VMRF(info.io_rcx);
  VMRF(info.io_rsi);
  VMRF(info.io_rdi);
  VMRF(info.io_rip);
  VMRF(info.guest_linear_address);
}

void vmcs_collect_shadow(struct vmcs *gvmcs) {
  struct vmcs *bvmcs = vmcs;
  vmcs = gvmcs;
  // Guest state
  VMR(gs.es_selector);
  VMR(gs.cs_selector);
  VMR(gs.ss_selector);
  VMR(gs.ds_selector);
  VMR(gs.fs_selector);
  VMR(gs.gs_selector);
  VMR(gs.ldtr_selector);
  VMR(gs.tr_selector);
  VMR(gs.ia32_debugctl);
  VMR(gs.ia32_pat);
  VMR(gs.ia32_efer);
  VMR(gs.ia32_perf_global_ctrl);
  VMR(gs.pdpte0);
  VMR(gs.pdpte1);
  VMR(gs.pdpte2);
  VMR(gs.pdpte3);
  VMR(gs.cr0);
  VMR(gs.cr3);
  VMR(gs.cr4);
  VMR(gs.es_base);
  VMR(gs.cs_base);
  VMR(gs.ss_base);
  VMR(gs.ds_base);
  VMR(gs.fs_base);
  VMR(gs.gs_base);
  VMR(gs.ldtr_base);
  VMR(gs.tr_base);
  VMR(gs.gdtr_base);
  VMR(gs.idtr_base);
  VMR(gs.dr7);
  VMR(gs.rsp);
  VMR(gs.rip);
  VMR(gs.rflags);
  VMR(gs.pending_dbg_exceptions);
  VMR(gs.sysenter_esp);
  VMR(gs.sysenter_eip);
  VMR(gs.es_limit);
  VMR(gs.cs_limit);
  VMR(gs.ss_limit);
  VMR(gs.ds_limit);
  VMR(gs.fs_limit);
  VMR(gs.gs_limit);
  VMR(gs.ldtr_limit);
  VMR(gs.tr_limit);
  VMR(gs.gdtr_limit);
  VMR(gs.idtr_limit);
  VMR(gs.es_ar_bytes);
  VMR(gs.cs_ar_bytes);
  VMR(gs.ss_ar_bytes);
  VMR(gs.ds_ar_bytes);
  VMR(gs.fs_ar_bytes);
  VMR(gs.gs_ar_bytes);
  VMR(gs.ldtr_ar_bytes);
  VMR(gs.tr_ar_bytes);
  VMR(gs.interruptibility_info);
  VMR(gs.activity_state);
  VMR(gs.smbase);
  VMR(gs.ia32_sysenter_cs);
  VMR(gs.vmcs_link_pointer);
  VMR(gs.interrupt_status);
  // Other control fields
  VMR(ctrls.entry.controls);
  VMR(ctrls.entry.intr_info_field);
  VMR(ctrls.ex.cr0_read_shadow);
  VMR(ctrls.ex.cr4_read_shadow);
  VMR(ctrls.ex.vmread_bitmap_addr);
  VMR(ctrls.ex.vmwrite_bitmap_addr);
  vmcs = bvmcs;
}

/**
 * VMWRITE every modified field of the VMCS
 */
void vmcs_commit(void) {
  // Execution controls
  VMF(ctrls.ex.virtual_processor_id);
  VMF(ctrls.ex.posted_int_notif_vector);
  VMF(ctrls.ex.eptp_index);
  VMF(ctrls.ex.io_bitmap_a);
  VMF(ctrls.ex.io_bitmap_b);
  VMF(ctrls.ex.msr_bitmap);
  VMF(ctrls.ex.executive_vmcs_pointer);
  VMF(ctrls.ex.tsc_offset);
  VMF(ctrls.ex.virtual_apic_page_addr);
  VMF(ctrls.ex.apic_access_addr);
  VMF(ctrls.ex.posted_intr_desc_addr);
  VMF(ctrls.ex.vm_function_controls);
  VMF(ctrls.ex.ept_pointer);
  VMF(ctrls.ex.eoi_exit_bitmap_0);
  VMF(ctrls.ex.eoi_exit_bitmap_1);
  VMF(ctrls.ex.eoi_exit_bitmap_2);
  VMF(ctrls.ex.eoi_exit_bitmap_3);
  VMF(ctrls.ex.eptp_list_addr);
  VMF(ctrls.ex.vmread_bitmap_addr);
  VMF(ctrls.ex.vmwrite_bitmap_addr);
  VMF(ctrls.ex.virt_excep_info_addr);
  VMF(ctrls.ex.xss_exiting_bitmap);
  VMF(ctrls.ex.pin_based_vm_exec_control);
  VMF(ctrls.ex.cpu_based_vm_exec_control);
  VMF(ctrls.ex.exception_bitmap);
  VMF(ctrls.ex.page_fault_error_code_mask);
  VMF(ctrls.ex.page_fault_error_code_match);
  VMF(ctrls.ex.cr3_target_count);
  VMF(ctrls.ex.tpr_threshold);
  VMF(ctrls.ex.secondary_vm_exec_control);
  VMF(ctrls.ex.ple_gap);
  VMF(ctrls.ex.ple_window);
  VMF(ctrls.ex.cr0_guest_host_mask);
  VMF(ctrls.ex.cr4_guest_host_mask);
  VMF(ctrls.ex.cr0_read_shadow);
  VMF(ctrls.ex.cr4_read_shadow);
  VMF(ctrls.ex.cr3_target_value0);
  VMF(ctrls.ex.cr3_target_value1);
  VMF(ctrls.ex.cr3_target_value2);
  VMF(ctrls.ex.cr3_target_value3);
  // Guest state
  VMF(gs.es_selector);
  VMF(gs.cs_selector);
  VMF(gs.ss_selector);
  VMF(gs.ds_selector);
  VMF(gs.fs_selector);
  VMF(gs.gs_selector);
  VMF(gs.ldtr_selector);
  VMF(gs.tr_selector);
  VMF(gs.ia32_debugctl);
  VMF(gs.ia32_pat);
  VMF(gs.ia32_efer);
  VMF(gs.ia32_perf_global_ctrl);
  VMF(gs.pdpte0);
  VMF(gs.pdpte1);
  VMF(gs.pdpte2);
  VMF(gs.pdpte3);
  VMF(gs.cr0);
  VMF(gs.cr3);
  VMF(gs.cr4);
  VMF(gs.es_base);
  VMF(gs.cs_base);
  VMF(gs.ss_base);
  VMF(gs.ds_base);
  VMF(gs.fs_base);
  VMF(gs.gs_base);
  VMF(gs.ldtr_base);
  VMF(gs.tr_base);
  VMF(gs.gdtr_base);
  VMF(gs.idtr_base);
  VMF(gs.dr7);
  VMF(gs.rsp);
  VMF(gs.rip);
  VMF(gs.rflags);
  VMF(gs.pending_dbg_exceptions);
  VMF(gs.sysenter_esp);
  VMF(gs.sysenter_eip);
  VMF(gs.es_limit);
  VMF(gs.cs_limit);
  VMF(gs.ss_limit);
  VMF(gs.ds_limit);
  VMF(gs.fs_limit);
  VMF(gs.gs_limit);
  VMF(gs.ldtr_limit);
  VMF(gs.tr_limit);
  VMF(gs.gdtr_limit);
  VMF(gs.idtr_limit);
  VMF(gs.es_ar_bytes);
  VMF(gs.cs_ar_bytes);
  VMF(gs.ss_ar_bytes);
  VMF(gs.ds_ar_bytes);
  VMF(gs.fs_ar_bytes);
  VMF(gs.gs_ar_bytes);
  VMF(gs.ldtr_ar_bytes);
  VMF(gs.tr_ar_bytes);
  VMF(gs.interruptibility_info);
  VMF(gs.activity_state);
  VMF(gs.smbase);
  VMF(gs.ia32_sysenter_cs);
  VMF(gs.vmcs_link_pointer);
  VMF(gs.interrupt_status);
  VMF(gs.vmx_preemption_timer_value);
  // Host state
  VMF(hs.es_selector);
  VMF(hs.cs_selector);
  VMF(hs.ss_selector);
  VMF(hs.ds_selector);
  VMF(hs.fs_selector);
  VMF(hs.gs_selector);
  VMF(hs.tr_selector);
  VMF(hs.ia32_pat);
  VMF(hs.ia32_efer);
  VMF(hs.ia32_perf_global_ctrl);
  VMF(hs.ia32_sysenter_cs);
  VMF(hs.cr0);
  VMF(hs.cr3);
  VMF(hs.cr4);
  VMF(hs.fs_base);
  VMF(hs.gs_base);
  VMF(hs.tr_base);
  VMF(hs.gdtr_base);
  VMF(hs.idtr_base);
  VMF(hs.ia32_sysenter_esp);
  VMF(hs.ia32_sysenter_eip);
  VMF(hs.rsp);
  VMF(hs.rip);
  // Vm exit controls
  VMF(ctrls.exit.msr_store_addr);
  VMF(ctrls.exit.msr_load_addr);
  VMF(ctrls.exit.controls);
  VMF(ctrls.exit.msr_store_count);
  VMF(ctrls.exit.msr_load_count);
  // VM entry controls
  VMF(ctrls.entry.msr_load_addr);
  VMF(ctrls.entry.controls);
  VMF(ctrls.entry.msr_load_count);
  VMF(ctrls.entry.intr_info_field);
  VMF(ctrls.entry.exception_error_code);
  VMF(ctrls.entry.instruction_len);
  // VM exit info
  VMF(info.guest_physical_address);
  VMF(info.vm_instruction_error);
  VMF(info.reason);
  VMF(info.intr_info);
  VMF(info.intr_error_code);
  VMF(info.idt_vectoring_info_field);
  VMF(info.idt_vectoring_error_code);
  VMF(info.instruction_len);
  VMF(info.vmx_instruction_info);
  VMF(info.qualification);
  VMF(info.io_rcx);
  VMF(info.io_rsi);
  VMF(info.io_rdi);
  VMF(info.io_rip);
  VMF(info.guest_linear_address);
}

void vmcs_encoding_init_all(void) {
  uint32_t i;
  for (i = 0; i < VM_NB; i++) {
    vmcs_encoding_init(&vmcs_cache_pool[i]);
  }
}

void vmcs_encoding_init(struct vmcs *v) {
  memset(v, 0, sizeof(struct vmcs));
  // Execution controls
  VMCSE(v, ctrls.ex.virtual_processor_id, VIRTUAL_PROCESSOR_ID);
  VMCSE(v, ctrls.ex.posted_int_notif_vector, POSTED_INT_NOTIF_VECTOR);
  VMCSE(v, ctrls.ex.eptp_index, EPTP_INDEX);
  VMCSE(v, ctrls.ex.io_bitmap_a, IO_BITMAP_A);
  VMCSE(v, ctrls.ex.io_bitmap_b, IO_BITMAP_B);
  VMCSE(v, ctrls.ex.msr_bitmap, MSR_BITMAP);
  VMCSE(v, ctrls.ex.executive_vmcs_pointer, EXECUTIVE_VMCS_POINTER);
  VMCSE(v, ctrls.ex.tsc_offset, TSC_OFFSET);
  VMCSE(v, ctrls.ex.virtual_apic_page_addr, VIRTUAL_APIC_PAGE_ADDR);
  VMCSE(v, ctrls.ex.apic_access_addr, APIC_ACCESS_ADDR);
  VMCSE(v, ctrls.ex.posted_intr_desc_addr, POSTED_INTR_DESC_ADDR);
  VMCSE(v, ctrls.ex.vm_function_controls, VM_FUNCTION_CONTROLS);
  VMCSE(v, ctrls.ex.ept_pointer, EPT_POINTER);
  VMCSE(v, ctrls.ex.eoi_exit_bitmap_0, EOI_EXIT_BITMAP_0);
  VMCSE(v, ctrls.ex.eoi_exit_bitmap_1, EOI_EXIT_BITMAP_1);
  VMCSE(v, ctrls.ex.eoi_exit_bitmap_2, EOI_EXIT_BITMAP_2);
  VMCSE(v, ctrls.ex.eoi_exit_bitmap_3, EOI_EXIT_BITMAP_3);
  VMCSE(v, ctrls.ex.eptp_list_addr, EPTP_LIST_ADDR);
  VMCSE(v, ctrls.ex.vmread_bitmap_addr, VMREAD_BITMAP_ADDR);
  VMCSE(v, ctrls.ex.vmwrite_bitmap_addr, VMWRITE_BITMAP_ADDR);
  VMCSE(v, ctrls.ex.virt_excep_info_addr, VIRT_EXCEP_INFO_ADDR);
  VMCSE(v, ctrls.ex.xss_exiting_bitmap, XSS_EXITING_BITMAP);
  VMCSE(v, ctrls.ex.pin_based_vm_exec_control, PIN_BASED_VM_EXEC_CONTROL);
  VMCSE(v, ctrls.ex.cpu_based_vm_exec_control, CPU_BASED_VM_EXEC_CONTROL);
  VMCSE(v, ctrls.ex.exception_bitmap, EXCEPTION_BITMAP);
  VMCSE(v, ctrls.ex.page_fault_error_code_mask, PAGE_FAULT_ERROR_CODE_MASK);
  VMCSE(v, ctrls.ex.page_fault_error_code_match, PAGE_FAULT_ERROR_CODE_MATCH);
  VMCSE(v, ctrls.ex.cr3_target_count, CR3_TARGET_COUNT);
  VMCSE(v, ctrls.ex.tpr_threshold, TPR_THRESHOLD);
  VMCSE(v, ctrls.ex.secondary_vm_exec_control, SECONDARY_VM_EXEC_CONTROL);
  VMCSE(v, ctrls.ex.ple_gap, PLE_GAP);
  VMCSE(v, ctrls.ex.ple_window, PLE_WINDOW);
  VMCSE(v, ctrls.ex.cr0_guest_host_mask, CR0_GUEST_HOST_MASK);
  VMCSE(v, ctrls.ex.cr4_guest_host_mask, CR4_GUEST_HOST_MASK);
  VMCSE(v, ctrls.ex.cr0_read_shadow, CR0_READ_SHADOW);
  VMCSE(v, ctrls.ex.cr4_read_shadow, CR4_READ_SHADOW);
  VMCSE(v, ctrls.ex.cr3_target_value0, CR3_TARGET_VALUE0);
  VMCSE(v, ctrls.ex.cr3_target_value1, CR3_TARGET_VALUE1);
  VMCSE(v, ctrls.ex.cr3_target_value2, CR3_TARGET_VALUE2);
  VMCSE(v, ctrls.ex.cr3_target_value3, CR3_TARGET_VALUE3);
  // Guest state
  VMCSE(v, gs.es_selector, GUEST_ES_SELECTOR);
  VMCSE(v, gs.cs_selector, GUEST_CS_SELECTOR);
  VMCSE(v, gs.ss_selector, GUEST_SS_SELECTOR);
  VMCSE(v, gs.ds_selector, GUEST_DS_SELECTOR);
  VMCSE(v, gs.fs_selector, GUEST_FS_SELECTOR);
  VMCSE(v, gs.gs_selector, GUEST_GS_SELECTOR);
  VMCSE(v, gs.ldtr_selector, GUEST_LDTR_SELECTOR);
  VMCSE(v, gs.tr_selector, GUEST_TR_SELECTOR);
  VMCSE(v, gs.ia32_debugctl, GUEST_IA32_DEBUGCTL);
  VMCSE(v, gs.ia32_pat, GUEST_IA32_PAT);
  VMCSE(v, gs.ia32_efer, GUEST_IA32_EFER);
  VMCSE(v, gs.ia32_perf_global_ctrl, GUEST_IA32_PERF_GLOBAL_CTRL);
  VMCSE(v, gs.pdpte0, GUEST_PDPTR0);
  VMCSE(v, gs.pdpte1, GUEST_PDPTR1);
  VMCSE(v, gs.pdpte2, GUEST_PDPTR2);
  VMCSE(v, gs.pdpte3, GUEST_PDPTR3);
  VMCSE(v, gs.cr0, GUEST_CR0);
  VMCSE(v, gs.cr3, GUEST_CR3);
  VMCSE(v, gs.cr4, GUEST_CR4);
  VMCSE(v, gs.es_base, GUEST_ES_BASE);
  VMCSE(v, gs.cs_base, GUEST_CS_BASE);
  VMCSE(v, gs.ss_base, GUEST_SS_BASE);
  VMCSE(v, gs.ds_base, GUEST_DS_BASE);
  VMCSE(v, gs.fs_base, GUEST_FS_BASE);
  VMCSE(v, gs.gs_base, GUEST_GS_BASE);
  VMCSE(v, gs.ldtr_base, GUEST_LDTR_BASE);
  VMCSE(v, gs.tr_base, GUEST_TR_BASE);
  VMCSE(v, gs.gdtr_base, GUEST_GDTR_BASE);
  VMCSE(v, gs.idtr_base, GUEST_IDTR_BASE);
  VMCSE(v, gs.dr7, GUEST_DR7);
  VMCSE(v, gs.rsp, GUEST_RSP);
  VMCSE(v, gs.rip, GUEST_RIP);
  VMCSE(v, gs.rflags, GUEST_RFLAGS);
  VMCSE(v, gs.pending_dbg_exceptions, GUEST_PENDING_DBG_EXCEPTIONS);
  VMCSE(v, gs.sysenter_esp, GUEST_SYSENTER_ESP);
  VMCSE(v, gs.sysenter_eip, GUEST_SYSENTER_EIP);
  VMCSE(v, gs.es_limit, GUEST_ES_LIMIT);
  VMCSE(v, gs.cs_limit, GUEST_CS_LIMIT);
  VMCSE(v, gs.ss_limit, GUEST_SS_LIMIT);
  VMCSE(v, gs.ds_limit, GUEST_DS_LIMIT);
  VMCSE(v, gs.fs_limit, GUEST_FS_LIMIT);
  VMCSE(v, gs.gs_limit, GUEST_GS_LIMIT);
  VMCSE(v, gs.ldtr_limit, GUEST_LDTR_LIMIT);
  VMCSE(v, gs.tr_limit, GUEST_TR_LIMIT);
  VMCSE(v, gs.gdtr_limit, GUEST_GDTR_LIMIT);
  VMCSE(v, gs.idtr_limit, GUEST_IDTR_LIMIT);
  VMCSE(v, gs.es_ar_bytes, GUEST_ES_AR_BYTES);
  VMCSE(v, gs.cs_ar_bytes, GUEST_CS_AR_BYTES);
  VMCSE(v, gs.ss_ar_bytes, GUEST_SS_AR_BYTES);
  VMCSE(v, gs.ds_ar_bytes, GUEST_DS_AR_BYTES);
  VMCSE(v, gs.fs_ar_bytes, GUEST_FS_AR_BYTES);
  VMCSE(v, gs.gs_ar_bytes, GUEST_GS_AR_BYTES);
  VMCSE(v, gs.ldtr_ar_bytes, GUEST_LDTR_AR_BYTES);
  VMCSE(v, gs.tr_ar_bytes, GUEST_TR_AR_BYTES);
  VMCSE(v, gs.interruptibility_info, GUEST_INTERRUPTIBILITY_INFO);
  VMCSE(v, gs.activity_state, GUEST_ACTIVITY_STATE);
  VMCSE(v, gs.smbase, GUEST_SMBASE);
  VMCSE(v, gs.ia32_sysenter_cs, GUEST_SYSENTER_CS);
  VMCSE(v, gs.vmcs_link_pointer, VMCS_LINK_POINTER);
  VMCSE(v, gs.interrupt_status, GUEST_INTERRUPT_STATUS);
  VMCSE(v, gs.vmx_preemption_timer_value, VMX_PREEMPTION_TIMER_VALUE);
  // Host state
  VMCSE(v, hs.es_selector, HOST_ES_SELECTOR);
  VMCSE(v, hs.cs_selector, HOST_CS_SELECTOR);
  VMCSE(v, hs.ss_selector, HOST_SS_SELECTOR);
  VMCSE(v, hs.ds_selector, HOST_DS_SELECTOR);
  VMCSE(v, hs.fs_selector, HOST_FS_SELECTOR);
  VMCSE(v, hs.gs_selector, HOST_GS_SELECTOR);
  VMCSE(v, hs.tr_selector, HOST_TR_SELECTOR);
  VMCSE(v, hs.ia32_pat, HOST_IA32_PAT);
  VMCSE(v, hs.ia32_efer, HOST_IA32_EFER);
  VMCSE(v, hs.ia32_perf_global_ctrl, HOST_IA32_PERF_GLOBAL_CTRL);
  VMCSE(v, hs.ia32_sysenter_cs, HOST_IA32_SYSENTER_CS);
  VMCSE(v, hs.cr0, HOST_CR0);
  VMCSE(v, hs.cr3, HOST_CR3);
  VMCSE(v, hs.cr4, HOST_CR4);
  VMCSE(v, hs.fs_base, HOST_FS_BASE);
  VMCSE(v, hs.gs_base, HOST_GS_BASE);
  VMCSE(v, hs.tr_base, HOST_TR_BASE);
  VMCSE(v, hs.gdtr_base, HOST_GDTR_BASE);
  VMCSE(v, hs.idtr_base, HOST_IDTR_BASE);
  VMCSE(v, hs.ia32_sysenter_esp, HOST_IA32_SYSENTER_ESP);
  VMCSE(v, hs.ia32_sysenter_eip, HOST_IA32_SYSENTER_EIP);
  VMCSE(v, hs.rsp, HOST_RSP);
  VMCSE(v, hs.rip, HOST_RIP);
  // Vm exit controls
  VMCSE(v, ctrls.exit.msr_store_addr, VM_EXIT_MSR_STORE_ADDR);
  VMCSE(v, ctrls.exit.msr_load_addr, VM_EXIT_MSR_LOAD_ADDR);
  VMCSE(v, ctrls.exit.controls, VM_EXIT_CONTROLS);
  VMCSE(v, ctrls.exit.msr_store_count, VM_EXIT_MSR_STORE_COUNT);
  VMCSE(v, ctrls.exit.msr_load_count, VM_EXIT_MSR_LOAD_COUNT);
  // VM entry controls
  VMCSE(v, ctrls.entry.msr_load_addr, VM_ENTRY_MSR_LOAD_ADDR);
  VMCSE(v, ctrls.entry.controls, VM_ENTRY_CONTROLS);
  VMCSE(v, ctrls.entry.msr_load_count, VM_ENTRY_MSR_LOAD_COUNT);
  VMCSE(v, ctrls.entry.intr_info_field, VM_ENTRY_INTR_INFO_FIELD);
  VMCSE(v, ctrls.entry.exception_error_code, VM_ENTRY_EXCEPTION_ERROR_CODE);
  VMCSE(v, ctrls.entry.instruction_len, VM_ENTRY_INSTRUCTION_LEN);
  // VM exit info
  VMCSE(v, info.guest_physical_address, GUEST_PHYSICAL_ADDRESS);
  VMCSE(v, info.vm_instruction_error, VM_INSTRUCTION_ERROR);
  VMCSE(v, info.reason, VM_EXIT_REASON);
  VMCSE(v, info.intr_info, VM_EXIT_INTR_INFO);
  VMCSE(v, info.intr_error_code, VM_EXIT_INTR_ERROR_CODE);
  VMCSE(v, info.idt_vectoring_info_field, IDT_VECTORING_INFO_FIELD);
  VMCSE(v, info.idt_vectoring_error_code, IDT_VECTORING_ERROR_CODE);
  VMCSE(v, info.instruction_len, VM_EXIT_INSTRUCTION_LEN);
  VMCSE(v, info.vmx_instruction_info, VMX_INSTRUCTION_INFO);
  VMCSE(v, info.qualification, EXIT_QUALIFICATION);
  VMCSE(v, info.io_rcx, IO_RCX);
  VMCSE(v, info.io_rsi, IO_RSI);
  VMCSE(v, info.io_rdi, IO_RDI);
  VMCSE(v, info.io_rip, IO_RIP);
  VMCSE(v, info.guest_linear_address, GUEST_LINEAR_ADDRESS);
}

void vmcs_host_config_host_state_fields(void) {
  struct segment_descriptor *dsc;
  struct idt_ptr idt_ptr;

  VMW(hs.cr0, cpu_adjust64(cpu_read_cr0(), MSR_ADDRESS_VMX_CR0_FIXED0,
        MSR_ADDRESS_VMX_CR0_FIXED1));
  // XXX TODO duplicate UEFI PAGE TABLES
  VMW(hs.cr3, paging_get_host_cr3());
  // TODO Bit 18 OSXSAVE Activation du XCR0 -> proxifier le XSETBV
  VMW(hs.cr4, cpu_adjust64(cpu_read_cr4() | (1 << 18),
        MSR_ADDRESS_VMX_CR4_FIXED0, MSR_ADDRESS_VMX_CR4_FIXED1));
  VMW(hs.rsp, (uint64_t)&vmm_stack[VMM_STACK_SIZE]);
  DBG("VMM stack @0x%016X\n", (uint64_t)&vmm_stack[VMM_STACK_SIZE]);
  VMW(hs.rip, (uint64_t)vmm_vm_exit_handler);

  VMW(hs.cs_selector, cpu_read_cs() & 0xf8);
  VMW(hs.ss_selector, cpu_read_ss() & 0xf8);
  VMW(hs.ds_selector, cpu_read_ds() & 0xf8);
  VMW(hs.es_selector, cpu_read_es() & 0xf8);
  VMW(hs.fs_selector, cpu_read_fs() & 0xf8);
  VMW(hs.gs_selector, cpu_read_gs() & 0xf8);
  VMW(hs.tr_selector, 0x8 & 0xf8);

  gdt_get_host_entry(cpu_read_fs(), &dsc);
  VMW(hs.fs_base, gdt_descriptor_get_base(dsc));
  gdt_get_host_entry(cpu_read_gs(), &dsc);
  VMW(hs.gs_base, gdt_descriptor_get_base(dsc));
  gdt_get_host_entry(0x8, &dsc);
  VMW(hs.tr_base, gdt_descriptor_get_base(dsc));

  VMW(hs.gdtr_base, gdt_get_host_base());

  idt_get_idt_ptr(&idt_ptr);
  VMW(hs.idtr_base, idt_ptr.base);

  VMW(hs.ia32_sysenter_cs, msr_read(MSR_ADDRESS_IA32_SYSENTER_CS));
  VMW(hs.ia32_sysenter_esp, msr_read(MSR_ADDRESS_IA32_SYSENTER_ESP));
  VMW(hs.ia32_sysenter_eip, msr_read(MSR_ADDRESS_IA32_SYSENTER_EIP));
  VMW(hs.ia32_efer, msr_read(MSR_ADDRESS_IA32_EFER));

  VMW(hs.ia32_perf_global_ctrl, 0);
}

void vmcs_host_config_vm_exec_control_fields(void) {
  uint32_t procbased_ctls = ACT_SECONDARY_CONTROLS | USE_MSR_BITMAPS |
      USE_IO_BITMAPS | USE_TSC_OFFSETTING;
  uint32_t procbased_ctls_2 = ENABLE_EPT | ENABLE_VPID | UNRESTRICTED_GUEST |
      ENABLE_RDTSCP;

  uint64_t eptp;
	//XXX
  uint32_t pinbased_ctls = 0;//ACT_VMX_PREEMPT_TIMER; // | NMI_EXITING;

  VMW(ctrls.ex.pin_based_vm_exec_control,
      cpu_adjust32(pinbased_ctls, MSR_ADDRESS_IA32_VMX_PINBASED_CTLS));

  procbased_ctls = cpu_adjust32(procbased_ctls,
      MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS);
  // extra capabilities enabled
  if (((msr_read(MSR_ADDRESS_IA32_VMX_BASIC) >> 55) & 1)
      // CR3_LOAD_EXITING & CR3_STORE_EXITING can be disabled
      && (((msr_read(MSR_ADDRESS_IA32_VMX_TRUE_PROCBASED_CTLS) >> 15) & 3)) ==
      0){
    procbased_ctls &= ~(CR3_LOAD_EXITING | CR3_STORE_EXITING);
  } else {
    panic("#!PROCBASED_CTLS CR3_LOAD_EXITING or CR3_STORE_EXITING required\n");
  }
  VMW(ctrls.ex.cpu_based_vm_exec_control, procbased_ctls);

  VMW(ctrls.ex.secondary_vm_exec_control,
      cpu_adjust32(procbased_ctls_2, MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS2));

  VMW(ctrls.ex.exception_bitmap, 0);
  VMW(ctrls.ex.page_fault_error_code_mask, 0);
  VMW(ctrls.ex.page_fault_error_code_match, 0);

  VMW(ctrls.ex.tsc_offset, 0);

  // As we are using UNRESTRICTED_GUEST procbased_ctrl, the guest can itself
  // modify CR0.PE and CR0.PG, see doc INTEL vol 3C chap 23.8
  VMW(ctrls.ex.cr0_guest_host_mask, (msr_read(MSR_ADDRESS_VMX_CR0_FIXED0) |
        (~msr_read(MSR_ADDRESS_VMX_CR0_FIXED1))) & ~(0x80000001));
  VMW(ctrls.ex.cr0_read_shadow, cpu_read_cr0());
  VMW(ctrls.ex.cr4_guest_host_mask, msr_read(MSR_ADDRESS_VMX_CR4_FIXED0) |
      ~msr_read(MSR_ADDRESS_VMX_CR4_FIXED1));
  VMW(ctrls.ex.cr4_read_shadow, cpu_read_cr4() &
      ~(uint64_t)(1<<13)); // We hide CR4.VMXE

  VMW(ctrls.ex.cr3_target_count, 0);
  VMW(ctrls.ex.cr3_target_value0, 0);
  VMW(ctrls.ex.cr3_target_value1, 0);
  VMW(ctrls.ex.cr3_target_value2, 0);
  VMW(ctrls.ex.cr3_target_value3, 0);

  eptp = ept_get_eptp();
  VMW(ctrls.ex.ept_pointer, eptp);
  VMW(ctrls.ex.virtual_processor_id, 0x1); // vmcs0 is 1
}

void vmcs_host_config_guest_state_fields() {
  struct segment_descriptor *dsc;
  struct idt_ptr idt_ptr;
  uint64_t msr;
  uint64_t sel;

  VMW(gs.cr0, cpu_adjust64(cpu_read_cr0(), MSR_ADDRESS_VMX_CR0_FIXED0,
        MSR_ADDRESS_VMX_CR0_FIXED1));
  VMW(gs.cr3, cpu_read_cr3());
  VMW(gs.cr4, cpu_adjust64(cpu_read_cr4(), MSR_ADDRESS_VMX_CR4_FIXED0,
        MSR_ADDRESS_VMX_CR4_FIXED1));
  VMW(gs.dr7, cpu_read_dr7());
  VMW(gs.rflags, (1 << 1));

  sel = cpu_read_cs();
  gdt_get_guest_entry(sel, &dsc);
  VMW(gs.cs_selector, sel);
  VMW(gs.cs_base, gdt_descriptor_get_base(dsc));
  VMW(gs.cs_limit, gdt_descriptor_get_limit(dsc));
  VMW(gs.cs_ar_bytes, (dsc->granularity << 12) | (dsc->access_rights << 4) |
      (dsc->type << 0));
  sel = cpu_read_ss();
  gdt_get_guest_entry(sel, &dsc);
  VMW(gs.ss_selector, sel);
  VMW(gs.ss_base, gdt_descriptor_get_base(dsc));
  VMW(gs.ss_limit, gdt_descriptor_get_limit(dsc));
  VMW(gs.ss_ar_bytes, (dsc->granularity << 12) | (dsc->access_rights << 4) |
      (dsc->type << 0));
  sel = cpu_read_ds();
  gdt_get_guest_entry(sel, &dsc);
  VMW(gs.ds_selector, sel);
  VMW(gs.ds_base, gdt_descriptor_get_base(dsc));
  VMW(gs.ds_limit, gdt_descriptor_get_limit(dsc));
  VMW(gs.ds_ar_bytes, (dsc->granularity << 12) | (dsc->access_rights << 4) |
      (dsc->type << 0));
  sel = cpu_read_es();
  gdt_get_guest_entry(sel, &dsc);
  VMW(gs.es_selector, sel);
  VMW(gs.es_base, gdt_descriptor_get_base(dsc));
  VMW(gs.es_limit, gdt_descriptor_get_limit(dsc));
  VMW(gs.es_ar_bytes, (dsc->granularity << 12) | (dsc->access_rights << 4) |
      (dsc->type << 0));
  sel = cpu_read_fs();
  gdt_get_guest_entry(sel, &dsc);
  VMW(gs.fs_selector, sel);
  VMW(gs.fs_base, gdt_descriptor_get_base(dsc));
  VMW(gs.fs_limit, gdt_descriptor_get_limit(dsc));
  VMW(gs.fs_ar_bytes, (dsc->granularity << 12) | (dsc->access_rights << 4) |
      (dsc->type << 0));
  sel = cpu_read_gs();
  gdt_get_guest_entry(sel, &dsc);
  VMW(gs.gs_selector, sel);
  VMW(gs.gs_base, gdt_descriptor_get_base(dsc));
  VMW(gs.gs_limit, gdt_descriptor_get_limit(dsc));
  VMW(gs.gs_ar_bytes, (dsc->granularity << 12) | (dsc->access_rights << 4) |
      (dsc->type << 0));

  /* TODO Verify if these values are correct. */
  VMW(gs.ldtr_selector, 0);
  VMW(gs.ldtr_base, 0);
  VMW(gs.ldtr_limit, 0xffff);
  VMW(gs.ldtr_ar_bytes, 0x82);
  VMW(gs.tr_selector, 0);
  VMW(gs.tr_base, 0);
  VMW(gs.tr_limit, 0xffff);
  VMW(gs.tr_ar_bytes, 0x8b);

  VMW(gs.gdtr_base, gdt_get_host_base());
  VMW(gs.gdtr_limit, gdt_get_host_limit());

	// TODO remplacer la copie de l'idt courante par celle sauvegardée dans le
	// fichier idt.c pour le guest
	//cpu_read_idt(&idt_ptr);
	idt_get_guest_idt_ptr(&idt_ptr);
	idt_dump(&idt_ptr);
	VMW(gs.idtr_base, idt_ptr.base);
  VMW(gs.idtr_limit, idt_ptr.limit);

  VMW(gs.ia32_debugctl, msr_read(MSR_ADDRESS_IA32_DEBUGCTL));
  VMW(gs.ia32_sysenter_cs, msr_read(MSR_ADDRESS_IA32_SYSENTER_CS));

  msr = msr_read(MSR_ADDRESS_IA32_EFER);
  VMW(gs.ia32_efer, msr);

	VMW(gs.activity_state, 0);
  VMW(gs.interruptibility_info, 0);
  VMW(gs.pending_dbg_exceptions, 0);
  VMW(gs.vmcs_link_pointer, 0xffffffffffffffff);

  VMW(gs.ia32_perf_global_ctrl, msr_read(MSR_ADDRESS_IA32_PERF_GLOBAL_CTRL));

	//INFO("%X\n", msr_read(MSR_ADDRESS_MSR_PLATFORM_INFO));
  // Init and compute vmx_preemption_timer_value
  //XXX
	/*tsc_freq_MHz = ((msr_read(MSR_ADDRESS_MSR_PLATFORM_INFO) >> 8) & 0xff) * 100;
	tsc_divider = msr_read(MSR_ADDRESS_IA32_VMX_MISC) & 0x7;
  vmcs_set_vmx_preemption_timer_value(hc,
      VMCS_DEFAULT_PREEMPTION_TIMER_MICROSEC);
	*/
}

void vmcs_host_config_vm_exit_control_fields(void) {
  uint32_t exit_controls = EXIT_SAVE_IA32_EFER | EXIT_LOAD_IA32_EFER |
    EXIT_LOAD_IA32_PERF_GLOBAL_CTRL | HOST_ADDR_SPACE_SIZE |
    SAVE_VMX_PREEMPT_TIMER_VAL;
  VMW(ctrls.exit.controls, cpu_adjust32(exit_controls,
        MSR_ADDRESS_IA32_VMX_EXIT_CTLS));
  VMW(ctrls.exit.msr_store_count, 0);
  VMW(ctrls.exit.msr_load_count, 0);
}

void vmcs_host_config_vm_entry_control_fields(void) {
  uint32_t entry_controls = ENTRY_LOAD_IA32_EFER |
    ENTRY_LOAD_IA32_PERF_GLOBAL_CTRL | IA32E_MODE_GUEST;
	VMW(ctrls.entry.controls, cpu_adjust32(entry_controls,
        MSR_ADDRESS_IA32_VMX_ENTRY_CTLS));
  VMW(ctrls.entry.msr_load_count, 0);
  VMW(ctrls.entry.intr_info_field, 0);
}

void vmcs_host_config_configure(void) {
  uint32_t vmcs_revision_identifier;
  uint32_t number_bytes_regions;
  uint64_t msr_value = msr_read(MSR_ADDRESS_IA32_VMX_BASIC);
  vmcs_revision_identifier = msr_value & 0xffffffff;
  number_bytes_regions = (msr_value >> 32) & 0x1fff;
  if (0x1000 < number_bytes_regions) {
    panic("!#SETUP SZ");
  }
  hc->revision_id = vmcs_revision_identifier;
  // Host configuration
  vmcs = hc; // We set the current vmcs for the VMW macro
  vmcs_host_config_host_state_fields();

  vmcs_host_config_guest_state_fields();

  vmcs_host_config_vm_exec_control_fields();

  vmcs_host_config_vm_exit_control_fields();

  vmcs_host_config_vm_entry_control_fields();
}

void vmcs_init(void) {
  uint32_t i;
  // Allocate VMCS cache for host configuration
  hc = efi_allocate_pool(sizeof(struct vmcs));
  vmcs_cache_pool = efi_allocate_pool(sizeof(struct vmcs) * VM_NB);
  vmcs_region_pool = efi_allocate_pages(VM_NB);
  vmxon = efi_allocate_pages(1);
  // Initialize VMCS region pool
  memset(&vmcs_region_pool[0], 0, VM_NB * 0x1000);
  // Initialize VMCS cache pool
  memset(&vmcs_cache_pool[0], 0, VM_NB * sizeof(struct vmcs));
  // Initialize the encodings
  INFO("Initializing encodings\n");
  vmcs_encoding_init(hc);
  // Copying the encodings
  for (i = 0; i < VM_NB; i++) {
    memcpy(&vmcs_cache_pool[i], hc, sizeof(struct vmcs));
  }
  INFO("Fills l0 host state and execution controls\n");
  vmcs_host_config_configure();
}

void vmcs_set_vmx_preemption_timer_value(struct vmcs *v, uint64_t time_microsec) {
  VMW(gs.vmx_preemption_timer_value, (tsc_freq_MHz * time_microsec) >> tsc_divider);

}
