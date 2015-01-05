#ifndef __VMX_H__
#define __VMX_H__

#include <efi.h>
#include "types.h"

void cpu_enable_vmxe(void);

void cpu_vmxon(uint8_t *region);

void cpu_vmclear(uint8_t *region);

void cpu_vmptrld(uint8_t *region);

void cpu_vmlaunch(void);

void cpu_vmwrite(uint64_t field, uint64_t value);

uint64_t cpu_vmread(uint64_t field);

uint8_t cpu_vmread_safe(unsigned long field, unsigned long *value);

uint32_t cpu_adjust32(uint32_t value, uint32_t msr);

uint64_t cpu_adjust64(uint64_t value, uint32_t msr_fixed0, uint32_t msr_fixed1);

/* We fix the calling conventions to sysv_abi */
void vmx_transition_display_error(uint8_t VMfailInvalid, uint8_t VMfailValid) __attribute__((sysv_abi));

#endif//__VMX_H__
