#ifndef __VMM_VMCS_H__
#define __VMM_VMCS_H__

#define VMCS_DEFAULT_PREEMPTION_TIMER_MICROSEC 5*1000000
#define NB_VMCS_FIELDS 185

extern uint8_t vmxon[4096];
extern uint8_t vmcs0[4096];

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

void vmcs_fill_guest_state_fields(void);

void vmcs_fill_host_state_fields(void);

void vmcs_fill_vm_exec_control_fields(void);

void vmcs_fill_vm_exit_control_fields(void);

void vmcs_fill_vm_entry_control_fields(void);

void vmcs_dump_vcpu(void);

void vmcs_set_vmx_preemption_timer_value(uint64_t time_microsec);

#endif
