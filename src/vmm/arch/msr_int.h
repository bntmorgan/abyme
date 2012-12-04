#ifndef __MSR_INT_H__
#define __MSR_INT_H__

#include <stdint.h>

#define MSR_ADDRESS_IA32_APIC_BASE                  0x01b
#define MSR_ADDRESS_IA32_DEBUGCTL                   0x1d9
#define MSR_ADDRESS_IA32_FEATURE_CONTROL_MSR        0x03a
#define MSR_ADDRESS_IA32_SYSENTER_CS                0x174
#define MSR_ADDRESS_IA32_SYSENTER_EIP               0x176
#define MSR_ADDRESS_IA32_SYSENTER_ESP               0x175
#define MSR_ADDRESS_IA32_VMX_BASIC                  0x480
#define MSR_ADDRESS_IA32_VMX_PINBASED_CTLS          0x481
#define MSR_ADDRESS_IA32_VMX_PROCBASED_CTLS         0x482
#define MSR_ADDRESS_VMX_CR0_FIXED0                  0x486
#define MSR_ADDRESS_VMX_CR0_FIXED1                  0x487
#define MSR_ADDRESS_VMX_CR4_FIXED0                  0x488
#define MSR_ADDRESS_VMX_CR4_FIXED1                  0x489
#define MSR_ADDRESS_IA32_VMX_TRUE_PINBASED_CTLS     0x48D
#define MSR_ADDRESS_IA32_VMX_TRUE_PROCBASED_CTLS    0x48E

void msr_read(uint32_t address, uint32_t *eax, uint32_t *edx);

void msr_write(uint32_t address, uint32_t eax, uint32_t edx);

void msr_check_feature_control_msr_lock(void);

#endif
