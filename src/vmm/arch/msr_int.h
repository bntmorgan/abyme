#ifndef __MSR_INT_H__
#define __MSR_INT_H__

#include <stdint.h>

#define MSR_ADDRESS_IA32_VMX_BASIC		0x480
#define MSR_ADDRESS_IA32_FEATURE_CONTROL_MSR	0x03a
#define MSR_ADDRESS_IA32_FEATURE_CONTROL_MSR	0x03a
#define MSR_ADDRESS_VMX_CR0_FIXED0		0x486
#define MSR_ADDRESS_VMX_CR0_FIXED1		0x487
#define MSR_ADDRESS_VMX_CR4_FIXED0		0x488
#define MSR_ADDRESS_VMX_CR4_FIXED1		0x489

void msr_read(uint32_t address, uint32_t *eax, uint32_t *edx);

void msr_check_feature_control_msr_lock(void);

#endif
