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

#ifndef __VMM_VMCS_H__
#define __VMM_VMCS_H__

#include "cpu.h"
#include "msr.h"

#define VMCS_DEFAULT_PREEMPTION_TIMER_MICROSEC 5*1000000
#define NB_VMCS_FIELDS 185

enum vmcs_field {                                 // ▼ require support of ▼
  /* 16-BIT FIELDS */
  /* 16-Bit Control Fields */
  VIRTUAL_PROCESSOR_ID             = 0x00000000,  // ENABLE_VPID
  POSTED_INT_NOTIF_VECTOR          = 0x00000002,  // PROCESS_POSTED_INTR
  EPTP_INDEX                       = 0x00000004,  // SECONDARY_VM_EXEC_CONTROL.EPT_VIOLATION_VE
  /* 16-Bit Guest-State Fields */
  GUEST_ES_SELECTOR                = 0x00000800,
  GUEST_CS_SELECTOR                = 0x00000802,
  GUEST_SS_SELECTOR                = 0x00000804,
  GUEST_DS_SELECTOR                = 0x00000806,
  GUEST_FS_SELECTOR                = 0x00000808,
  GUEST_GS_SELECTOR                = 0x0000080a,
  GUEST_LDTR_SELECTOR              = 0x0000080c,
  GUEST_TR_SELECTOR                = 0x0000080e,
  GUEST_INTERRUPT_STATUS           = 0x00000810,  // SECONDARY_VM_EXEC_CONTROL.VIRT_INTR_DELIVERY
  /* 16-Bit Host-State Fields */
  HOST_ES_SELECTOR                 = 0x00000c00,
  HOST_CS_SELECTOR                 = 0x00000c02,
  HOST_SS_SELECTOR                 = 0x00000c04,
  HOST_DS_SELECTOR                 = 0x00000c06,
  HOST_FS_SELECTOR                 = 0x00000c08,
  HOST_GS_SELECTOR                 = 0x00000c0a,
  HOST_TR_SELECTOR                 = 0x00000c0c,
  /* 64-BIT FIELDS */
  /* 64-Bit Control Fields */
  IO_BITMAP_A                      = 0x00002000,
  IO_BITMAP_A_HIGH                 = 0x00002001,
  IO_BITMAP_B                      = 0x00002002,
  IO_BITMAP_B_HIGH                 = 0x00002003,
  MSR_BITMAP                       = 0x00002004,  // CPU_BASED_VM_EXEC_CONTROL.USE_MSR_BITMAPS
  MSR_BITMAP_HIGH                  = 0x00002005,  //            ||
  VM_EXIT_MSR_STORE_ADDR           = 0x00002006,
  VM_EXIT_MSR_STORE_ADDR_HIGH      = 0x00002007,
  VM_EXIT_MSR_LOAD_ADDR            = 0x00002008,
  VM_EXIT_MSR_LOAD_ADDR_HIGH       = 0x00002009,
  VM_ENTRY_MSR_LOAD_ADDR           = 0x0000200a,
  VM_ENTRY_MSR_LOAD_ADDR_HIGH      = 0x0000200b,
  EXECUTIVE_VMCS_POINTER           = 0x0000200c,
  EXECUTIVE_VMCS_POINTER_HIGH      = 0x0000200d,
  TSC_OFFSET                       = 0x00002010,
  TSC_OFFSET_HIGH                  = 0x00002011,
  VIRTUAL_APIC_PAGE_ADDR           = 0x00002012,  // CPU_BASED_VM_EXEC_CONTROL.USE_TPR_SHADOW
  VIRTUAL_APIC_PAGE_ADDR_HIGH      = 0x00002013,  //            ||
  APIC_ACCESS_ADDR                 = 0x00002014,  // SECONDARY_VM_EXEC_CONTROL.VIRT_APIC_ACCESSES
  APIC_ACCESS_ADDR_HIGH            = 0x00002015,  //            ||
  POSTED_INTR_DESC_ADDR            = 0x00002016,  // PIN_BASED_VM_EXEC_CONTROL.PROCESS_POSTED_INTR
  POSTED_INTR_DESC_ADDR_HIGH       = 0x00002017,  //            ||
  VM_FUNCTION_CONTROLS             = 0x00002018,  // SECONDARY_VM_EXEC_CONTROL.ENABLE_VM_FUNCTIONS
  VM_FUNCTION_CONTROLS_HIGH        = 0x00002019,  //            ||
  EPT_POINTER                      = 0x0000201a,  // SECONDARY_VM_EXEC_CONTROL.ENABLE_EPT
  EPT_POINTER_HIGH                 = 0x0000201b,  //            ||
  EOI_EXIT_BITMAP_0                = 0x0000201c,  // SECONDARY_VM_EXEC_CONTROL.VIRT_INTR_DELIVERY
  EOI_EXIT_BITMAP_0_HIGH           = 0x0000201d,  //            ||
  EOI_EXIT_BITMAP_1                = 0x0000201e,  //            ||
  EOI_EXIT_BITMAP_1_HIGH           = 0x0000201f,  //            ||
  EOI_EXIT_BITMAP_2                = 0x00002020,  //            ||
  EOI_EXIT_BITMAP_2_HIGH           = 0x00002021,  //            ||
  EOI_EXIT_BITMAP_3                = 0x00002022,  //            ||
  EOI_EXIT_BITMAP_3_HIGH           = 0x00002023,  //            ||
  EPTP_LIST_ADDR                   = 0x00002024,  // VM_FUNCTION_CONTROLS[0] (EPTP_switching)
  EPTP_LIST_ADDR_HIGH              = 0x00002025,  //            ||
  VMREAD_BITMAP_ADDR               = 0x00002026,  // SECONDARY_VM_EXEC_CONTROL.VMCS_SHADOWING
  VMREAD_BITMAP_ADDR_HIGH          = 0x00002027,  //            ||
  VMWRITE_BITMAP_ADDR              = 0x00002028,  //            ||
  VMWRITE_BITMAP_ADDR_HIGH         = 0x00002029,  //            ||
  VIRT_EXCEP_INFO_ADDR             = 0x0000202a,  // SECONDARY_VM_EXEC_CONTROL.EPT_VIOLATION_VE
  VIRT_EXCEP_INFO_ADDR_HIGH        = 0x0000202b,  //            ||
  XSS_EXITING_BITMAP               = 0x0000202c,  // SECONDARY_VM_EXEC_CONTROL.EN_XSAVES_XRSTORS
  XSS_EXITING_BITMAP_HIGH          = 0x0000202d,  //            ||
  /* 64-Bit Read-Only Data Field */
  GUEST_PHYSICAL_ADDRESS           = 0x00002400,  // SECONDARY_VM_EXEC_CONTROL.ENABLE_EPT
  GUEST_PHYSICAL_ADDRESS_HIGH      = 0x00002401,  //            ||
  /* 64-Bit Guest-State Fields */
  VMCS_LINK_POINTER                = 0x00002800,
  VMCS_LINK_POINTER_HIGH           = 0x00002801,
  GUEST_IA32_DEBUGCTL              = 0x00002802,
  GUEST_IA32_DEBUGCTL_HIGH         = 0x00002803,
  GUEST_IA32_PAT                   = 0x00002804,  // VM_ENTRY_CONTROLS.ENTRY_LOAD_IA32_PAT or VM_EXIT_CONTROLS.EXIT_SAVE_IA32_PAT
  GUEST_IA32_PAT_HIGH              = 0x00002805,  //            ||
  GUEST_IA32_EFER                  = 0x00002806,  // VM_ENTRY_CONTROLS.ENTRY_LOAD_IA32_EFER or VM_EXIT_CONTROLS.EXIT_SAVE_IA32_EFER
  GUEST_IA32_EFER_HIGH             = 0x00002807,  //            ||
  GUEST_IA32_PERF_GLOBAL_CTRL      = 0x00002808,  // VM_ENTRY_CONTROLS.ENTRY_LOAD_IA32_PERF_GLOBAL_CTRL
  GUEST_IA32_PERF_GLOBAL_CTRL_HIGH = 0x00002809,  //            ||
  GUEST_PDPTR0                     = 0x0000280a,  // SECONDARY_VM_EXEC_CONTROL.ENABLE_EPT
  GUEST_PDPTR0_HIGH                = 0x0000280b,  //            ||
  GUEST_PDPTR1                     = 0x0000280c,  //            ||
  GUEST_PDPTR1_HIGH                = 0x0000280d,  //            ||
  GUEST_PDPTR2                     = 0x0000280e,  //            ||
  GUEST_PDPTR2_HIGH                = 0x0000280f,  //            ||
  GUEST_PDPTR3                     = 0x00002810,  //            ||
  GUEST_PDPTR3_HIGH                = 0x00002811,  //            ||
  /* 64-Bit Host-State Fields */
  HOST_IA32_PAT                    = 0x00002c00,  // VM_EXIT_CONTROLS.EXIT_LOAD_IA32_PAT
  HOST_IA32_PAT_HIGH               = 0x00002c01,  //            ||
  HOST_IA32_EFER                   = 0x00002c02,  // VM_EXIT_CONTROLS.EXIT_LOAD_IA32_EFER
  HOST_IA32_EFER_HIGH              = 0x00002c03,  //            ||
  HOST_IA32_PERF_GLOBAL_CTRL       = 0x00002c04,  // VM_EXIT_CONTROLS.EXIT_LOAD_IA32_PERF_GLOBAL_CTRL
  HOST_IA32_PERF_GLOBAL_CTRL_HIGH  = 0x00002c05,  //            ||
  /* 32-BIT FIELDS */
  /* 32-Bit Control Fields */
  PIN_BASED_VM_EXEC_CONTROL        = 0x00004000,
  CPU_BASED_VM_EXEC_CONTROL        = 0x00004002,
  EXCEPTION_BITMAP                 = 0x00004004,
  PAGE_FAULT_ERROR_CODE_MASK       = 0x00004006,
  PAGE_FAULT_ERROR_CODE_MATCH      = 0x00004008,
  CR3_TARGET_COUNT                 = 0x0000400a,
  VM_EXIT_CONTROLS                 = 0x0000400c,
  VM_EXIT_MSR_STORE_COUNT          = 0x0000400e,
  VM_EXIT_MSR_LOAD_COUNT           = 0x00004010,
  VM_ENTRY_CONTROLS                = 0x00004012,
  VM_ENTRY_MSR_LOAD_COUNT          = 0x00004014,
  VM_ENTRY_INTR_INFO_FIELD         = 0x00004016,
  VM_ENTRY_EXCEPTION_ERROR_CODE    = 0x00004018,
  VM_ENTRY_INSTRUCTION_LEN         = 0x0000401a,
  TPR_THRESHOLD                    = 0x0000401c,  // CPU_BASED_VM_EXEC_CONTROL.USE_TPR_SHADOW
  SECONDARY_VM_EXEC_CONTROL        = 0x0000401e,  // CPU_BASED_VM_EXEC_CONTROL.ACT_SECONDARY_CONTROLS
  PLE_GAP                          = 0x00004020,  // SECONDARY_VM_EXEC_CONTROL.PAUSE_LOOP_EXITING
  PLE_WINDOW                       = 0x00004022,  //            ||
  /* 32-Bit Read-Only Data Fields */
  VM_INSTRUCTION_ERROR             = 0x00004400,
  VM_EXIT_REASON                   = 0x00004402,
  VM_EXIT_INTR_INFO                = 0x00004404,
  VM_EXIT_INTR_ERROR_CODE          = 0x00004406,
  IDT_VECTORING_INFO_FIELD         = 0x00004408,
  IDT_VECTORING_ERROR_CODE         = 0x0000440a,
  VM_EXIT_INSTRUCTION_LEN          = 0x0000440c,
  VMX_INSTRUCTION_INFO             = 0x0000440e,
  /* 32-Bit Guest-State Fields */
  GUEST_ES_LIMIT                   = 0x00004800,
  GUEST_CS_LIMIT                   = 0x00004802,
  GUEST_SS_LIMIT                   = 0x00004804,
  GUEST_DS_LIMIT                   = 0x00004806,
  GUEST_FS_LIMIT                   = 0x00004808,
  GUEST_GS_LIMIT                   = 0x0000480a,
  GUEST_LDTR_LIMIT                 = 0x0000480c,
  GUEST_TR_LIMIT                   = 0x0000480e,
  GUEST_GDTR_LIMIT                 = 0x00004810,
  GUEST_IDTR_LIMIT                 = 0x00004812,
  GUEST_ES_AR_BYTES                = 0x00004814,
  GUEST_CS_AR_BYTES                = 0x00004816,
  GUEST_SS_AR_BYTES                = 0x00004818,
  GUEST_DS_AR_BYTES                = 0x0000481a,
  GUEST_FS_AR_BYTES                = 0x0000481c,
  GUEST_GS_AR_BYTES                = 0x0000481e,
  GUEST_LDTR_AR_BYTES              = 0x00004820,
  GUEST_TR_AR_BYTES                = 0x00004822,
  GUEST_INTERRUPTIBILITY_INFO      = 0x00004824,
  GUEST_ACTIVITY_STATE             = 0x00004826,
  GUEST_SMBASE                     = 0x00004828,
  GUEST_SYSENTER_CS                = 0x0000482a,
  VMX_PREEMPTION_TIMER_VALUE       = 0x0000482e,  // PIN_BASED_VM_EXEC_CONTROL.ACT_VMX_PREEMPT_TIMER
  /* 32-Bit Host-State Field */
  HOST_IA32_SYSENTER_CS            = 0x00004c00,
  /* NATURAL-WIDTH FIELDS */
  /* Natural-Width Control Fields */
  CR0_GUEST_HOST_MASK              = 0x00006000,
  CR4_GUEST_HOST_MASK              = 0x00006002,
  CR0_READ_SHADOW                  = 0x00006004,
  CR4_READ_SHADOW                  = 0x00006006,
  CR3_TARGET_VALUE0                = 0x00006008,
  CR3_TARGET_VALUE1                = 0x0000600a,
  CR3_TARGET_VALUE2                = 0x0000600c,
  CR3_TARGET_VALUE3                = 0x0000600e,
  /* Natural-Width Read-Only Data Fields */
  EXIT_QUALIFICATION               = 0x00006400,
  IO_RCX                           = 0x00006402,
  IO_RSI                           = 0x00006404,
  IO_RDI                           = 0x00006406,
  IO_RIP                           = 0x00006408,
  GUEST_LINEAR_ADDRESS             = 0x0000640a,
  /* Natural-Width Guest-State Fields */
  GUEST_CR0                        = 0x00006800,
  GUEST_CR3                        = 0x00006802,
  GUEST_CR4                        = 0x00006804,
  GUEST_ES_BASE                    = 0x00006806,
  GUEST_CS_BASE                    = 0x00006808,
  GUEST_SS_BASE                    = 0x0000680a,
  GUEST_DS_BASE                    = 0x0000680c,
  GUEST_FS_BASE                    = 0x0000680e,
  GUEST_GS_BASE                    = 0x00006810,
  GUEST_LDTR_BASE                  = 0x00006812,
  GUEST_TR_BASE                    = 0x00006814,
  GUEST_GDTR_BASE                  = 0x00006816,
  GUEST_IDTR_BASE                  = 0x00006818,
  GUEST_DR7                        = 0x0000681a,
  GUEST_RSP                        = 0x0000681c,
  GUEST_RIP                        = 0x0000681e,
  GUEST_RFLAGS                     = 0x00006820,
  GUEST_PENDING_DBG_EXCEPTIONS     = 0x00006822,
  GUEST_SYSENTER_ESP               = 0x00006824,
  GUEST_SYSENTER_EIP               = 0x00006826,
  /* Natural-Width Host-State Fields */
  HOST_CR0                         = 0x00006c00,
  HOST_CR3                         = 0x00006c02,
  HOST_CR4                         = 0x00006c04,
  HOST_FS_BASE                     = 0x00006c06,
  HOST_GS_BASE                     = 0x00006c08,
  HOST_TR_BASE                     = 0x00006c0a,
  HOST_GDTR_BASE                   = 0x00006c0c,
  HOST_IDTR_BASE                   = 0x00006c0e,
  HOST_IA32_SYSENTER_ESP           = 0x00006c10,
  HOST_IA32_SYSENTER_EIP           = 0x00006c12,
  HOST_RSP                         = 0x00006c14,
  HOST_RIP                         = 0x00006c16,
};

enum pin_based_vm_exec_control {
  EXT_INTR_EXITING      = (1 << 0),
  NMI_EXITING           = (1 << 3),
  VIRTUAL_NMIS          = (1 << 5),
  ACT_VMX_PREEMPT_TIMER = (1 << 6),
  PROCESS_POSTED_INTR   = (1 << 7),
};

enum proc_based_vm_exec_control {
  INTR_WINDOW_EXITING    = (1 << 2),
  USE_TSC_OFFSETTING     = (1 << 3),
  HLT_EXITING            = (1 << 7),
  INVLPG_EXITING         = (1 << 9),
  MWAIT_EXITING          = (1 << 10),
  RDPMC_EXITING          = (1 << 11),
  RDTSC_EXITING          = (1 << 12),
  CR3_LOAD_EXITING       = (1 << 15),
  CR3_STORE_EXITING      = (1 << 16),
  CR8_LOAD_EXITING       = (1 << 19),
  CR8_STORE_EXITING      = (1 << 20),
  USE_TPR_SHADOW         = (1 << 21),
  NMI_WINDOW_EXITING     = (1 << 22),
  MOV_DR_EXITING         = (1 << 23),
  UNCOND_IO_EXITING      = (1 << 24),
  USE_IO_BITMAPS         = (1 << 25),
  MONITOR_TRAP_FLAG      = (1 << 27),
  USE_MSR_BITMAPS        = (1 << 28),
  MONITOR_EXITING        = (1 << 29),
  PAUSE_EXITING          = (1 << 30),
  ACT_SECONDARY_CONTROLS = (1 << 31),
};

enum secondary_proc_based_vm_exec_control {
  VIRT_APIC_ACCESSES       = (1 << 0),
  ENABLE_EPT               = (1 << 1),
  DESCRIPTOR_TABLE_EXITING = (1 << 2),
  ENABLE_RDTSCP            = (1 << 3),
  VIRT_X2APIC_MODE         = (1 << 4),
  ENABLE_VPID              = (1 << 5),
  WBINVD_EXITING           = (1 << 6),
  UNRESTRICTED_GUEST       = (1 << 7),
  APIC_REGISTER_VIRT       = (1 << 8),
  VIRT_INTR_DELIVERY       = (1 << 9),
  PAUSE_LOOP_EXITING       = (1 << 10),
  RDRAND_EXITING           = (1 << 11),
  ENABLE_INVPCID           = (1 << 12),
  ENABLE_VM_FUNCTIONS      = (1 << 13),
  VMCS_SHADOWING           = (1 << 14),
  RDSEED_EXITING           = (1 << 16),
  EPT_VIOLATION_VE         = (1 << 18),
  EN_XSAVES_XRSTORS        = (1 << 20),
};

enum vm_exit_control {
  SAVE_DEBUG_CONTROLS             = (1 << 2),
  HOST_ADDR_SPACE_SIZE            = (1 << 9),
  EXIT_LOAD_IA32_PERF_GLOBAL_CTRL = (1 << 12),
  ACK_INTR_ON_EXIT                = (1 << 15),
  EXIT_SAVE_IA32_PAT              = (1 << 18),
  EXIT_LOAD_IA32_PAT              = (1 << 19),
  EXIT_SAVE_IA32_EFER             = (1 << 20),
  EXIT_LOAD_IA32_EFER             = (1 << 21),
  SAVE_VMX_PREEMPT_TIMER_VAL      = (1 << 22),
};

enum vm_entry_control {
  LOAD_DEBUG_CONTROLS              = (1 << 2),
  IA32E_MODE_GUEST                 = (1 << 9),
  ENTRY_TO_SMM                     = (1 << 10),
  DEACT_DUAL_MONITOR_TREATMENT     = (1 << 11),
  ENTRY_LOAD_IA32_PERF_GLOBAL_CTRL = (1 << 13),
  ENTRY_LOAD_IA32_PAT              = (1 << 14),
  ENTRY_LOAD_IA32_EFER             = (1 << 15),
};

enum vm_entry_interrupt_type {
  VM_ENTRY_INT_TYPE_EXT_INT              = 0,
  VM_ENTRY_INT_TYPE_RESERVED             = 1,
  VM_ENTRY_INT_TYPE_NMI                  = 2,
  VM_ENTRY_INT_TYPE_HW_EXCEPTION         = 3,
  VM_ENTRY_INT_TYPE_SOFT_INT             = 4,
  VM_ENTRY_INT_TYPE_PRIVILEGED_SOFT_INT  = 5,
  VM_ENTRY_INT_TYPE_SOFT_EXCEPTION       = 6,
  VM_ENTRY_INT_TYPE_OTHER                = 7
};

/**
 * VMCS Fields
 */

union vm_entry_interrupt_info {
  struct {
    uint32_t vector:8;
    uint32_t type:3;
    uint32_t deliver_error_code:1;
    uint32_t _r0:19;
    uint32_t valid:1;
  };
  uint32_t raw;
};

union vm_exit_interrupt_info {
  struct {
    uint32_t vector:8;
    uint32_t type:3;
    uint32_t error_code_valid:1;
    uint32_t nmi_blocking_due_to_iret:1;
    uint32_t _r0:18;
    uint32_t valid:1;
  };
  uint32_t raw;
};

union exit_reason {
  struct {
    uint32_t basic_exit_reason:16;
    uint32_t r0:12;
    uint32_t pending_mtf_vmxexit:1;
    uint32_t vmexit_from_vmxroot_operation:1;
    uint32_t r1:1;
    uint32_t vm_entry_failure:1;
  };
  uint32_t raw;
};

union exit_controls {
  struct {
    uint32_t r0:2;
    uint32_t save_debug_controls:1;
    uint32_t r1:6;
    uint32_t host_address_space_size:1;
    uint32_t r2:2;
    uint32_t load_ia32_perf_global_ctrl:1;
    uint32_t r3:2;
    uint32_t acknoledge_interrupt_on_exit:1;
    uint32_t r4:2;
    uint32_t save_ia32_pat:1;
    uint32_t load_ia32_pat:1;
    uint32_t save_ia32_efer:1;
    uint32_t load_ia32_efer:1;
    uint32_t save_vmx_preemption_timer_value:1;
    uint32_t r5:9;
  };
  uint32_t raw;
};

union entry_controls {
  struct {
    uint32_t r0:2;
    uint32_t load_debug_controls:1;
    uint32_t r1:6;
    uint32_t ia32_mode_guest:1;
    uint32_t entry_to_smm:1;
    uint32_t deactivate_dual_monitor_treatment:1;
    uint32_t r2:1;
    uint32_t load_ia32_perf_global_ctrl:1;
    uint32_t load_ia32_pat:1;
    uint32_t load_ia32_efer:1;
    uint32_t r3:16;
  };
  uint32_t raw;
};

union pin_based {
  struct {
    uint32_t external_interrupt_exiting:1;
    uint32_t r0:2;
    uint32_t nmi_exiting:1;
    uint32_t r1:1;
    uint32_t virtual_nmi:1;
    uint32_t activate_vmx_preemption_timer:1;
    uint32_t process_posted_interrupts:1;
    uint32_t r2:24;
  };
  uint32_t raw;
};

union proc_based {
  struct {
    uint32_t r0:2;
    uint32_t interrupt_window_exiting:1;
    uint32_t use_tsc_offsetting:1;
    uint32_t r1:3;
    uint32_t hlt_exiting:1;
    uint32_t r2:1;
    uint32_t invlpg_exiting:1;
    uint32_t mwait_exiting:1;
    uint32_t rdpmc_exiting:1;
    uint32_t rdtsc_exiting:1;
    uint32_t r3:2;
    uint32_t cr3_load_exiting:1;
    uint32_t cr3_store_exiting:1;
    uint32_t r4:2;
    uint32_t cr8_load_exiting:1;
    uint32_t cr8_store_exiting:1;
    uint32_t use_tpr_shadow:1;
    uint32_t nmi_window_exiting:1;
    uint32_t mov_dr_exiting:1;
    uint32_t unconditional_io_exiting:1;
    uint32_t use_io_bitmaps:1;
    uint32_t r5:1;
    uint32_t monitor_trap_flag:1;
    uint32_t use_msr_bitmaps:1;
    uint32_t monitor_exiting:1;
    uint32_t pause_exiting:1;
    uint32_t activate_secondary_controls:1;
  };
  uint32_t raw;
};

union proc_based_2 {
  struct {
    uint32_t virtualize_apic_access:1;
    uint32_t enable_ept:1;
    uint32_t descriptor_table_exiting:1;
    uint32_t enable_rdtscp:1;
    uint32_t virtualize_x2apic_mode:1;
    uint32_t enable_vpid:1;
    uint32_t wbinvd_exiting:1;
    uint32_t unrestricted_guest:1;
    uint32_t apic_register_virtualization:1;
    uint32_t virtual_interrupt_delivery:1;
    uint32_t pause_loop_exiting:1;
    uint32_t rdrand_exiting:1;
    uint32_t enable_invpcid:1;
    uint32_t enable_vm_functions:1;
    uint32_t vmcs_shadowing:1;
    uint32_t rdseed_exiting:1;
    uint32_t r0:1;
    uint32_t ept_violation_ve:1;
    uint32_t r1:1;
    uint32_t enable_xsaves_xrstors:1;
    uint32_t r2:12;
  };
  uint32_t raw;
};

union access_rights {
  struct {
    uint32_t type:4;
    uint32_t s:1;
    uint32_t dpl:2;
    uint32_t p:1;
    uint32_t r0:4;
    uint32_t avl:1;
    uint32_t l:1;
    uint32_t d:1;
    uint32_t g:1;
    uint32_t unusable:1;
    uint32_t r1:15;
  };
  struct {
    uint32_t :14;
    uint32_t b:1;
    uint32_t :17;
  };
  uint32_t raw;
};

struct field_16 {
  uint16_t raw;
};

struct field_32 {
  uint32_t raw;
};

struct field_64 {
  uint64_t raw;
};

struct field_signed_64 {
  int64_t raw;
};

/**
 * VMCS
 *
 * VMCS encoding handling has been inspired from ramooflax code
 */

#define VMC(__name__, __dst__, __src__) \
  (__dst__)->__name__##_enc.d = 1; \
  (__dst__)->__name__.raw = (__src__)->__name__.raw;
#define VMC2(__name_dst__, __name_src__, __dst__, __src__) \
  (__dst__)->__name_dst__##_enc.d = 1; \
  (__dst__)->__name_dst__.raw = (__src__)->__name_src__.raw;
#define VMD(__name__) \
  vmcs->__name__##_enc.d = 1;
#define VMD2(__vmcs__, __name__) \
  __vmcs__->__name__##_enc.d = 1;
#define VMW(__name__, __val__) \
  vmcs->__name__##_enc.d = 1; \
  vmcs->__name__.raw = (__val__);
#define VMW3(__vmcs__, __name__, __val__) \
  (__vmcs__)->__name__##_enc.d = 1; \
  (__vmcs__)->__name__.raw = (__val__);
#define VMCSF(__type__, __name__) \
  __type__ __name__; \
  union vmcs_field_encoding __name__##_enc
#define VMCSE(__vmcs__, __name__, __encoding__) \
  (__vmcs__)->__name__##_enc.raw = __encoding__; \
  (__vmcs__)->__name__##_enc.meta = 0x0;
#define VMR(__name__) \
  if (!vmcs->__name__##_enc.r && !vmcs->__name__##_enc.d) { \
    vmcs->__name__.raw = cpu_vmread(vmcs->__name__##_enc.raw); \
    vmcs->__name__##_enc.r = 1; \
  }
#define VMRF(__name__) \
  vmcs->__name__.raw = cpu_vmread(vmcs->__name__##_enc.raw); \
  vmcs->__name__##_enc.r = 1;
#define VMR2(__name__, __to__) \
  VMR(__name__) \
  (__to__) = vmcs->__name__.raw;
#define VMF(__name__) \
  vmcs->__name__##_enc.r = 0; \
  if (vmcs->__name__##_enc.d) { \
    vmcs->__name__##_enc.d = 0; \
    cpu_vmwrite(vmcs->__name__##_enc.raw, vmcs->__name__.raw); \
  }
#define VMP(__vmcs__, __name__) \
  if ((__vmcs__)->__name__##_enc.r || (__vmcs__)->__name__##_enc.d) { \
    printk("  "#__name__"[%x, %x]: 0x%016X\n", (__vmcs__)->__name__##_enc.d, \
        (__vmcs__)->__name__##_enc.r, (__vmcs__)->__name__.raw); \
  } else { \
    printk("  "#__name__"[%x, %x]: ------------------\n", \
        (__vmcs__)->__name__##_enc.d, (__vmcs__)->__name__##_enc.r); \
  }
#define VMPF(__vmcs__, __name__) \
  printk("  "#__name__" : 0x%016X\n", (__vmcs__)->__name__.raw); \
  printk("  - d(0x%x)\n  - r(0x%x)\n  - enc(0x%08x)\n", \
      (__vmcs__)->__name__##_enc.d, (__vmcs__)->__name__##_enc.r, \
      (__vmcs__)->__name__##_enc.raw);

union vmcs_field_encoding {
  struct {
    uint32_t atype:1; // 0: full, 1: high
    uint32_t index:9; // Index
    uint32_t type:2; // 0: ctrl, 1: VM-exit info, 2: guest state, 3: host state
    uint32_t r0:1;
    uint32_t width:2; // 0: 16-bit, 1: 64-bit, 2: 32-bit, 3: natural width
    uint32_t d:1; // written, needs to be committed
    uint32_t r:1; // already read
    uint32_t r1:15;
  };
  struct{
    uint32_t raw:15;
    uint32_t meta:17;
  };
};

struct vmcs_guest_state {
  // 16-bit fields
  VMCSF(struct field_16, es_selector);
  VMCSF(struct field_16, cs_selector);
  VMCSF(struct field_16, ss_selector);
  VMCSF(struct field_16, ds_selector);
  VMCSF(struct field_16, fs_selector);
  VMCSF(struct field_16, gs_selector);
  VMCSF(struct field_16, ldtr_selector);
  VMCSF(struct field_16, tr_selector);
  VMCSF(struct field_16, interrupt_status);
  // 64-bit fields
  VMCSF(struct field_64, vmcs_link_pointer);
  VMCSF(struct field_64, ia32_debugctl);
  VMCSF(struct field_64, ia32_pat);
  VMCSF(union msr_ia32_efer, ia32_efer);
  VMCSF(struct field_64, ia32_perf_global_ctrl);
  VMCSF(struct field_64, pdpte0);
  VMCSF(struct field_64, pdpte1);
  VMCSF(struct field_64, pdpte2);
  VMCSF(struct field_64, pdpte3);
  // 32-bit fields
  VMCSF(struct field_32, es_limit);
  VMCSF(struct field_32, cs_limit);
  VMCSF(struct field_32, ss_limit);
  VMCSF(struct field_32, ds_limit);
  VMCSF(struct field_32, fs_limit);
  VMCSF(struct field_32, gs_limit);
  VMCSF(struct field_32, ldtr_limit);
  VMCSF(struct field_32, tr_limit);
  VMCSF(struct field_32, gdtr_limit);
  VMCSF(struct field_32, idtr_limit);
  VMCSF(union access_rights, es_ar_bytes);
  VMCSF(union access_rights, cs_ar_bytes);
  VMCSF(union access_rights, ss_ar_bytes);
  VMCSF(union access_rights, ds_ar_bytes);
  VMCSF(union access_rights, fs_ar_bytes);
  VMCSF(union access_rights, gs_ar_bytes);
  VMCSF(union access_rights, ldtr_ar_bytes);
  VMCSF(union access_rights, tr_ar_bytes);
  VMCSF(struct field_32, interruptibility_info);
  VMCSF(struct field_32, activity_state);
  VMCSF(struct field_32, smbase);
  VMCSF(struct field_32, ia32_sysenter_cs);
  VMCSF(struct field_32, vmx_preemption_timer_value);
  // Natural-width fields
  VMCSF(union cr0, cr0);
  VMCSF(union cr3, cr3);
  VMCSF(union cr4, cr4);
  VMCSF(struct field_64, es_base);
  VMCSF(struct field_64, cs_base);
  VMCSF(struct field_64, ss_base);
  VMCSF(struct field_64, ds_base);
  VMCSF(struct field_64, fs_base);
  VMCSF(struct field_64, gs_base);
  VMCSF(struct field_64, ldtr_base);
  VMCSF(struct field_64, tr_base);
  VMCSF(struct field_64, gdtr_base);
  VMCSF(struct field_64, idtr_base);
  VMCSF(struct field_64, dr7);
  VMCSF(struct field_64, rsp);
  VMCSF(struct field_64, rip);
  VMCSF(union rflags, rflags);
  VMCSF(struct field_64, pending_dbg_exceptions);
  VMCSF(struct field_64, sysenter_esp);
  VMCSF(struct field_64, sysenter_eip);
};

struct vmcs_host_state {
  // 16-bit fields
  VMCSF(struct field_16, es_selector);
  VMCSF(struct field_16, cs_selector);
  VMCSF(struct field_16, ss_selector);
  VMCSF(struct field_16, ds_selector);
  VMCSF(struct field_16, fs_selector);
  VMCSF(struct field_16, gs_selector);
  VMCSF(struct field_16, tr_selector);
  // 64-bit fields
  VMCSF(struct field_64, ia32_pat);
  VMCSF(union msr_ia32_efer, ia32_efer);
  VMCSF(struct field_64, ia32_perf_global_ctrl);
  // 32-bit fields
  VMCSF(struct field_32, ia32_sysenter_cs);
  // Natural-width fields
  VMCSF(struct field_32, cr0);
  VMCSF(struct field_32, cr3);
  VMCSF(struct field_32, cr4);
  VMCSF(struct field_64, fs_base);
  VMCSF(struct field_64, gs_base);
  VMCSF(struct field_64, tr_base);
  VMCSF(struct field_64, gdtr_base);
  VMCSF(struct field_64, idtr_base);
  VMCSF(struct field_64, ia32_sysenter_esp);
  VMCSF(struct field_64, ia32_sysenter_eip);
  VMCSF(struct field_64, rsp);
  VMCSF(struct field_64, rip);
};

struct vmcs_vmexit_information {
  // 64-bit fields
  VMCSF(struct field_64, guest_physical_address);
  // 32-bit fields
  VMCSF(struct field_32, vm_instruction_error);
  VMCSF(union exit_reason, reason);
  VMCSF(struct field_32, intr_info);
  VMCSF(struct field_32, intr_error_code);
  VMCSF(struct field_32, idt_vectoring_info_field);
  VMCSF(struct field_32, idt_vectoring_error_code);
  VMCSF(struct field_32, instruction_len);
  VMCSF(struct field_32, vmx_instruction_info);
  // Natural-width fields
  VMCSF(struct field_64, qualification);
  VMCSF(struct field_64, io_rcx);
  VMCSF(struct field_64, io_rsi);
  VMCSF(struct field_64, io_rdi);
  VMCSF(struct field_64, io_rip);
  VMCSF(struct field_64, guest_linear_address);
};

struct vmcs_execution_controls {
  // 16-bit fields
  VMCSF(struct field_16, virtual_processor_id);
  VMCSF(struct field_16, posted_int_notif_vector);
  VMCSF(struct field_16, eptp_index);
  // 64-bit fields
  VMCSF(struct field_64, io_bitmap_a);
  VMCSF(struct field_64, io_bitmap_b);
  VMCSF(struct field_64, msr_bitmap);
  VMCSF(struct field_64, executive_vmcs_pointer);
  VMCSF(struct field_signed_64, tsc_offset);
  VMCSF(struct field_64, virtual_apic_page_addr);
  VMCSF(struct field_64, apic_access_addr);
  VMCSF(struct field_64, posted_intr_desc_addr);
  VMCSF(struct field_64, vm_function_controls);
  VMCSF(struct field_64, ept_pointer);
  VMCSF(struct field_64, eoi_exit_bitmap_0);
  VMCSF(struct field_64, eoi_exit_bitmap_1);
  VMCSF(struct field_64, eoi_exit_bitmap_2);
  VMCSF(struct field_64, eoi_exit_bitmap_3);
  VMCSF(struct field_64, eptp_list_addr);
  VMCSF(struct field_64, vmread_bitmap_addr);
  VMCSF(struct field_64, vmwrite_bitmap_addr);
  VMCSF(struct field_64, virt_excep_info_addr);
  VMCSF(struct field_64, xss_exiting_bitmap);
  // 32-bit fields
  VMCSF(union pin_based, pin_based_vm_exec_control);
  VMCSF(union proc_based, cpu_based_vm_exec_control);
  VMCSF(struct field_32, exception_bitmap);
  VMCSF(struct field_32, page_fault_error_code_mask);
  VMCSF(struct field_32, page_fault_error_code_match);
  VMCSF(struct field_32, cr3_target_count);
  VMCSF(struct field_32, tpr_threshold);
  VMCSF(union proc_based_2, secondary_vm_exec_control);
  VMCSF(struct field_32, ple_gap);
  VMCSF(struct field_32, ple_window);
  // Natural-width fields
  VMCSF(struct field_32, cr0_guest_host_mask);
  VMCSF(struct field_32, cr4_guest_host_mask);
  VMCSF(struct field_32, cr0_read_shadow);
  VMCSF(struct field_32, cr4_read_shadow);
  VMCSF(struct field_32, cr3_target_value0);
  VMCSF(struct field_32, cr3_target_value1);
  VMCSF(struct field_32, cr3_target_value2);
  VMCSF(struct field_32, cr3_target_value3);
};

struct vmcs_exit_controls {
  // 64-bit fields
  VMCSF(struct field_64, msr_store_addr);
  VMCSF(struct field_64, msr_load_addr);
  // 32-bit fields
  VMCSF(union exit_controls, controls);
  VMCSF(struct field_32, msr_store_count);
  VMCSF(struct field_32, msr_load_count);
};

struct vmcs_entry_controls {
  // 64-bit fields
  VMCSF(struct field_64, msr_load_addr);
  // 32-bit fields
  VMCSF(union entry_controls, controls);
  VMCSF(struct field_32, msr_load_count);
  VMCSF(struct field_32, intr_info_field);
  VMCSF(struct field_32, exception_error_code);
  VMCSF(struct field_32, instruction_len);
};

struct vmcs_controls {
  struct vmcs_execution_controls ex;
  struct vmcs_exit_controls exit;
  struct vmcs_entry_controls entry;
};

struct vmcs {
  uint32_t revision_id;
  uint32_t abort;
  struct vmcs_guest_state gs;
  struct vmcs_host_state hs;
  struct vmcs_controls ctrls;
  struct vmcs_vmexit_information info;
};

void vmcs_dump_vcpu(void);
void vmcs_set_vmx_preemption_timer_value(struct vmcs *v, uint64_t time_microsec);
void vmcs_init(void);
void vmcs_alloc(uint32_t *index, uint8_t **region, struct vmcs **cache);
void vmcs_free(uint32_t index);
void vmcs_clone(struct vmcs *v);
void vmcs_commit(void);
void vmcs_dump_ctrls(struct vmcs *v);
void vmcs_dump_gs(struct vmcs *v);
void vmcs_dump_hs(struct vmcs *v);
void vmcs_dump_info(struct vmcs *v);
void vmcs_dump(struct vmcs *v);
void vmcs_update(void);
void vmcs_force_update(void);
void vmcs_collect_shadow(struct vmcs *gvmcs);
void vmcs_create_vmcs_regions(void);
void vmcs_encoding_init(struct vmcs *v);
void vmcs_encoding_init_all(void);

extern uint8_t *vmxon;
extern struct vmcs *vmcs_cache_pool;
extern uint8_t (*vmcs_region_pool)[0x1000];
extern struct vmcs *vmcs;
extern struct vmcs *hc;

#endif
