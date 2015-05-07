#ifndef __CPUID_H__
#define __CPUID_H__

#include <efi.h>
#include "types.h"

void cpuid_setup(void);

uint8_t cpuid_is_long_mode_supported(void);

uint8_t cpuid_is_pae_supported(void);

uint8_t cpuid_is_page1g_supported(void);

uint8_t cpuid_is_vmx_supported(void);

uint8_t cpuid_has_local_apic(void);

uint8_t cpuid_get_maxphyaddr(void);

uint8_t cpuid_are_mtrr_supported(void);

uint8_t cpuid_is_x2APIC_supported(void);

#endif
