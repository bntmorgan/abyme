#include <efi.h>
#include <efilib.h>

#include "smp.h"

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image, systab);

  smp_setup();
  smp_activate_cores();

  return EFI_SUCCESS;
}
