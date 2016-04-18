#include <efi.h>
#include <efilib.h>
#include "stdio.h"

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image, systab);

  INFO("Hello World\n");

  return EFI_SUCCESS;
}
