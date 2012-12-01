#ifndef __CPUID_INT_H__
#define __CPUID_INT_H__

#include <stdint.h>

void cpuid_setup(void);

uint8_t cpuid_is_long_mode_supported(void);

uint8_t cpuid_is_pae_supported(void);

uint8_t cpuid_is_vmx_supported(void);

uint8_t cpuid_has_local_apic(void);

#endif
