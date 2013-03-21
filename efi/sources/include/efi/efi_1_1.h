#ifndef __efi_1_1_h__
#define __efi_1_1_h__

#include "efi/efiapi_1_1.h"

#define GET_EFI_BOOT_SERVICES_1_1(p) ((EFI_BOOT_SERVICES_1_1*)(((EFI_BOOT_SERVICES*)p) + 1))

#endif
