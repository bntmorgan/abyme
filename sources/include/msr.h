#ifndef __MSR_H__
#define __MSR_H__

// TODO: pourquoi les deux a la fois?
#include <efi.h>
#include "types.h"

union msr_ia32_efer {
  struct {
    uint64_t sce:1;
    uint64_t r0:7;
    uint64_t lme:1;
    uint64_t r1:1;
    uint64_t lma:1;
    uint64_t nxe:1;
    uint64_t r2:52;
  };
  uint64_t raw;
};

#define MSR_ADDRESS_IA32_EFER                       0xc0000080
#define MSR_ADDRESS_IA32_TIME_STAMP_COUNTER         0x010
#define MSR_ADDRESS_IA32_APIC_BASE                  0x01b
#define MSR_ADDRESS_IA32_FEATURE_CONTROL_MSR        0x03a
#define MSR_ADDRESS_IA32_SMM_MONITOR_CTL            0x09b
#define MSR_ADDRESS_MSR_PLATFORM_INFO               0x0ce
#define MSR_ADDRESS_IA32_MTRR_CAP                   0x0fe
#define MSR_ADDRESS_IA32_DEBUGCTL                   0x1d9
#define MSR_ADDRESS_IA32_SYSENTER_CS                0x174
#define MSR_ADDRESS_IA32_SYSENTER_EIP               0x176
#define MSR_ADDRESS_IA32_SYSENTER_ESP               0x175
#define MSR_ADDRESS_IA32_MTRR_FIX64K_00000          0x250
#define MSR_ADDRESS_IA32_MTRR_FIX16K_80000          0x258
#define MSR_ADDRESS_IA32_MTRR_FIX16K_A0000          0x259
#define MSR_ADDRESS_IA32_MTRR_FIX4K_C0000           0x268
#define MSR_ADDRESS_IA32_MTRR_FIX4K_C8000           0x269
#define MSR_ADDRESS_IA32_MTRR_FIX4K_D0000           0x26a
#define MSR_ADDRESS_IA32_MTRR_FIX4K_D8000           0x26b
#define MSR_ADDRESS_IA32_MTRR_FIX4K_E0000           0x26c
#define MSR_ADDRESS_IA32_MTRR_FIX4K_E8000           0x26d
#define MSR_ADDRESS_IA32_MTRR_FIX4K_F0000           0x26e
#define MSR_ADDRESS_IA32_MTRR_FIX4K_F8000           0x26f
#define MSR_ADDRESS_IA32_PAT                        0x277
#define MSR_ADDRESS_IA32_PERF_GLOBAL_CTRL           0x38f
#define MSR_ADDRESS_IA32_VMX_BASIC                  0x480
#define MSR_ADDRESS_IA32_VMX_PINBASED_CTLS          0x481
#define MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS         0x482
#define MSR_ADDRESS_IA32_VMX_EXIT_CTLS              0x483
#define MSR_ADDRESS_IA32_VMX_ENTRY_CTLS             0x484
#define MSR_ADDRESS_IA32_VMX_MISC                   0x485
#define MSR_ADDRESS_VMX_CR0_FIXED0                  0x486
#define MSR_ADDRESS_VMX_CR0_FIXED1                  0x487
#define MSR_ADDRESS_VMX_CR4_FIXED0                  0x488
#define MSR_ADDRESS_VMX_CR4_FIXED1                  0x489
#define MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS2        0x48B
#define MSR_ADDRESS_IA32_VMX_TRUE_PINBASED_CTLS     0x48D
#define MSR_ADDRESS_IA32_VMX_TRUE_PROCBASED_CTLS    0x48E
#define MSR_ADDRESS_IA32_VMX_TRUE_EXIT_CTLS         0x48F
#define MSR_ADDRESS_IA32_VMX_TRUE_ENTRY_CTLS        0x490
#define MSR_ADDRESS_IA32_TSC_DEADLINE               0x6E0

#define MSR_ADDRESS_IA32_MTRRCAP                    0x0fe
#define MSR_ADDRESS_IA32_MTRR_DEF_TYPE              0x2ff
#define MSR_ADDRESS_IA32_MTRR_FIX64K_00000          0x250
#define MSR_ADDRESS_IA32_MTRR_FIX16K_80000          0x258
#define MSR_ADDRESS_IA32_MTRR_FIX16K_A0000          0x259
#define MSR_ADDRESS_IA32_MTRR_FIX4K_C0000           0x268
#define MSR_ADDRESS_IA32_MTRR_FIX4K_C8000           0x269
#define MSR_ADDRESS_IA32_MTRR_FIX4K_D0000           0x26a
#define MSR_ADDRESS_IA32_MTRR_FIX4K_D8000           0x26b
#define MSR_ADDRESS_IA32_MTRR_FIX4K_E0000           0x26c
#define MSR_ADDRESS_IA32_MTRR_FIX4K_E8000           0x26d
#define MSR_ADDRESS_IA32_MTRR_FIX4K_F0000           0x26e
#define MSR_ADDRESS_IA32_MTRR_FIX4K_F8000           0x26f
#define MSR_ADDRESS_IA32_MTRR_PHYSBASE0             0x200

#define MSR_ADDRESS_IA32_X2APIC_APICID              0x802
#define MSR_ADDRESS_IA32_X2APIC_VESION              0x803
#define MSR_ADDRESS_IA32_X2APIC_LVT_CMCI            0x82f
#define MSR_ADDRESS_IA32_X2APIC_LVT_TIMER           0x832
#define MSR_ADDRESS_IA32_X2APIC_LVT_THERMAL         0x833
#define MSR_ADDRESS_IA32_X2APIC_LVT_PMI             0x834
#define MSR_ADDRESS_IA32_X2APIC_LVT_LINT0           0x835
#define MSR_ADDRESS_IA32_X2APIC_LVT_LINT1           0x836
#define MSR_ADDRESS_IA32_X2APIC_LVT_ERROR           0x837
#define MSR_ADDRESS_IA32_X2APIC_LVT_INITIAL_COUNT   0x838
#define MSR_ADDRESS_IA32_X2APIC_LVT_CURRENT_COUNT   0x839

uint64_t msr_read(uint64_t msr_address);

void msr_write(uint64_t msr_address, uint64_t msr_value);

void msr_check_feature_control_msr_lock(void);

#endif
