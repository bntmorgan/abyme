#include <efi.h>
#include <efilib.h>
#include "stdio.h"
#include "smm.h"

EFI_STATUS
efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
  InitializeLib(image_handle, systab);
  smm_unlock_smram();
  return EFI_SUCCESS;
}

