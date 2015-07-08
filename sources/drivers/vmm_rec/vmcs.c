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
uint8_t vmcs0[4096] __attribute((aligned(0x1000)));
uint8_t vapic[4096] __attribute((aligned(0x1000)));

/**
 * Host vmcs configuration reference
 */
struct vmcs *hc;

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

void vmcs_dump(struct vmcs *v) {
  INFO("VMCS dump(0x%016X)\n", v);
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
  VMP(v, ctrls.ex.cpu_based_vm_exec_control);
  VMP(v, ctrls.ex.exception_bitmap);
  VMP(v, ctrls.ex.page_fault_error_code_mask);
  VMP(v, ctrls.ex.page_fault_error_code_match);
  VMP(v, ctrls.ex.cr3_target_count);
  VMP(v, ctrls.ex.tpr_threshold);
  VMP(v, ctrls.ex.secondary_vm_exec_control);
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
  VMP(v, gs.pdptr0);
  VMP(v, gs.pdptr1);
  VMP(v, gs.pdptr2);
  VMP(v, gs.pdptr3);
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
  VMP(v, gs.sysenter_cs);
  VMP(v, gs.vmcs_link_pointer);
  VMP(v, gs.interrupt_status);
  VMP(v, gs.vmx_preemption_timer_value);
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
  printk("Vm exit controls\n");
  VMP(v, ctrls.exit.msr_store_addr);
  VMP(v, ctrls.exit.msr_load_addr);
  VMP(v, ctrls.exit.controls);
  VMP(v, ctrls.exit.msr_store_count);
  VMP(v, ctrls.exit.msr_load_count);
  printk("VM entry controls\n");
  VMP(v, ctrls.entry.msr_load_addr);
  VMP(v, ctrls.entry.controls);
  VMP(v, ctrls.entry.msr_load_count);
  VMP(v, ctrls.entry.intr_info_field);
  VMP(v, ctrls.entry.exception_error_code);
  VMP(v, ctrls.entry.instruction_len);
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

/**
 * VMREAD every fields of the VMCS
 */
void vmcs_update(struct vmcs *v) {
  // Execution controls
  VMR(v, ctrls.ex.virtual_processor_id);
  VMR(v, ctrls.ex.posted_int_notif_vector);
  VMR(v, ctrls.ex.eptp_index);
  VMR(v, ctrls.ex.io_bitmap_a);
  VMR(v, ctrls.ex.io_bitmap_b);
  VMR(v, ctrls.ex.msr_bitmap);
  VMR(v, ctrls.ex.executive_vmcs_pointer);
  VMR(v, ctrls.ex.tsc_offset);
  VMR(v, ctrls.ex.virtual_apic_page_addr);
  VMR(v, ctrls.ex.apic_access_addr);
  VMR(v, ctrls.ex.posted_intr_desc_addr);
  VMR(v, ctrls.ex.vm_function_controls);
  VMR(v, ctrls.ex.ept_pointer);
  VMR(v, ctrls.ex.eoi_exit_bitmap_0);
  VMR(v, ctrls.ex.eoi_exit_bitmap_1);
  VMR(v, ctrls.ex.eoi_exit_bitmap_2);
  VMR(v, ctrls.ex.eoi_exit_bitmap_3);
  VMR(v, ctrls.ex.eptp_list_addr);
  VMR(v, ctrls.ex.vmread_bitmap_addr);
  VMR(v, ctrls.ex.vmwrite_bitmap_addr);
  VMR(v, ctrls.ex.virt_excep_info_addr);
  VMR(v, ctrls.ex.xss_exiting_bitmap);
  VMR(v, ctrls.ex.pin_based_vm_exec_control);
  VMR(v, ctrls.ex.cpu_based_vm_exec_control);
  VMR(v, ctrls.ex.exception_bitmap);
  VMR(v, ctrls.ex.page_fault_error_code_mask);
  VMR(v, ctrls.ex.page_fault_error_code_match);
  VMR(v, ctrls.ex.cr3_target_count);
  VMR(v, ctrls.ex.tpr_threshold);
  VMR(v, ctrls.ex.secondary_vm_exec_control);
  VMR(v, ctrls.ex.ple_gap);
  VMR(v, ctrls.ex.ple_window);
  VMR(v, ctrls.ex.cr0_guest_host_mask);
  VMR(v, ctrls.ex.cr4_guest_host_mask);
  VMR(v, ctrls.ex.cr0_read_shadow);
  VMR(v, ctrls.ex.cr4_read_shadow);
  VMR(v, ctrls.ex.cr3_target_value0);
  VMR(v, ctrls.ex.cr3_target_value1);
  VMR(v, ctrls.ex.cr3_target_value2);
  VMR(v, ctrls.ex.cr3_target_value3);
  // Guest state
  VMR(v, gs.es_selector);
  VMR(v, gs.cs_selector);
  VMR(v, gs.ss_selector);
  VMR(v, gs.ds_selector);
  VMR(v, gs.fs_selector);
  VMR(v, gs.gs_selector);
  VMR(v, gs.ldtr_selector);
  VMR(v, gs.tr_selector);
  VMR(v, gs.ia32_debugctl);
  VMR(v, gs.ia32_pat);
  VMR(v, gs.ia32_efer);
  VMR(v, gs.ia32_perf_global_ctrl);
  VMR(v, gs.pdptr0);
  VMR(v, gs.pdptr1);
  VMR(v, gs.pdptr2);
  VMR(v, gs.pdptr3);
  VMR(v, gs.cr0);
  VMR(v, gs.cr3);
  VMR(v, gs.cr4);
  VMR(v, gs.es_base);
  VMR(v, gs.cs_base);
  VMR(v, gs.ss_base);
  VMR(v, gs.ds_base);
  VMR(v, gs.fs_base);
  VMR(v, gs.gs_base);
  VMR(v, gs.ldtr_base);
  VMR(v, gs.tr_base);
  VMR(v, gs.gdtr_base);
  VMR(v, gs.idtr_base);
  VMR(v, gs.dr7);
  VMR(v, gs.rsp);
  VMR(v, gs.rip);
  VMR(v, gs.rflags);
  VMR(v, gs.pending_dbg_exceptions);
  VMR(v, gs.sysenter_esp);
  VMR(v, gs.sysenter_eip);
  VMR(v, gs.es_limit);
  VMR(v, gs.cs_limit);
  VMR(v, gs.ss_limit);
  VMR(v, gs.ds_limit);
  VMR(v, gs.fs_limit);
  VMR(v, gs.gs_limit);
  VMR(v, gs.ldtr_limit);
  VMR(v, gs.tr_limit);
  VMR(v, gs.gdtr_limit);
  VMR(v, gs.idtr_limit);
  VMR(v, gs.es_ar_bytes);
  VMR(v, gs.cs_ar_bytes);
  VMR(v, gs.ss_ar_bytes);
  VMR(v, gs.ds_ar_bytes);
  VMR(v, gs.fs_ar_bytes);
  VMR(v, gs.gs_ar_bytes);
  VMR(v, gs.ldtr_ar_bytes);
  VMR(v, gs.tr_ar_bytes);
  VMR(v, gs.interruptibility_info);
  VMR(v, gs.activity_state);
  VMR(v, gs.smbase);
  VMR(v, gs.sysenter_cs);
  VMR(v, gs.vmcs_link_pointer);
  VMR(v, gs.interrupt_status);
  VMR(v, gs.vmx_preemption_timer_value);
  // Host state
  VMR(v, hs.es_selector);
  VMR(v, hs.cs_selector);
  VMR(v, hs.ss_selector);
  VMR(v, hs.ds_selector);
  VMR(v, hs.fs_selector);
  VMR(v, hs.gs_selector);
  VMR(v, hs.tr_selector);
  VMR(v, hs.ia32_pat);
  VMR(v, hs.ia32_efer);
  VMR(v, hs.ia32_perf_global_ctrl);
  VMR(v, hs.ia32_sysenter_cs);
  VMR(v, hs.cr0);
  VMR(v, hs.cr3);
  VMR(v, hs.cr4);
  VMR(v, hs.fs_base);
  VMR(v, hs.gs_base);
  VMR(v, hs.tr_base);
  VMR(v, hs.gdtr_base);
  VMR(v, hs.idtr_base);
  VMR(v, hs.ia32_sysenter_esp);
  VMR(v, hs.ia32_sysenter_eip);
  VMR(v, hs.rsp);
  VMR(v, hs.rip);
  // Vm exit controls
  VMR(v, ctrls.exit.msr_store_addr);
  VMR(v, ctrls.exit.msr_load_addr);
  VMR(v, ctrls.exit.controls);
  VMR(v, ctrls.exit.msr_store_count);
  VMR(v, ctrls.exit.msr_load_count);
  // VM entry controls
  VMR(v, ctrls.entry.msr_load_addr);
  VMR(v, ctrls.entry.controls);
  VMR(v, ctrls.entry.msr_load_count);
  VMR(v, ctrls.entry.intr_info_field);
  VMR(v, ctrls.entry.exception_error_code);
  VMR(v, ctrls.entry.instruction_len);
  // VM exit info
  VMR(v, info.guest_physical_address);
  VMR(v, info.vm_instruction_error);
  VMR(v, info.reason);
  VMR(v, info.intr_info);
  VMR(v, info.intr_error_code);
  VMR(v, info.idt_vectoring_info_field);
  VMR(v, info.idt_vectoring_error_code);
  VMR(v, info.instruction_len);
  VMR(v, info.vmx_instruction_info);
  VMR(v, info.qualification);
  VMR(v, info.io_rcx);
  VMR(v, info.io_rsi);
  VMR(v, info.io_rdi);
  VMR(v, info.io_rip);
  VMR(v, info.guest_linear_address);
}

/**
 * VMWRITE every modified field of the VMCS
 */
void vmcs_commit(struct vmcs *v) {
  // Execution controls
  VMF(v, ctrls.ex.virtual_processor_id);
  VMF(v, ctrls.ex.posted_int_notif_vector);
  VMF(v, ctrls.ex.eptp_index);
  VMF(v, ctrls.ex.io_bitmap_a);
  VMF(v, ctrls.ex.io_bitmap_b);
  VMF(v, ctrls.ex.msr_bitmap);
  VMF(v, ctrls.ex.executive_vmcs_pointer);
  VMF(v, ctrls.ex.tsc_offset);
  VMF(v, ctrls.ex.virtual_apic_page_addr);
  VMF(v, ctrls.ex.apic_access_addr);
  VMF(v, ctrls.ex.posted_intr_desc_addr);
  VMF(v, ctrls.ex.vm_function_controls);
  VMF(v, ctrls.ex.ept_pointer);
  VMF(v, ctrls.ex.eoi_exit_bitmap_0);
  VMF(v, ctrls.ex.eoi_exit_bitmap_1);
  VMF(v, ctrls.ex.eoi_exit_bitmap_2);
  VMF(v, ctrls.ex.eoi_exit_bitmap_3);
  VMF(v, ctrls.ex.eptp_list_addr);
  VMF(v, ctrls.ex.vmread_bitmap_addr);
  VMF(v, ctrls.ex.vmwrite_bitmap_addr);
  VMF(v, ctrls.ex.virt_excep_info_addr);
  VMF(v, ctrls.ex.xss_exiting_bitmap);
  VMF(v, ctrls.ex.pin_based_vm_exec_control);
  VMF(v, ctrls.ex.cpu_based_vm_exec_control);
  VMF(v, ctrls.ex.exception_bitmap);
  VMF(v, ctrls.ex.page_fault_error_code_mask);
  VMF(v, ctrls.ex.page_fault_error_code_match);
  VMF(v, ctrls.ex.cr3_target_count);
  VMF(v, ctrls.ex.tpr_threshold);
  VMF(v, ctrls.ex.secondary_vm_exec_control);
  VMF(v, ctrls.ex.ple_gap);
  VMF(v, ctrls.ex.ple_window);
  VMF(v, ctrls.ex.cr0_guest_host_mask);
  VMF(v, ctrls.ex.cr4_guest_host_mask);
  VMF(v, ctrls.ex.cr0_read_shadow);
  VMF(v, ctrls.ex.cr4_read_shadow);
  VMF(v, ctrls.ex.cr3_target_value0);
  VMF(v, ctrls.ex.cr3_target_value1);
  VMF(v, ctrls.ex.cr3_target_value2);
  VMF(v, ctrls.ex.cr3_target_value3);
  // Guest state
  VMF(v, gs.es_selector);
  VMF(v, gs.cs_selector);
  VMF(v, gs.ss_selector);
  VMF(v, gs.ds_selector);
  VMF(v, gs.fs_selector);
  VMF(v, gs.gs_selector);
  VMF(v, gs.ldtr_selector);
  VMF(v, gs.tr_selector);
  VMF(v, gs.ia32_debugctl);
  VMF(v, gs.ia32_pat);
  VMF(v, gs.ia32_efer);
  VMF(v, gs.ia32_perf_global_ctrl);
  VMF(v, gs.pdptr0);
  VMF(v, gs.pdptr1);
  VMF(v, gs.pdptr2);
  VMF(v, gs.pdptr3);
  VMF(v, gs.cr0);
  VMF(v, gs.cr3);
  VMF(v, gs.cr4);
  VMF(v, gs.es_base);
  VMF(v, gs.cs_base);
  VMF(v, gs.ss_base);
  VMF(v, gs.ds_base);
  VMF(v, gs.fs_base);
  VMF(v, gs.gs_base);
  VMF(v, gs.ldtr_base);
  VMF(v, gs.tr_base);
  VMF(v, gs.gdtr_base);
  VMF(v, gs.idtr_base);
  VMF(v, gs.dr7);
  VMF(v, gs.rsp);
  VMF(v, gs.rip);
  VMF(v, gs.rflags);
  VMF(v, gs.pending_dbg_exceptions);
  VMF(v, gs.sysenter_esp);
  VMF(v, gs.sysenter_eip);
  VMF(v, gs.es_limit);
  VMF(v, gs.cs_limit);
  VMF(v, gs.ss_limit);
  VMF(v, gs.ds_limit);
  VMF(v, gs.fs_limit);
  VMF(v, gs.gs_limit);
  VMF(v, gs.ldtr_limit);
  VMF(v, gs.tr_limit);
  VMF(v, gs.gdtr_limit);
  VMF(v, gs.idtr_limit);
  VMF(v, gs.es_ar_bytes);
  VMF(v, gs.cs_ar_bytes);
  VMF(v, gs.ss_ar_bytes);
  VMF(v, gs.ds_ar_bytes);
  VMF(v, gs.fs_ar_bytes);
  VMF(v, gs.gs_ar_bytes);
  VMF(v, gs.ldtr_ar_bytes);
  VMF(v, gs.tr_ar_bytes);
  VMF(v, gs.interruptibility_info);
  VMF(v, gs.activity_state);
  VMF(v, gs.smbase);
  VMF(v, gs.sysenter_cs);
  VMF(v, gs.vmcs_link_pointer);
  VMF(v, gs.interrupt_status);
  VMF(v, gs.vmx_preemption_timer_value);
  // Host state
  VMF(v, hs.es_selector);
  VMF(v, hs.cs_selector);
  VMF(v, hs.ss_selector);
  VMF(v, hs.ds_selector);
  VMF(v, hs.fs_selector);
  VMF(v, hs.gs_selector);
  VMF(v, hs.tr_selector);
  VMF(v, hs.ia32_pat);
  VMF(v, hs.ia32_efer);
  VMF(v, hs.ia32_perf_global_ctrl);
  VMF(v, hs.ia32_sysenter_cs);
  VMF(v, hs.cr0);
  VMF(v, hs.cr3);
  VMF(v, hs.cr4);
  VMF(v, hs.fs_base);
  VMF(v, hs.gs_base);
  VMF(v, hs.tr_base);
  VMF(v, hs.gdtr_base);
  VMF(v, hs.idtr_base);
  VMF(v, hs.ia32_sysenter_esp);
  VMF(v, hs.ia32_sysenter_eip);
  VMF(v, hs.rsp);
  VMF(v, hs.rip);
  // Vm exit controls
  VMF(v, ctrls.exit.msr_store_addr);
  VMF(v, ctrls.exit.msr_load_addr);
  VMF(v, ctrls.exit.controls);
  VMF(v, ctrls.exit.msr_store_count);
  VMF(v, ctrls.exit.msr_load_count);
  // VM entry controls
  VMF(v, ctrls.entry.msr_load_addr);
  VMF(v, ctrls.entry.controls);
  VMF(v, ctrls.entry.msr_load_count);
  VMF(v, ctrls.entry.intr_info_field);
  VMF(v, ctrls.entry.exception_error_code);
  VMF(v, ctrls.entry.instruction_len);
  // VM exit info
  VMF(v, info.guest_physical_address);
  VMF(v, info.vm_instruction_error);
  VMF(v, info.reason);
  VMF(v, info.intr_info);
  VMF(v, info.intr_error_code);
  VMF(v, info.idt_vectoring_info_field);
  VMF(v, info.idt_vectoring_error_code);
  VMF(v, info.instruction_len);
  VMF(v, info.vmx_instruction_info);
  VMF(v, info.qualification);
  VMF(v, info.io_rcx);
  VMF(v, info.io_rsi);
  VMF(v, info.io_rdi);
  VMF(v, info.io_rip);
  VMF(v, info.guest_linear_address);
}

void vmcs_encoding_init(void) {
  uint32_t i;
  // Execution controls
  VMCSE(hc, ctrls.ex.virtual_processor_id, VIRTUAL_PROCESSOR_ID);
  VMCSE(hc, ctrls.ex.posted_int_notif_vector, POSTED_INT_NOTIF_VECTOR);
  VMCSE(hc, ctrls.ex.eptp_index, EPTP_INDEX);
  VMCSE(hc, ctrls.ex.io_bitmap_a, IO_BITMAP_A);
  VMCSE(hc, ctrls.ex.io_bitmap_b, IO_BITMAP_B);
  VMCSE(hc, ctrls.ex.msr_bitmap, MSR_BITMAP);
  VMCSE(hc, ctrls.ex.executive_vmcs_pointer, EXECUTIVE_VMCS_POINTER);
  VMCSE(hc, ctrls.ex.tsc_offset, TSC_OFFSET);
  VMCSE(hc, ctrls.ex.virtual_apic_page_addr, VIRTUAL_APIC_PAGE_ADDR);
  VMCSE(hc, ctrls.ex.apic_access_addr, APIC_ACCESS_ADDR);
  VMCSE(hc, ctrls.ex.posted_intr_desc_addr, POSTED_INTR_DESC_ADDR);
  VMCSE(hc, ctrls.ex.vm_function_controls, VM_FUNCTION_CONTROLS);
  VMCSE(hc, ctrls.ex.ept_pointer, EPT_POINTER);
  VMCSE(hc, ctrls.ex.eoi_exit_bitmap_0, EOI_EXIT_BITMAP_0);
  VMCSE(hc, ctrls.ex.eoi_exit_bitmap_1, EOI_EXIT_BITMAP_1);
  VMCSE(hc, ctrls.ex.eoi_exit_bitmap_2, EOI_EXIT_BITMAP_2);
  VMCSE(hc, ctrls.ex.eoi_exit_bitmap_3, EOI_EXIT_BITMAP_3);
  VMCSE(hc, ctrls.ex.eptp_list_addr, EPTP_LIST_ADDR);
  VMCSE(hc, ctrls.ex.vmread_bitmap_addr, VMREAD_BITMAP_ADDR);
  VMCSE(hc, ctrls.ex.vmwrite_bitmap_addr, VMWRITE_BITMAP_ADDR);
  VMCSE(hc, ctrls.ex.virt_excep_info_addr, VIRT_EXCEP_INFO_ADDR);
  VMCSE(hc, ctrls.ex.xss_exiting_bitmap, XSS_EXITING_BITMAP);
  VMCSE(hc, ctrls.ex.pin_based_vm_exec_control, PIN_BASED_VM_EXEC_CONTROL);
  VMCSE(hc, ctrls.ex.cpu_based_vm_exec_control, CPU_BASED_VM_EXEC_CONTROL);
  VMCSE(hc, ctrls.ex.exception_bitmap, EXCEPTION_BITMAP);
  VMCSE(hc, ctrls.ex.page_fault_error_code_mask, PAGE_FAULT_ERROR_CODE_MASK);
  VMCSE(hc, ctrls.ex.page_fault_error_code_match, PAGE_FAULT_ERROR_CODE_MATCH);
  VMCSE(hc, ctrls.ex.cr3_target_count, CR3_TARGET_COUNT);
  VMCSE(hc, ctrls.ex.tpr_threshold, TPR_THRESHOLD);
  VMCSE(hc, ctrls.ex.secondary_vm_exec_control, SECONDARY_VM_EXEC_CONTROL);
  VMCSE(hc, ctrls.ex.ple_gap, PLE_GAP);
  VMCSE(hc, ctrls.ex.ple_window, PLE_WINDOW);
  VMCSE(hc, ctrls.ex.cr0_guest_host_mask, CR0_GUEST_HOST_MASK);
  VMCSE(hc, ctrls.ex.cr4_guest_host_mask, CR4_GUEST_HOST_MASK);
  VMCSE(hc, ctrls.ex.cr0_read_shadow, CR0_READ_SHADOW);
  VMCSE(hc, ctrls.ex.cr4_read_shadow, CR4_READ_SHADOW);
  VMCSE(hc, ctrls.ex.cr3_target_value0, CR3_TARGET_VALUE0);
  VMCSE(hc, ctrls.ex.cr3_target_value1, CR3_TARGET_VALUE1);
  VMCSE(hc, ctrls.ex.cr3_target_value2, CR3_TARGET_VALUE2);
  VMCSE(hc, ctrls.ex.cr3_target_value3, CR3_TARGET_VALUE3);
  // Guest state
  VMCSE(hc, gs.es_selector, GUEST_ES_SELECTOR);
  VMCSE(hc, gs.cs_selector, GUEST_CS_SELECTOR);
  VMCSE(hc, gs.ss_selector, GUEST_SS_SELECTOR);
  VMCSE(hc, gs.ds_selector, GUEST_DS_SELECTOR);
  VMCSE(hc, gs.fs_selector, GUEST_FS_SELECTOR);
  VMCSE(hc, gs.gs_selector, GUEST_GS_SELECTOR);
  VMCSE(hc, gs.ldtr_selector, GUEST_LDTR_SELECTOR);
  VMCSE(hc, gs.tr_selector, GUEST_TR_SELECTOR);
  VMCSE(hc, gs.ia32_debugctl, GUEST_IA32_DEBUGCTL);
  VMCSE(hc, gs.ia32_pat, GUEST_IA32_PAT);
  VMCSE(hc, gs.ia32_efer, GUEST_IA32_EFER);
  VMCSE(hc, gs.ia32_perf_global_ctrl, GUEST_IA32_PERF_GLOBAL_CTRL);
  VMCSE(hc, gs.pdptr0, GUEST_PDPTR0);
  VMCSE(hc, gs.pdptr1, GUEST_PDPTR1);
  VMCSE(hc, gs.pdptr2, GUEST_PDPTR2);
  VMCSE(hc, gs.pdptr3, GUEST_PDPTR3);
  VMCSE(hc, gs.cr0, GUEST_CR0);
  VMCSE(hc, gs.cr3, GUEST_CR3);
  VMCSE(hc, gs.cr4, GUEST_CR4);
  VMCSE(hc, gs.es_base, GUEST_ES_BASE);
  VMCSE(hc, gs.cs_base, GUEST_CS_BASE);
  VMCSE(hc, gs.ss_base, GUEST_SS_BASE);
  VMCSE(hc, gs.ds_base, GUEST_DS_BASE);
  VMCSE(hc, gs.fs_base, GUEST_FS_BASE);
  VMCSE(hc, gs.gs_base, GUEST_GS_BASE);
  VMCSE(hc, gs.ldtr_base, GUEST_LDTR_BASE);
  VMCSE(hc, gs.tr_base, GUEST_TR_BASE);
  VMCSE(hc, gs.gdtr_base, GUEST_GDTR_BASE);
  VMCSE(hc, gs.idtr_base, GUEST_IDTR_BASE);
  VMCSE(hc, gs.dr7, GUEST_DR7);
  VMCSE(hc, gs.rsp, GUEST_RSP);
  VMCSE(hc, gs.rip, GUEST_RIP);
  VMCSE(hc, gs.rflags, GUEST_RFLAGS);
  VMCSE(hc, gs.pending_dbg_exceptions, GUEST_PENDING_DBG_EXCEPTIONS);
  VMCSE(hc, gs.sysenter_esp, GUEST_SYSENTER_ESP);
  VMCSE(hc, gs.sysenter_eip, GUEST_SYSENTER_EIP);
  VMCSE(hc, gs.es_limit, GUEST_ES_LIMIT);
  VMCSE(hc, gs.cs_limit, GUEST_CS_LIMIT);
  VMCSE(hc, gs.ss_limit, GUEST_SS_LIMIT);
  VMCSE(hc, gs.ds_limit, GUEST_DS_LIMIT);
  VMCSE(hc, gs.fs_limit, GUEST_FS_LIMIT);
  VMCSE(hc, gs.gs_limit, GUEST_GS_LIMIT);
  VMCSE(hc, gs.ldtr_limit, GUEST_LDTR_LIMIT);
  VMCSE(hc, gs.tr_limit, GUEST_TR_LIMIT);
  VMCSE(hc, gs.gdtr_limit, GUEST_GDTR_LIMIT);
  VMCSE(hc, gs.idtr_limit, GUEST_IDTR_LIMIT);
  VMCSE(hc, gs.es_ar_bytes, GUEST_ES_AR_BYTES);
  VMCSE(hc, gs.cs_ar_bytes, GUEST_CS_AR_BYTES);
  VMCSE(hc, gs.ss_ar_bytes, GUEST_SS_AR_BYTES);
  VMCSE(hc, gs.ds_ar_bytes, GUEST_DS_AR_BYTES);
  VMCSE(hc, gs.fs_ar_bytes, GUEST_FS_AR_BYTES);
  VMCSE(hc, gs.gs_ar_bytes, GUEST_GS_AR_BYTES);
  VMCSE(hc, gs.ldtr_ar_bytes, GUEST_LDTR_AR_BYTES);
  VMCSE(hc, gs.tr_ar_bytes, GUEST_TR_AR_BYTES);
  VMCSE(hc, gs.interruptibility_info, GUEST_INTERRUPTIBILITY_INFO);
  VMCSE(hc, gs.activity_state, GUEST_ACTIVITY_STATE);
  VMCSE(hc, gs.smbase, GUEST_SMBASE);
  VMCSE(hc, gs.sysenter_cs, GUEST_SYSENTER_CS);
  VMCSE(hc, gs.vmcs_link_pointer, VMCS_LINK_POINTER);
  VMCSE(hc, gs.interrupt_status, GUEST_INTERRUPT_STATUS);
  VMCSE(hc, gs.vmx_preemption_timer_value, VMX_PREEMPTION_TIMER_VALUE);
  // Host state
  VMCSE(hc, hs.es_selector, HOST_ES_SELECTOR);
  VMCSE(hc, hs.cs_selector, HOST_CS_SELECTOR);
  VMCSE(hc, hs.ss_selector, HOST_SS_SELECTOR);
  VMCSE(hc, hs.ds_selector, HOST_DS_SELECTOR);
  VMCSE(hc, hs.fs_selector, HOST_FS_SELECTOR);
  VMCSE(hc, hs.gs_selector, HOST_GS_SELECTOR);
  VMCSE(hc, hs.tr_selector, HOST_TR_SELECTOR);
  VMCSE(hc, hs.ia32_pat, HOST_IA32_PAT);
  VMCSE(hc, hs.ia32_efer, HOST_IA32_EFER);
  VMCSE(hc, hs.ia32_perf_global_ctrl, HOST_IA32_PERF_GLOBAL_CTRL);
  VMCSE(hc, hs.ia32_sysenter_cs, HOST_IA32_SYSENTER_CS);
  VMCSE(hc, hs.cr0, HOST_CR0);
  VMCSE(hc, hs.cr3, HOST_CR3);
  VMCSE(hc, hs.cr4, HOST_CR4);
  VMCSE(hc, hs.fs_base, HOST_FS_BASE);
  VMCSE(hc, hs.gs_base, HOST_GS_BASE);
  VMCSE(hc, hs.tr_base, HOST_TR_BASE);
  VMCSE(hc, hs.gdtr_base, HOST_GDTR_BASE);
  VMCSE(hc, hs.idtr_base, HOST_IDTR_BASE);
  VMCSE(hc, hs.ia32_sysenter_esp, HOST_IA32_SYSENTER_ESP);
  VMCSE(hc, hs.ia32_sysenter_eip, HOST_IA32_SYSENTER_EIP);
  VMCSE(hc, hs.rsp, HOST_RSP);
  VMCSE(hc, hs.rip, HOST_RIP);
  // Vm exit controls
  VMCSE(hc, ctrls.exit.msr_store_addr, VM_EXIT_MSR_STORE_ADDR);
  VMCSE(hc, ctrls.exit.msr_load_addr, VM_EXIT_MSR_LOAD_ADDR);
  VMCSE(hc, ctrls.exit.controls, VM_EXIT_CONTROLS);
  VMCSE(hc, ctrls.exit.msr_store_count, VM_EXIT_MSR_STORE_COUNT);
  VMCSE(hc, ctrls.exit.msr_load_count, VM_EXIT_MSR_LOAD_COUNT);
  // VM entry controls
  VMCSE(hc, ctrls.entry.msr_load_addr, VM_ENTRY_MSR_LOAD_ADDR);
  VMCSE(hc, ctrls.entry.controls, VM_ENTRY_CONTROLS);
  VMCSE(hc, ctrls.entry.msr_load_count, VM_ENTRY_MSR_LOAD_COUNT);
  VMCSE(hc, ctrls.entry.intr_info_field, VM_ENTRY_INTR_INFO_FIELD);
  VMCSE(hc, ctrls.entry.exception_error_code, VM_ENTRY_EXCEPTION_ERROR_CODE);
  VMCSE(hc, ctrls.entry.instruction_len, VM_ENTRY_INSTRUCTION_LEN);
  // VM exit info
  VMCSE(hc, info.guest_physical_address, GUEST_PHYSICAL_ADDRESS);
  VMCSE(hc, info.vm_instruction_error, VM_INSTRUCTION_ERROR);
  VMCSE(hc, info.reason, VM_EXIT_REASON);
  VMCSE(hc, info.intr_info, VM_EXIT_INTR_INFO);
  VMCSE(hc, info.intr_error_code, VM_EXIT_INTR_ERROR_CODE);
  VMCSE(hc, info.idt_vectoring_info_field, IDT_VECTORING_INFO_FIELD);
  VMCSE(hc, info.idt_vectoring_error_code, IDT_VECTORING_ERROR_CODE);
  VMCSE(hc, info.instruction_len, VM_EXIT_INSTRUCTION_LEN);
  VMCSE(hc, info.vmx_instruction_info, VMX_INSTRUCTION_INFO);
  VMCSE(hc, info.qualification, EXIT_QUALIFICATION);
  VMCSE(hc, info.io_rcx, IO_RCX);
  VMCSE(hc, info.io_rsi, IO_RSI);
  VMCSE(hc, info.io_rdi, IO_RDI);
  VMCSE(hc, info.io_rip, IO_RIP);
  VMCSE(hc, info.guest_linear_address, GUEST_LINEAR_ADDRESS);
  // Copying the encodings
  for (i = 0; i < VM_NB; i++) {
    memcpy(&vmcs_cache_pool[i], hc, sizeof(struct vmcs));
  }
}

void vmcs_host_config_host_state_fields(void) {
  struct gdt_entry gdt_entry;
  struct idt_ptr idt_ptr;

  VMW(hc, hs.cr0, cpu_adjust64(cpu_read_cr0(),
        MSR_ADDRESS_VMX_CR0_FIXED0, MSR_ADDRESS_VMX_CR0_FIXED1));
  VMW(hc, hs.cr3, paging_get_host_cr3());
  // TODO Bit 18 OSXSAVE Activation du XCR0 -> proxifier le XSETBV
  VMW(hc, hs.cr4, cpu_adjust64(cpu_read_cr4() | (1 << 18),
        MSR_ADDRESS_VMX_CR4_FIXED0, MSR_ADDRESS_VMX_CR4_FIXED1));
  VMW(hc, hs.rsp, (uint64_t)&vmm_stack[VMM_STACK_SIZE]);
  VMW(hc, hs.rip, (uint64_t)vmm_vm_exit_handler);

  VMW(hc, hs.cs_selector, cpu_read_cs() & 0xf8);
  VMW(hc, hs.ss_selector, cpu_read_ss() & 0xf8);
  VMW(hc, hs.ds_selector, cpu_read_ds() & 0xf8);
  VMW(hc, hs.es_selector, cpu_read_es() & 0xf8);
  VMW(hc, hs.fs_selector, cpu_read_fs() & 0xf8);
  VMW(hc, hs.gs_selector, cpu_read_gs() & 0xf8);
  VMW(hc, hs.tr_selector, 0x8 & 0xf8);

  gdt_get_host_entry(cpu_read_fs(), &gdt_entry);
  VMW(hc, hs.fs_base, gdt_entry.base);
  gdt_get_host_entry(cpu_read_gs(), &gdt_entry);
  VMW(hc, hs.gs_base, gdt_entry.base);
  gdt_get_host_entry(0x8, &gdt_entry);
  VMW(hc, hs.tr_base, gdt_entry.base);

  VMW(hc, hs.gdtr_base, gdt_get_host_base());

  idt_get_idt_ptr(&idt_ptr);
  VMW(hc, hs.idtr_base, idt_ptr.base);

  VMW(hc, hs.ia32_sysenter_cs,
      msr_read(MSR_ADDRESS_IA32_SYSENTER_CS));
  VMW(hc, hs.ia32_sysenter_esp,
      msr_read(MSR_ADDRESS_IA32_SYSENTER_ESP));
  VMW(hc, hs.ia32_sysenter_eip,
      msr_read(MSR_ADDRESS_IA32_SYSENTER_EIP));
  VMW(hc, hs.ia32_efer, msr_read(MSR_ADDRESS_IA32_EFER));

  VMW(hc, hs.ia32_perf_global_ctrl, 0);
}

void vmcs_host_config_vm_exec_control_fields(void) {
  uint32_t procbased_ctls = ACT_SECONDARY_CONTROLS | USE_MSR_BITMAPS |
      USE_IO_BITMAPS | CR3_LOAD_EXITING;
  uint32_t procbased_ctls_2 = ENABLE_EPT | ENABLE_VPID | UNRESTRICTED_GUEST |
      ENABLE_RDTSCP;
                             
  uint64_t msr_bitmap_ptr;
  uint64_t eptp;
  uint64_t io_bitmap_ptr;
  uint32_t pinbased_ctls = ACT_VMX_PREEMPT_TIMER | NMI_EXITING;

  VMW(hc, ctrls.ex.pin_based_vm_exec_control,
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
  VMW(hc, ctrls.ex.cpu_based_vm_exec_control, procbased_ctls);

  VMW(hc, ctrls.ex.secondary_vm_exec_control,
      cpu_adjust32(procbased_ctls_2, MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS2));

  VMW(hc, ctrls.ex.exception_bitmap, 0);
  VMW(hc, ctrls.ex.page_fault_error_code_mask, 0);
  VMW(hc, ctrls.ex.page_fault_error_code_match, 0);

  io_bitmap_ptr = io_bitmap_get_ptr_a();
  VMW(hc, ctrls.ex.io_bitmap_a, io_bitmap_ptr);
  io_bitmap_ptr = io_bitmap_get_ptr_b();
  VMW(hc, ctrls.ex.io_bitmap_b, io_bitmap_ptr);

  VMW(hc, ctrls.ex.tsc_offset, 0);

  // As we are using UNRESTRICTED_GUEST procbased_ctrl, the guest can itself
  // modify CR0.PE and CR0.PG, see doc INTEL vol 3C chap 23.8
  VMW(hc, ctrls.ex.cr0_guest_host_mask,
      (msr_read(MSR_ADDRESS_VMX_CR0_FIXED0) |
       (~msr_read(MSR_ADDRESS_VMX_CR0_FIXED1))) & ~(0x80000001));
  VMW(hc, ctrls.ex.cr0_read_shadow, cpu_read_cr0());
  VMW(hc, ctrls.ex.cr4_guest_host_mask,
      msr_read(MSR_ADDRESS_VMX_CR4_FIXED0) |
      ~msr_read(MSR_ADDRESS_VMX_CR4_FIXED1));
  VMW(hc, ctrls.ex.cr4_read_shadow, cpu_read_cr4() &
      ~(uint64_t)(1<<13)); // We hide CR4.VMXE

  VMW(hc, ctrls.ex.cr3_target_count, 0);
  VMW(hc, ctrls.ex.cr3_target_value0, 0);
  VMW(hc, ctrls.ex.cr3_target_value1, 0);
  VMW(hc, ctrls.ex.cr3_target_value2, 0);
  VMW(hc, ctrls.ex.cr3_target_value3, 0);

  msr_bitmap_ptr = msr_bitmap_get_ptr();
  VMW(hc, ctrls.ex.msr_bitmap, msr_bitmap_ptr);

  eptp = ept_get_eptp();
  VMW(hc, ctrls.ex.ept_pointer, eptp);
  VMW(hc, ctrls.ex.virtual_processor_id, 0x1); // vmcs0 is 1

  VMW(hc, ctrls.ex.virtual_apic_page_addr,
      ((uintptr_t)&vapic[0]));
}

void vmcs_host_config_guest_state_fields() {
  struct gdt_entry gdt_entry;
  struct idt_ptr idt_ptr;
  uint64_t msr;
  uint64_t sel;

  VMW(hc, gs.cr0, cpu_adjust64(cpu_read_cr0(), MSR_ADDRESS_VMX_CR0_FIXED0, MSR_ADDRESS_VMX_CR0_FIXED1));
  VMW(hc, gs.cr3, cpu_read_cr3());
  VMW(hc, gs.cr4, cpu_adjust64(cpu_read_cr4(), MSR_ADDRESS_VMX_CR4_FIXED0, MSR_ADDRESS_VMX_CR4_FIXED1));
  VMW(hc, gs.dr7, cpu_read_dr7());
  VMW(hc, gs.rflags, (1 << 1));

  sel = cpu_read_cs();
  gdt_get_guest_entry(sel, &gdt_entry);
  VMW(hc, gs.cs_selector, sel);
  VMW(hc, gs.cs_base, gdt_entry.base);
  VMW(hc, gs.cs_limit, gdt_entry.limit);
  VMW(hc, gs.cs_ar_bytes, (gdt_entry.granularity << 8) | (gdt_entry.access << 0));
  sel = cpu_read_ss();
  gdt_get_guest_entry(sel, &gdt_entry);
  VMW(hc, gs.ss_selector, sel);
  VMW(hc, gs.ss_base, gdt_entry.base);
  VMW(hc, gs.ss_limit, gdt_entry.limit);
  VMW(hc, gs.ss_ar_bytes, (gdt_entry.granularity << 8) | (gdt_entry.access << 0));
  sel = cpu_read_ds();
  gdt_get_guest_entry(sel, &gdt_entry);
  VMW(hc, gs.ds_selector, sel);
  VMW(hc, gs.ds_base, gdt_entry.base);
  VMW(hc, gs.ds_limit, gdt_entry.limit);
  VMW(hc, gs.ds_ar_bytes, (gdt_entry.granularity << 8) | (gdt_entry.access << 0));
  sel = cpu_read_es();
  gdt_get_guest_entry(sel, &gdt_entry);
  VMW(hc, gs.es_selector, sel);
  VMW(hc, gs.es_base, gdt_entry.base);
  VMW(hc, gs.es_limit, gdt_entry.limit);
  VMW(hc, gs.es_ar_bytes, (gdt_entry.granularity << 8) | (gdt_entry.access << 0));
  sel = cpu_read_fs();
  gdt_get_guest_entry(sel, &gdt_entry);
  VMW(hc, gs.fs_selector, sel);
  VMW(hc, gs.fs_base, gdt_entry.base);
  VMW(hc, gs.fs_limit, gdt_entry.limit);
  VMW(hc, gs.fs_ar_bytes, (gdt_entry.granularity << 8) | (gdt_entry.access << 0));
  sel = cpu_read_gs();
  gdt_get_guest_entry(sel, &gdt_entry);
  VMW(hc, gs.gs_selector, sel);
  VMW(hc, gs.gs_base, gdt_entry.base);
  VMW(hc, gs.gs_limit, gdt_entry.limit);
  VMW(hc, gs.gs_ar_bytes, (gdt_entry.granularity << 8) | (gdt_entry.access << 0));

  /* TODO Verify if these values are correct. */
  VMW(hc, gs.ldtr_selector, 0);
  VMW(hc, gs.ldtr_base, 0);
  VMW(hc, gs.ldtr_limit, 0xffff);
  VMW(hc, gs.ldtr_ar_bytes, 0x82);
  VMW(hc, gs.tr_selector, 0);
  VMW(hc, gs.tr_base, 0);
  VMW(hc, gs.tr_limit, 0xffff);
  VMW(hc, gs.tr_ar_bytes, 0x8b);

  VMW(hc, gs.gdtr_base, gdt_get_guest_base());
  VMW(hc, gs.gdtr_limit, gdt_get_guest_limit());

  cpu_read_idt((uint8_t *) &idt_ptr);
  VMW(hc, gs.idtr_base, idt_ptr.base);
  VMW(hc, gs.idtr_limit, idt_ptr.limit);

  VMW(hc, gs.ia32_debugctl, msr_read(MSR_ADDRESS_IA32_DEBUGCTL));
  VMW(hc, gs.sysenter_cs, msr_read(MSR_ADDRESS_IA32_SYSENTER_CS));

  msr = msr_read(MSR_ADDRESS_IA32_EFER);
  VMW(hc, gs.ia32_efer, msr);

  VMW(hc, gs.activity_state, 0);
  VMW(hc, gs.interruptibility_info, 0);
  VMW(hc, gs.pending_dbg_exceptions, 0);
  VMW(hc, gs.vmcs_link_pointer, 0xffffffffffffffff);

  VMW(hc, gs.ia32_perf_global_ctrl, msr_read(MSR_ADDRESS_IA32_PERF_GLOBAL_CTRL));

  // Init and compute vmx_preemption_timer_value
  tsc_freq_MHz = ((msr_read(MSR_ADDRESS_MSR_PLATFORM_INFO) >> 8) & 0xff) * 100;
  tsc_divider = msr_read(MSR_ADDRESS_IA32_VMX_MISC) & 0x7;
  vmcs_set_vmx_preemption_timer_value(hc, VMCS_DEFAULT_PREEMPTION_TIMER_MICROSEC);
}

void vmcs_host_config_vm_exit_control_fields(void) {
  uint32_t exit_controls = EXIT_SAVE_IA32_EFER | EXIT_LOAD_IA32_EFER |
    EXIT_LOAD_IA32_PERF_GLOBAL_CTRL | HOST_ADDR_SPACE_SIZE |
    SAVE_VMX_PREEMPT_TIMER_VAL;
  VMW(hc, ctrls.exit.controls, cpu_adjust32(exit_controls,
        MSR_ADDRESS_IA32_VMX_EXIT_CTLS));
  VMW(hc, ctrls.exit.msr_store_count, 0);
  VMW(hc, ctrls.exit.msr_load_count, 0);
}

void vmcs_host_config_vm_entry_control_fields(void) {
  uint32_t entry_controls = ENTRY_LOAD_IA32_EFER |
    ENTRY_LOAD_IA32_PERF_GLOBAL_CTRL | IA32E_MODE_GUEST;
  VMW(hc, ctrls.entry.controls, cpu_adjust32(entry_controls,
        MSR_ADDRESS_IA32_VMX_ENTRY_CTLS));
  VMW(hc, ctrls.entry.msr_load_count, 0);
  VMW(hc, ctrls.entry.intr_info_field, 0);
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
  vmcs_host_config_host_state_fields();
  vmcs_host_config_guest_state_fields();
  vmcs_host_config_vm_exec_control_fields();
  vmcs_host_config_vm_exit_control_fields();
  vmcs_host_config_vm_entry_control_fields();
}

void vmcs_init(void) {
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
  vmcs_encoding_init();
  INFO("Fills l0 host state and execution controls\n");
  vmcs_host_config_configure();
}

void vmcs_set_vmx_preemption_timer_value(struct vmcs *v, uint64_t time_microsec) {
  VMW(v, gs.vmx_preemption_timer_value, (tsc_freq_MHz * time_microsec) >> tsc_divider);
}
