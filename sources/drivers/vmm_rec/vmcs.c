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
 * Host vmcs reference
 */
struct vmcs *host_vmcs;

/**
 * VMCS cache pool
 */
struct vmcs *vmcs_cache_pool;

/**
 * VMCS region pool
 */
uint8_t **vmcs_region_pool;

static uint16_t tsc_freq_MHz;
static uint8_t tsc_divider;

void vmcs_encoding_init(void) {
  uint32_t i;
  // Execution controls
  VMCS_ENC(host_vmcs->ctrls.exec, virtual_processor_id, VIRTUAL_PROCESSOR_ID);
  VMCS_ENC(host_vmcs->ctrls.exec, posted_int_notif_vector, POSTED_INT_NOTIF_VECTOR);
  VMCS_ENC(host_vmcs->ctrls.exec, eptp_index, EPTP_INDEX);
  VMCS_ENC(host_vmcs->ctrls.exec, io_bitmap_a, IO_BITMAP_A);
  VMCS_ENC(host_vmcs->ctrls.exec, io_bitmap_b, IO_BITMAP_B);
  VMCS_ENC(host_vmcs->ctrls.exec, msr_bitmap, MSR_BITMAP);
  VMCS_ENC(host_vmcs->ctrls.exec, executive_vmcs_pointer, EXECUTIVE_VMCS_POINTER);
  VMCS_ENC(host_vmcs->ctrls.exec, tsc_offset, TSC_OFFSET);
  VMCS_ENC(host_vmcs->ctrls.exec, virtual_apic_page_addr, VIRTUAL_APIC_PAGE_ADDR);
  VMCS_ENC(host_vmcs->ctrls.exec, apic_access_addr, APIC_ACCESS_ADDR);
  VMCS_ENC(host_vmcs->ctrls.exec, posted_intr_desc_addr, POSTED_INTR_DESC_ADDR);
  VMCS_ENC(host_vmcs->ctrls.exec, vm_function_controls, VM_FUNCTION_CONTROLS);
  VMCS_ENC(host_vmcs->ctrls.exec, ept_pointer, EPT_POINTER);
  VMCS_ENC(host_vmcs->ctrls.exec, eoi_exit_bitmap_0, EOI_EXIT_BITMAP_0);
  VMCS_ENC(host_vmcs->ctrls.exec, eoi_exit_bitmap_1, EOI_EXIT_BITMAP_1);
  VMCS_ENC(host_vmcs->ctrls.exec, eoi_exit_bitmap_2, EOI_EXIT_BITMAP_2);
  VMCS_ENC(host_vmcs->ctrls.exec, eoi_exit_bitmap_3, EOI_EXIT_BITMAP_3);
  VMCS_ENC(host_vmcs->ctrls.exec, eptp_list_addr, EPTP_LIST_ADDR);
  VMCS_ENC(host_vmcs->ctrls.exec, vmread_bitmap_addr, VMREAD_BITMAP_ADDR);
  VMCS_ENC(host_vmcs->ctrls.exec, vmwrite_bitmap_addr, VMWRITE_BITMAP_ADDR);
  VMCS_ENC(host_vmcs->ctrls.exec, virt_excep_info_addr, VIRT_EXCEP_INFO_ADDR);
  VMCS_ENC(host_vmcs->ctrls.exec, xss_exiting_bitmap, XSS_EXITING_BITMAP);
  VMCS_ENC(host_vmcs->ctrls.exec, pin_based_vm_exec_control, PIN_BASED_VM_EXEC_CONTROL);
  VMCS_ENC(host_vmcs->ctrls.exec, cpu_based_vm_exec_control, CPU_BASED_VM_EXEC_CONTROL);
  VMCS_ENC(host_vmcs->ctrls.exec, exception_bitmap, EXCEPTION_BITMAP);
  VMCS_ENC(host_vmcs->ctrls.exec, page_fault_error_code_mask, PAGE_FAULT_ERROR_CODE_MASK);
  VMCS_ENC(host_vmcs->ctrls.exec, page_fault_error_code_match, PAGE_FAULT_ERROR_CODE_MATCH);
  VMCS_ENC(host_vmcs->ctrls.exec, cr3_target_count, CR3_TARGET_COUNT);
  VMCS_ENC(host_vmcs->ctrls.exec, tpr_threshold, TPR_THRESHOLD);
  VMCS_ENC(host_vmcs->ctrls.exec, secondary_vm_exec_control, SECONDARY_VM_EXEC_CONTROL);
  VMCS_ENC(host_vmcs->ctrls.exec, ple_gap, PLE_GAP);
  VMCS_ENC(host_vmcs->ctrls.exec, ple_window, PLE_WINDOW);
  VMCS_ENC(host_vmcs->ctrls.exec, cr0_guest_host_mask, CR0_GUEST_HOST_MASK);
  VMCS_ENC(host_vmcs->ctrls.exec, cr4_guest_host_mask, CR4_GUEST_HOST_MASK);
  VMCS_ENC(host_vmcs->ctrls.exec, cr0_read_shadow, CR0_READ_SHADOW);
  VMCS_ENC(host_vmcs->ctrls.exec, cr4_read_shadow, CR4_READ_SHADOW);
  VMCS_ENC(host_vmcs->ctrls.exec, cr3_target_value0, CR3_TARGET_VALUE0);
  VMCS_ENC(host_vmcs->ctrls.exec, cr3_target_value1, CR3_TARGET_VALUE1);
  VMCS_ENC(host_vmcs->ctrls.exec, cr3_target_value2, CR3_TARGET_VALUE2);
  VMCS_ENC(host_vmcs->ctrls.exec, cr3_target_value3, CR3_TARGET_VALUE3);
  // Guest state
  VMCS_ENC(host_vmcs->gs, es_selector, GUEST_ES_SELECTOR);
  VMCS_ENC(host_vmcs->gs, cs_selector, GUEST_CS_SELECTOR);
  VMCS_ENC(host_vmcs->gs, ss_selector, GUEST_SS_SELECTOR);
  VMCS_ENC(host_vmcs->gs, ds_selector, GUEST_DS_SELECTOR);
  VMCS_ENC(host_vmcs->gs, fs_selector, GUEST_FS_SELECTOR);
  VMCS_ENC(host_vmcs->gs, gs_selector, GUEST_GS_SELECTOR);
  VMCS_ENC(host_vmcs->gs, ldtr_selector, GUEST_LDTR_SELECTOR);
  VMCS_ENC(host_vmcs->gs, tr_selector, GUEST_TR_SELECTOR);
  VMCS_ENC(host_vmcs->gs, ia32_debugctl, GUEST_IA32_DEBUGCTL);
  VMCS_ENC(host_vmcs->gs, ia32_pat, GUEST_IA32_PAT);
  VMCS_ENC(host_vmcs->gs, ia32_efer, GUEST_IA32_EFER);
  VMCS_ENC(host_vmcs->gs, ia32_perf_global_ctrl, GUEST_IA32_PERF_GLOBAL_CTRL);
  VMCS_ENC(host_vmcs->gs, pdptr0, GUEST_PDPTR0);
  VMCS_ENC(host_vmcs->gs, pdptr1, GUEST_PDPTR1);
  VMCS_ENC(host_vmcs->gs, pdptr2, GUEST_PDPTR2);
  VMCS_ENC(host_vmcs->gs, pdptr3, GUEST_PDPTR3);
  VMCS_ENC(host_vmcs->gs, cr0, GUEST_CR0);
  VMCS_ENC(host_vmcs->gs, cr3, GUEST_CR3);
  VMCS_ENC(host_vmcs->gs, cr4, GUEST_CR4);
  VMCS_ENC(host_vmcs->gs, es_base, GUEST_ES_BASE);
  VMCS_ENC(host_vmcs->gs, cs_base, GUEST_CS_BASE);
  VMCS_ENC(host_vmcs->gs, ss_base, GUEST_SS_BASE);
  VMCS_ENC(host_vmcs->gs, ds_base, GUEST_DS_BASE);
  VMCS_ENC(host_vmcs->gs, fs_base, GUEST_FS_BASE);
  VMCS_ENC(host_vmcs->gs, gs_base, GUEST_GS_BASE);
  VMCS_ENC(host_vmcs->gs, ldtr_base, GUEST_LDTR_BASE);
  VMCS_ENC(host_vmcs->gs, tr_base, GUEST_TR_BASE);
  VMCS_ENC(host_vmcs->gs, gdtr_base, GUEST_GDTR_BASE);
  VMCS_ENC(host_vmcs->gs, idtr_base, GUEST_IDTR_BASE);
  VMCS_ENC(host_vmcs->gs, dr7, GUEST_DR7);
  VMCS_ENC(host_vmcs->gs, rsp, GUEST_RSP);
  VMCS_ENC(host_vmcs->gs, rip, GUEST_RIP);
  VMCS_ENC(host_vmcs->gs, rflags, GUEST_RFLAGS);
  VMCS_ENC(host_vmcs->gs, pending_dbg_exceptions, GUEST_PENDING_DBG_EXCEPTIONS);
  VMCS_ENC(host_vmcs->gs, sysenter_esp, GUEST_SYSENTER_ESP);
  VMCS_ENC(host_vmcs->gs, sysenter_eip, GUEST_SYSENTER_EIP);
  VMCS_ENC(host_vmcs->gs, es_limit, GUEST_ES_LIMIT);
  VMCS_ENC(host_vmcs->gs, cs_limit, GUEST_CS_LIMIT);
  VMCS_ENC(host_vmcs->gs, ss_limit, GUEST_SS_LIMIT);
  VMCS_ENC(host_vmcs->gs, ds_limit, GUEST_DS_LIMIT);
  VMCS_ENC(host_vmcs->gs, fs_limit, GUEST_FS_LIMIT);
  VMCS_ENC(host_vmcs->gs, gs_limit, GUEST_GS_LIMIT);
  VMCS_ENC(host_vmcs->gs, ldtr_limit, GUEST_LDTR_LIMIT);
  VMCS_ENC(host_vmcs->gs, tr_limit, GUEST_TR_LIMIT);
  VMCS_ENC(host_vmcs->gs, gdtr_limit, GUEST_GDTR_LIMIT);
  VMCS_ENC(host_vmcs->gs, idtr_limit, GUEST_IDTR_LIMIT);
  VMCS_ENC(host_vmcs->gs, es_ar_bytes, GUEST_ES_AR_BYTES);
  VMCS_ENC(host_vmcs->gs, cs_ar_bytes, GUEST_CS_AR_BYTES);
  VMCS_ENC(host_vmcs->gs, ss_ar_bytes, GUEST_SS_AR_BYTES);
  VMCS_ENC(host_vmcs->gs, ds_ar_bytes, GUEST_DS_AR_BYTES);
  VMCS_ENC(host_vmcs->gs, fs_ar_bytes, GUEST_FS_AR_BYTES);
  VMCS_ENC(host_vmcs->gs, gs_ar_bytes, GUEST_GS_AR_BYTES);
  VMCS_ENC(host_vmcs->gs, ldtr_ar_bytes, GUEST_LDTR_AR_BYTES);
  VMCS_ENC(host_vmcs->gs, tr_ar_bytes, GUEST_TR_AR_BYTES);
  VMCS_ENC(host_vmcs->gs, interruptibility_info, GUEST_INTERRUPTIBILITY_INFO);
  VMCS_ENC(host_vmcs->gs, activity_state, GUEST_ACTIVITY_STATE);
  VMCS_ENC(host_vmcs->gs, smbase, GUEST_SMBASE);
  VMCS_ENC(host_vmcs->gs, sysenter_cs, GUEST_SYSENTER_CS);
  VMCS_ENC(host_vmcs->gs, vmcs_link_pointer, VMCS_LINK_POINTER);
  VMCS_ENC(host_vmcs->gs, interrupt_status, GUEST_INTERRUPT_STATUS);
  VMCS_ENC(host_vmcs->gs, vmx_preemption_timer_value, VMX_PREEMPTION_TIMER_VALUE);
  // Host state
  VMCS_ENC(host_vmcs->hs, es_selector, HOST_ES_SELECTOR);
  VMCS_ENC(host_vmcs->hs, cs_selector, HOST_CS_SELECTOR);
  VMCS_ENC(host_vmcs->hs, ss_selector, HOST_SS_SELECTOR);
  VMCS_ENC(host_vmcs->hs, ds_selector, HOST_DS_SELECTOR);
  VMCS_ENC(host_vmcs->hs, fs_selector, HOST_FS_SELECTOR);
  VMCS_ENC(host_vmcs->hs, gs_selector, HOST_GS_SELECTOR);
  VMCS_ENC(host_vmcs->hs, tr_selector, HOST_TR_SELECTOR);
  VMCS_ENC(host_vmcs->hs, ia32_pat, HOST_IA32_PAT);
  VMCS_ENC(host_vmcs->hs, ia32_efer, HOST_IA32_EFER);
  VMCS_ENC(host_vmcs->hs, ia32_perf_global_ctrl, HOST_IA32_PERF_GLOBAL_CTRL);
  VMCS_ENC(host_vmcs->hs, ia32_sysenter_cs, HOST_IA32_SYSENTER_CS);
  VMCS_ENC(host_vmcs->hs, cr0, HOST_CR0);
  VMCS_ENC(host_vmcs->hs, cr3, HOST_CR3);
  VMCS_ENC(host_vmcs->hs, cr4, HOST_CR4);
  VMCS_ENC(host_vmcs->hs, fs_base, HOST_FS_BASE);
  VMCS_ENC(host_vmcs->hs, gs_base, HOST_GS_BASE);
  VMCS_ENC(host_vmcs->hs, tr_base, HOST_TR_BASE);
  VMCS_ENC(host_vmcs->hs, gdtr_base, HOST_GDTR_BASE);
  VMCS_ENC(host_vmcs->hs, idtr_base, HOST_IDTR_BASE);
  VMCS_ENC(host_vmcs->hs, ia32_sysenter_esp, HOST_IA32_SYSENTER_ESP);
  VMCS_ENC(host_vmcs->hs, ia32_sysenter_eip, HOST_IA32_SYSENTER_EIP);
  VMCS_ENC(host_vmcs->hs, rsp, HOST_RSP);
  VMCS_ENC(host_vmcs->hs, rip, HOST_RIP);
  // Vm exit controls
  VMCS_ENC(host_vmcs->ctrls.exit, msr_store_addr, VM_EXIT_MSR_STORE_ADDR);
  VMCS_ENC(host_vmcs->ctrls.exit, msr_load_addr, VM_EXIT_MSR_LOAD_ADDR);
  VMCS_ENC(host_vmcs->ctrls.exit, controls, VM_EXIT_CONTROLS);
  VMCS_ENC(host_vmcs->ctrls.exit, msr_store_count, VM_EXIT_MSR_STORE_COUNT);
  VMCS_ENC(host_vmcs->ctrls.exit, msr_load_count, VM_EXIT_MSR_LOAD_COUNT);
  // VM entry controls
  VMCS_ENC(host_vmcs->ctrls.entry, msr_load_addr, VM_ENTRY_MSR_LOAD_ADDR);
  VMCS_ENC(host_vmcs->ctrls.entry, controls, VM_ENTRY_CONTROLS);
  VMCS_ENC(host_vmcs->ctrls.entry, msr_load_count, VM_ENTRY_MSR_LOAD_COUNT);
  VMCS_ENC(host_vmcs->ctrls.entry, intr_info_field, VM_ENTRY_INTR_INFO_FIELD);
  VMCS_ENC(host_vmcs->ctrls.entry, exception_error_code, VM_ENTRY_EXCEPTION_ERROR_CODE);
  VMCS_ENC(host_vmcs->ctrls.entry, instruction_len, VM_ENTRY_INSTRUCTION_LEN);
  // VM exit info
  VMCS_ENC(host_vmcs->info, guest_physical_address, GUEST_PHYSICAL_ADDRESS);
  VMCS_ENC(host_vmcs->info, vm_instruction_error, VM_INSTRUCTION_ERROR);
  VMCS_ENC(host_vmcs->info, reason, VM_EXIT_REASON);
  VMCS_ENC(host_vmcs->info, intr_info, VM_EXIT_INTR_INFO);
  VMCS_ENC(host_vmcs->info, intr_error_code, VM_EXIT_INTR_ERROR_CODE);
  VMCS_ENC(host_vmcs->info, idt_vectoring_info_field, IDT_VECTORING_INFO_FIELD);
  VMCS_ENC(host_vmcs->info, idt_vectoring_error_code, IDT_VECTORING_ERROR_CODE);
  VMCS_ENC(host_vmcs->info, instruction_len, VM_EXIT_INSTRUCTION_LEN);
  VMCS_ENC(host_vmcs->info, vmx_instruction_info, VMX_INSTRUCTION_INFO);
  VMCS_ENC(host_vmcs->info, qualification, EXIT_QUALIFICATION);
  VMCS_ENC(host_vmcs->info, io_rcx, IO_RCX);
  VMCS_ENC(host_vmcs->info, io_rsi, IO_RSI);
  VMCS_ENC(host_vmcs->info, io_rdi, IO_RDI);
  VMCS_ENC(host_vmcs->info, io_rip, IO_RIP);
  VMCS_ENC(host_vmcs->info, guest_linear_address, GUEST_LINEAR_ADDRESS);
  // Copying the encodings
  for (i = 0; i < VM_NB; i++) {
    memcpy(&vmcs_cache_pool[i], host_vmcs, sizeof(struct vmcs));
  }
}

void vmcs_init(void) {
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
}

void vmcs_fill_guest_state_fields(void) {
  struct gdt_entry gdt_entry;
  struct idt_ptr idt_ptr;
  uint64_t msr;
  uint64_t sel;

  cpu_vmwrite(GUEST_CR0, cpu_adjust64(cpu_read_cr0(), MSR_ADDRESS_VMX_CR0_FIXED0, MSR_ADDRESS_VMX_CR0_FIXED1));
  cpu_vmwrite(GUEST_CR3, cpu_read_cr3());
  cpu_vmwrite(GUEST_CR4, cpu_adjust64(cpu_read_cr4(), MSR_ADDRESS_VMX_CR4_FIXED0, MSR_ADDRESS_VMX_CR4_FIXED1));
  cpu_vmwrite(GUEST_DR7, cpu_read_dr7());
  cpu_vmwrite(GUEST_RFLAGS, (1 << 1));

  sel = cpu_read_cs();
  gdt_get_guest_entry(sel, &gdt_entry);
  cpu_vmwrite(GUEST_CS_SELECTOR, sel);
  cpu_vmwrite(GUEST_CS_BASE, gdt_entry.base);
  cpu_vmwrite(GUEST_CS_LIMIT, gdt_entry.limit);
  cpu_vmwrite(GUEST_CS_AR_BYTES, (gdt_entry.granularity << 8) | (gdt_entry.access << 0));
  sel = cpu_read_ss();
  gdt_get_guest_entry(sel, &gdt_entry);
  cpu_vmwrite(GUEST_SS_SELECTOR, sel);
  cpu_vmwrite(GUEST_SS_BASE, gdt_entry.base);
  cpu_vmwrite(GUEST_SS_LIMIT, gdt_entry.limit);
  cpu_vmwrite(GUEST_SS_AR_BYTES, (gdt_entry.granularity << 8) | (gdt_entry.access << 0));
  sel = cpu_read_ds();
  gdt_get_guest_entry(sel, &gdt_entry);
  cpu_vmwrite(GUEST_DS_SELECTOR, sel);
  cpu_vmwrite(GUEST_DS_BASE, gdt_entry.base);
  cpu_vmwrite(GUEST_DS_LIMIT, gdt_entry.limit);
  cpu_vmwrite(GUEST_DS_AR_BYTES, (gdt_entry.granularity << 8) | (gdt_entry.access << 0));
  sel = cpu_read_es();
  gdt_get_guest_entry(sel, &gdt_entry);
  cpu_vmwrite(GUEST_ES_SELECTOR, sel);
  cpu_vmwrite(GUEST_ES_BASE, gdt_entry.base);
  cpu_vmwrite(GUEST_ES_LIMIT, gdt_entry.limit);
  cpu_vmwrite(GUEST_ES_AR_BYTES, (gdt_entry.granularity << 8) | (gdt_entry.access << 0));
  sel = cpu_read_fs();
  gdt_get_guest_entry(sel, &gdt_entry);
  cpu_vmwrite(GUEST_FS_SELECTOR, sel);
  cpu_vmwrite(GUEST_FS_BASE, gdt_entry.base);
  cpu_vmwrite(GUEST_FS_LIMIT, gdt_entry.limit);
  cpu_vmwrite(GUEST_FS_AR_BYTES, (gdt_entry.granularity << 8) | (gdt_entry.access << 0));
  sel = cpu_read_gs();
  gdt_get_guest_entry(sel, &gdt_entry);
  cpu_vmwrite(GUEST_GS_SELECTOR, sel);
  cpu_vmwrite(GUEST_GS_BASE, gdt_entry.base);
  cpu_vmwrite(GUEST_GS_LIMIT, gdt_entry.limit);
  cpu_vmwrite(GUEST_GS_AR_BYTES, (gdt_entry.granularity << 8) | (gdt_entry.access << 0));

  /* TODO Verify if these values are correct. */
  cpu_vmwrite(GUEST_LDTR_SELECTOR, 0);
  cpu_vmwrite(GUEST_LDTR_BASE, 0);
  cpu_vmwrite(GUEST_LDTR_LIMIT, 0xffff);
  cpu_vmwrite(GUEST_LDTR_AR_BYTES, 0x82);
  cpu_vmwrite(GUEST_TR_SELECTOR, 0);
  cpu_vmwrite(GUEST_TR_BASE, 0);
  cpu_vmwrite(GUEST_TR_LIMIT, 0xffff);
  cpu_vmwrite(GUEST_TR_AR_BYTES, 0x8b);

  cpu_vmwrite(GUEST_GDTR_BASE, gdt_get_guest_base());
  cpu_vmwrite(GUEST_GDTR_LIMIT, gdt_get_guest_limit());

  cpu_read_idt((uint8_t *) &idt_ptr);
  cpu_vmwrite(GUEST_IDTR_BASE, idt_ptr.base);
  cpu_vmwrite(GUEST_IDTR_LIMIT, idt_ptr.limit);

  cpu_vmwrite(GUEST_IA32_DEBUGCTL, msr_read(MSR_ADDRESS_IA32_DEBUGCTL));
  cpu_vmwrite(GUEST_SYSENTER_CS, msr_read(MSR_ADDRESS_IA32_SYSENTER_CS));

  msr = msr_read(MSR_ADDRESS_IA32_EFER);
  cpu_vmwrite(GUEST_IA32_EFER, msr),

  cpu_vmwrite(GUEST_ACTIVITY_STATE, 0);
  cpu_vmwrite(GUEST_INTERRUPTIBILITY_INFO, 0);
  cpu_vmwrite(GUEST_PENDING_DBG_EXCEPTIONS, 0);
  cpu_vmwrite(VMCS_LINK_POINTER, 0xffffffffffffffff);

  cpu_vmwrite(GUEST_IA32_PERF_GLOBAL_CTRL, msr_read(MSR_ADDRESS_IA32_PERF_GLOBAL_CTRL));

  // Init and compute vmx_preemption_timer_value
  tsc_freq_MHz = ((msr_read(MSR_ADDRESS_MSR_PLATFORM_INFO) >> 8) & 0xff) * 100;
  tsc_divider = msr_read(MSR_ADDRESS_IA32_VMX_MISC) & 0x7;
  vmcs_set_vmx_preemption_timer_value(VMCS_DEFAULT_PREEMPTION_TIMER_MICROSEC);
}

void vmcs_fill_host_state_fields(void) {
  uint64_t sel;
  struct gdt_entry gdt_entry;
  struct idt_ptr idt_ptr;

  cpu_vmwrite(HOST_CR0, cpu_adjust64(cpu_read_cr0(), MSR_ADDRESS_VMX_CR0_FIXED0, MSR_ADDRESS_VMX_CR0_FIXED1));
  // cpu_vmwrite(HOST_CR3, cpu_read_cr3());
  cpu_vmwrite(HOST_CR3, paging_get_host_cr3());
  // TODO Bit 18 OSXSAVE Activation du XCR0 -> proxifier le XSETBV
  cpu_vmwrite(HOST_CR4, cpu_adjust64(cpu_read_cr4() | (1 << 18), MSR_ADDRESS_VMX_CR4_FIXED0, MSR_ADDRESS_VMX_CR4_FIXED1));
  cpu_vmwrite(HOST_RSP, (uint64_t) &vmm_stack[VMM_STACK_SIZE]);
  cpu_vmwrite(HOST_RIP, (uint64_t) vmm_vm_exit_handler);

  cpu_vmwrite(HOST_CS_SELECTOR, cpu_read_cs() & 0xf8);
  cpu_vmwrite(HOST_SS_SELECTOR, cpu_read_ss() & 0xf8);
  cpu_vmwrite(HOST_DS_SELECTOR, cpu_read_ds() & 0xf8);
  cpu_vmwrite(HOST_ES_SELECTOR, cpu_read_es() & 0xf8);
  cpu_vmwrite(HOST_FS_SELECTOR, cpu_read_fs() & 0xf8);
  cpu_vmwrite(HOST_GS_SELECTOR, cpu_read_gs() & 0xf8);
  cpu_vmwrite(HOST_TR_SELECTOR, 0x8 & 0xf8);

  sel = cpu_read_fs();
  gdt_get_host_entry(sel, &gdt_entry);
  cpu_vmwrite(HOST_FS_BASE, gdt_entry.base);
  sel = cpu_read_gs();
  gdt_get_host_entry(sel, &gdt_entry);
  cpu_vmwrite(HOST_GS_BASE, gdt_entry.base);
  sel = 0x8;
  gdt_get_host_entry(sel, &gdt_entry);
  cpu_vmwrite(HOST_TR_BASE, gdt_entry.base);

  cpu_vmwrite(HOST_GDTR_BASE, gdt_get_host_base());

  idt_get_idt_ptr(&idt_ptr);
  // cpu_read_idt((uint8_t *) &idt_ptr);
  cpu_vmwrite(HOST_IDTR_BASE, idt_ptr.base);

  cpu_vmwrite(HOST_IA32_SYSENTER_CS, msr_read(MSR_ADDRESS_IA32_SYSENTER_CS));
  cpu_vmwrite(HOST_IA32_SYSENTER_ESP, msr_read(MSR_ADDRESS_IA32_SYSENTER_ESP));
  cpu_vmwrite(HOST_IA32_SYSENTER_EIP, msr_read(MSR_ADDRESS_IA32_SYSENTER_EIP));
  cpu_vmwrite(HOST_IA32_EFER, msr_read(MSR_ADDRESS_IA32_EFER)),

  cpu_vmwrite(HOST_IA32_PERF_GLOBAL_CTRL, 0);
}

void vmcs_fill_vm_exec_control_fields(void) {
  uint32_t procbased_ctls = ACT_SECONDARY_CONTROLS | USE_MSR_BITMAPS | USE_IO_BITMAPS | CR3_LOAD_EXITING;
  uint32_t procbased_ctls_2 = ENABLE_EPT | ENABLE_VPID | UNRESTRICTED_GUEST | ENABLE_RDTSCP;
                             
  uint64_t msr_bitmap_ptr;
  uint64_t eptp;
  uint64_t io_bitmap_ptr;
  uint32_t pinbased_ctls = ACT_VMX_PREEMPT_TIMER | NMI_EXITING;

  cpu_vmwrite(PIN_BASED_VM_EXEC_CONTROL, cpu_adjust32(pinbased_ctls, MSR_ADDRESS_IA32_VMX_PINBASED_CTLS));

  procbased_ctls = cpu_adjust32(procbased_ctls, MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS);
  if (((msr_read(MSR_ADDRESS_IA32_VMX_BASIC) >> 55) & 1)                       // extra capabilities enabled
  && (((msr_read(MSR_ADDRESS_IA32_VMX_TRUE_PROCBASED_CTLS) >> 15) & 3)) == 0){ // CR3_LOAD_EXITING & CR3_STORE_EXITING can be disabled
    procbased_ctls &= ~(CR3_LOAD_EXITING | CR3_STORE_EXITING);
  } else {
    panic("#!PROCBASED_CTLS CR3_LOAD_EXITING or CR3_STORE_EXITING couldn't be disabled\n");
  }
  cpu_vmwrite(CPU_BASED_VM_EXEC_CONTROL, procbased_ctls);

  cpu_vmwrite(SECONDARY_VM_EXEC_CONTROL, cpu_adjust32(procbased_ctls_2, MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS2));

  cpu_vmwrite(EXCEPTION_BITMAP, 0);
  cpu_vmwrite(PAGE_FAULT_ERROR_CODE_MASK, 0);
  cpu_vmwrite(PAGE_FAULT_ERROR_CODE_MATCH, 0);

  io_bitmap_ptr = io_bitmap_get_ptr_a();
  cpu_vmwrite(IO_BITMAP_A, io_bitmap_ptr);
  io_bitmap_ptr = io_bitmap_get_ptr_b();
  cpu_vmwrite(IO_BITMAP_B, io_bitmap_ptr);

  cpu_vmwrite(TSC_OFFSET, 0);

  // As we are using UNRESTRICTED_GUEST procbased_ctrl, the guest can itself modify CR0.PE and CR0.PG, see doc INTEL vol 3C chap 23.8
  cpu_vmwrite(CR0_GUEST_HOST_MASK, (msr_read(MSR_ADDRESS_VMX_CR0_FIXED0) | (~msr_read(MSR_ADDRESS_VMX_CR0_FIXED1))) & ~(0x80000001));
  cpu_vmwrite(CR0_READ_SHADOW, cpu_read_cr0());
  cpu_vmwrite(CR4_GUEST_HOST_MASK, msr_read(MSR_ADDRESS_VMX_CR4_FIXED0) | ~msr_read(MSR_ADDRESS_VMX_CR4_FIXED1));
  cpu_vmwrite(CR4_READ_SHADOW, cpu_read_cr4() & ~(uint64_t)(1<<13)); // We hide CR4.VMXE

  cpu_vmwrite(CR3_TARGET_COUNT, 0);
  cpu_vmwrite(CR3_TARGET_VALUE0, 0);
  cpu_vmwrite(CR3_TARGET_VALUE1, 0);
  cpu_vmwrite(CR3_TARGET_VALUE2, 0);
  cpu_vmwrite(CR3_TARGET_VALUE3, 0);

  msr_bitmap_ptr = msr_bitmap_get_ptr();
  cpu_vmwrite(MSR_BITMAP, msr_bitmap_ptr);

  eptp = ept_get_eptp();

  cpu_vmwrite(EPT_POINTER, eptp);
  cpu_vmwrite(VIRTUAL_PROCESSOR_ID, 0x1); // vmcs0 is 1

  // XXX TEST virtual APIC
  cpu_vmwrite(VIRTUAL_APIC_PAGE_ADDR, ((uintptr_t)&vapic[0]));
}

void vmcs_fill_vm_exit_control_fields(void) {
  uint32_t exit_controls = EXIT_SAVE_IA32_EFER | EXIT_LOAD_IA32_EFER |
    EXIT_LOAD_IA32_PERF_GLOBAL_CTRL | HOST_ADDR_SPACE_SIZE |
    SAVE_VMX_PREEMPT_TIMER_VAL;
  cpu_vmwrite(VM_EXIT_CONTROLS, cpu_adjust32(exit_controls, MSR_ADDRESS_IA32_VMX_EXIT_CTLS));
  cpu_vmwrite(VM_EXIT_MSR_STORE_COUNT, 0);
  cpu_vmwrite(VM_EXIT_MSR_LOAD_COUNT, 0);
}

void vmcs_fill_vm_entry_control_fields(void) {
  uint32_t entry_controls = ENTRY_LOAD_IA32_EFER | ENTRY_LOAD_IA32_PERF_GLOBAL_CTRL | IA32E_MODE_GUEST;
  cpu_vmwrite(VM_ENTRY_CONTROLS, cpu_adjust32(entry_controls, MSR_ADDRESS_IA32_VMX_ENTRY_CTLS));
  cpu_vmwrite(VM_ENTRY_MSR_LOAD_COUNT, 0);
  cpu_vmwrite(VM_ENTRY_INTR_INFO_FIELD, 0);
}

/* Dump a section of VMCS */
void print_section(char *header, uint32_t start, uint32_t end,
    int incr) {
  uint32_t addr, j;
  unsigned long val;
  int code, rc;
  char *fmt[4] = {"0x%04x ", "0x%016x ", "0x%08x ", "0x%016x "};
  char *err[4] = {"------ ", "------------------ ",
                  "---------- ", "------------------ "};
  /* Find width of the field (encoded in bits 14:13 of address) */
  code = (start>>13)&3;
  if (header)
    printk("\t %s", header);
  for (addr=start, j=0; addr<=end; addr+=incr, j++) {
    if (!(j&3))
      printk("\n\t\t0x%08x: ", addr);
    val = cpu_vmread_safe(addr, (long unsigned int *)&rc);
    if (val != 0)
      printk(fmt[code], rc);
    else
      printk("%s", err[code]);
  }
  printk("\n");
}

void vmcs_dump_vcpu(void)
{
  print_section("16-bit Control Fields", 0x000, 0x004, 2);
  print_section("16-bit Guest-State Fields", 0x800, 0x810, 2);
  print_section("16-bit Host-State Fields", 0xc00, 0xc0c, 2);
  print_section("64-bit Control Fields", 0x2000, 0x202d, 1);
  print_section("64-bit RO Data Fields", 0x2400, 0x2401, 1);
  print_section("64-bit Guest-State Fields", 0x2800, 0x2811, 1);
  print_section("64-bit Host-State Fields", 0x2c00, 0x2c05, 1);
  print_section("32-bit Control Fields", 0x4000, 0x4022, 2);
  print_section("32-bit RO Data Fields", 0x4400, 0x440e, 2);
  print_section("32-bit Guest-State Fields", 0x4800, 0x482e, 2);
  print_section("32-bit Host-State Fields", 0x4c00, 0x4c00, 2);
  print_section("Natural 64-bit Control Fields", 0x6000, 0x600e, 2);
  print_section("Natural RO Data Fields", 0x6400, 0x640a, 2);
  print_section("Natural 64-bit Guest-State Fields", 0x6800, 0x6826, 2);
  print_section("Natural 64-bit Host-State Fields", 0x6c00, 0x6c16, 2);
}

void vmcs_set_vmx_preemption_timer_value(uint64_t time_microsec) {
  cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE, (tsc_freq_MHz * time_microsec) >> tsc_divider);
}
