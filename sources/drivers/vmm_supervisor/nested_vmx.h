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

#ifndef __NESTED_VMX_H__
#define __NESTED_VMX_H__

#define NESTED_GUEST_FIELDS \
          GUEST_ES_SELECTOR, \
          GUEST_CS_SELECTOR, \
          GUEST_SS_SELECTOR, \
          GUEST_DS_SELECTOR, \
          GUEST_FS_SELECTOR, \
          GUEST_GS_SELECTOR, \
          GUEST_LDTR_SELECTOR, \
          GUEST_TR_SELECTOR, \
          GUEST_INTERRUPT_STATUS, \
          VMCS_LINK_POINTER, \
          VMCS_LINK_POINTER_HIGH, \
          GUEST_IA32_DEBUGCTL, \
          GUEST_IA32_DEBUGCTL_HIGH, \
          GUEST_IA32_PAT, \
          GUEST_IA32_PAT_HIGH, \
          GUEST_IA32_EFER, \
          GUEST_IA32_EFER_HIGH, \
          GUEST_IA32_PERF_GLOBAL_CTRL, \
          GUEST_IA32_PERF_GLOBAL_CTRL_HIGH, \
          GUEST_PDPTR0, \
          GUEST_PDPTR0_HIGH, \
          GUEST_PDPTR1, \
          GUEST_PDPTR1_HIGH, \
          GUEST_PDPTR2, \
          GUEST_PDPTR2_HIGH, \
          GUEST_PDPTR3, \
          GUEST_PDPTR3_HIGH, \
          GUEST_ES_LIMIT, \
          GUEST_CS_LIMIT, \
          GUEST_SS_LIMIT, \
          GUEST_DS_LIMIT, \
          GUEST_FS_LIMIT, \
          GUEST_GS_LIMIT, \
          GUEST_LDTR_LIMIT, \
          GUEST_TR_LIMIT, \
          GUEST_GDTR_LIMIT, \
          GUEST_IDTR_LIMIT, \
          GUEST_ES_AR_BYTES, \
          GUEST_CS_AR_BYTES, \
          GUEST_SS_AR_BYTES, \
          GUEST_DS_AR_BYTES, \
          GUEST_FS_AR_BYTES, \
          GUEST_GS_AR_BYTES, \
          GUEST_LDTR_AR_BYTES, \
          GUEST_TR_AR_BYTES, \
          GUEST_INTERRUPTIBILITY_INFO, \
          GUEST_ACTIVITY_STATE, \
          GUEST_SMBASE, \
          GUEST_SYSENTER_CS, \
          /*VMX_PREEMPTION_TIMER_VALUE,*/ \
          GUEST_CR0, \
          GUEST_CR3, \
          GUEST_CR4, \
          GUEST_ES_BASE, \
          GUEST_CS_BASE, \
          GUEST_SS_BASE, \
          GUEST_DS_BASE, \
          GUEST_FS_BASE, \
          GUEST_GS_BASE, \
          GUEST_LDTR_BASE, \
          GUEST_TR_BASE, \
          GUEST_GDTR_BASE, \
          GUEST_IDTR_BASE, \
          GUEST_DR7, \
          GUEST_RSP, \
          GUEST_RIP, \
          GUEST_RFLAGS, \
          GUEST_PENDING_DBG_EXCEPTIONS, \
          GUEST_SYSENTER_ESP, \
          GUEST_SYSENTER_EIP

#define NESTED_HOST_FIELDS \
          HOST_ES_SELECTOR, \
          HOST_CS_SELECTOR, \
          HOST_SS_SELECTOR, \
          HOST_DS_SELECTOR, \
          HOST_FS_SELECTOR, \
          HOST_GS_SELECTOR, \
          HOST_TR_SELECTOR, \
          HOST_IA32_PAT, \
          HOST_IA32_PAT_HIGH, \
          HOST_IA32_EFER, \
          HOST_IA32_EFER_HIGH, \
          HOST_IA32_PERF_GLOBAL_CTRL, \
          HOST_IA32_PERF_GLOBAL_CTRL_HIGH, \
          HOST_IA32_SYSENTER_CS, \
          HOST_CR0, \
          HOST_CR3, \
          HOST_CR4, \
          HOST_FS_BASE, \
          HOST_GS_BASE, \
          HOST_TR_BASE, \
          HOST_GDTR_BASE, \
          HOST_IDTR_BASE, \
          HOST_IA32_SYSENTER_ESP, \
          HOST_IA32_SYSENTER_EIP, \
          HOST_RSP, \
          HOST_RIP

#define NESTED_HOST_FIELDS_DEST \
          GUEST_ES_SELECTOR, \
          GUEST_CS_SELECTOR, \
          GUEST_SS_SELECTOR, \
          GUEST_DS_SELECTOR, \
          GUEST_FS_SELECTOR, \
          GUEST_GS_SELECTOR, \
          GUEST_TR_SELECTOR, \
          GUEST_IA32_PAT, \
          GUEST_IA32_PAT_HIGH, \
          GUEST_IA32_EFER, \
          GUEST_IA32_EFER_HIGH, \
          GUEST_IA32_PERF_GLOBAL_CTRL, \
          GUEST_IA32_PERF_GLOBAL_CTRL_HIGH, \
          GUEST_SYSENTER_CS, \
          GUEST_CR0, \
          GUEST_CR3, \
          GUEST_CR4, \
          GUEST_FS_BASE, \
          GUEST_GS_BASE, \
          GUEST_TR_BASE, \
          GUEST_GDTR_BASE, \
          GUEST_IDTR_BASE, \
          GUEST_SYSENTER_ESP, \
          GUEST_SYSENTER_EIP, \
          GUEST_RSP, \
          GUEST_RIP

#define NESTED_READ_ONLY_DATA_FIELDS \
          GUEST_PHYSICAL_ADDRESS, \
          GUEST_PHYSICAL_ADDRESS_HIGH, \
          VM_INSTRUCTION_ERROR, \
          VM_EXIT_REASON, \
          VM_EXIT_INTR_INFO, \
          VM_EXIT_INTR_ERROR_CODE, \
          IDT_VECTORING_INFO_FIELD, \
          IDT_VECTORING_ERROR_CODE, \
          VM_EXIT_INSTRUCTION_LEN, \
          VMX_INSTRUCTION_INFO, \
          EXIT_QUALIFICATION, \
          IO_RCX, \
          IO_RSI, \
          IO_RDI, \
          IO_RIP, \
          GUEST_LINEAR_ADDRESS

#define NESTED_CTRL_FIELDS \
          VIRTUAL_PROCESSOR_ID, \
          POSTED_INT_NOTIF_VECTOR, \
          EPTP_INDEX, \
          IO_BITMAP_A, \
          IO_BITMAP_A_HIGH, \
          IO_BITMAP_B, \
          IO_BITMAP_B_HIGH, \
          MSR_BITMAP, \
          MSR_BITMAP_HIGH, \
          VM_EXIT_MSR_STORE_ADDR, \
          VM_EXIT_MSR_STORE_ADDR_HIGH, \
          VM_EXIT_MSR_LOAD_ADDR, \
          VM_EXIT_MSR_LOAD_ADDR_HIGH, \
          VM_ENTRY_MSR_LOAD_ADDR, \
          VM_ENTRY_MSR_LOAD_ADDR_HIGH, \
          EXECUTIVE_VMCS_POINTER, \
          EXECUTIVE_VMCS_POINTER_HIGH, \
          TSC_OFFSET, \
          TSC_OFFSET_HIGH, \
          VIRTUAL_APIC_PAGE_ADDR, \
          VIRTUAL_APIC_PAGE_ADDR_HIGH, \
          APIC_ACCESS_ADDR, \
          APIC_ACCESS_ADDR_HIGH, \
          POSTED_INTR_DESC_ADDR, \
          POSTED_INTR_DESC_ADDR_HIGH, \
          VM_FUNCTION_CONTROLS, \
          VM_FUNCTION_CONTROLS_HIGH, \
          EPT_POINTER, \
          EPT_POINTER_HIGH, \
          EOI_EXIT_BITMAP_0, \
          EOI_EXIT_BITMAP_0_HIGH, \
          EOI_EXIT_BITMAP_1, \
          EOI_EXIT_BITMAP_1_HIGH, \
          EOI_EXIT_BITMAP_2, \
          EOI_EXIT_BITMAP_2_HIGH, \
          EOI_EXIT_BITMAP_3, \
          EOI_EXIT_BITMAP_3_HIGH, \
          EPTP_LIST_ADDR, \
          EPTP_LIST_ADDR_HIGH, \
          VMREAD_BITMAP_ADDR, \
          VMREAD_BITMAP_ADDR_HIGH, \
          VMWRITE_BITMAP_ADDR, \
          VMWRITE_BITMAP_ADDR_HIGH, \
          VIRT_EXCEP_INFO_ADDR, \
          VIRT_EXCEP_INFO_ADDR_HIGH, \
          XSS_EXITING_BITMAP, \
          XSS_EXITING_BITMAP_HIGH, \
          PIN_BASED_VM_EXEC_CONTROL, \
          CPU_BASED_VM_EXEC_CONTROL, \
          EXCEPTION_BITMAP, \
          PAGE_FAULT_ERROR_CODE_MASK, \
          PAGE_FAULT_ERROR_CODE_MATCH, \
          CR3_TARGET_COUNT, \
          VM_EXIT_CONTROLS, \
          VM_EXIT_MSR_STORE_COUNT, \
          VM_EXIT_MSR_LOAD_COUNT, \
          VM_ENTRY_CONTROLS, \
          VM_ENTRY_MSR_LOAD_COUNT, \
          VM_ENTRY_INTR_INFO_FIELD, \
          VM_ENTRY_EXCEPTION_ERROR_CODE, \
          VM_ENTRY_INSTRUCTION_LEN, \
          TPR_THRESHOLD, \
          SECONDARY_VM_EXEC_CONTROL, \
          PLE_GAP, \
          PLE_WINDOW, \
          CR0_GUEST_HOST_MASK, \
          CR4_GUEST_HOST_MASK, \
          CR0_READ_SHADOW, \
          CR4_READ_SHADOW, \
          CR3_TARGET_VALUE0, \
          CR3_TARGET_VALUE1, \
          CR3_TARGET_VALUE2, \
          CR3_TARGET_VALUE3


extern uint8_t guest_vmcs[4096];

enum NESTED_STATE {
  NESTED_DISABLED,
  NESTED_HOST_RUNNING,
  NESTED_GUEST_RUNNING
};

extern uint8_t nested_state;

void nested_vmxon(uint8_t *vmxon_guest);

void nested_vmclear(uint8_t *shadow_vmcs);

void nested_vmptrld(uint8_t *shadow_vmcs);

void nested_vmlaunch(void);

uint64_t nested_vmread(uint64_t field);

void nested_vmwrite(uint64_t field, uint64_t value);

void nested_load_guest(void);

void nested_load_host(void);

#endif
