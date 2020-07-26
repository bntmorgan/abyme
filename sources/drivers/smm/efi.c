#include <efi.h>
#include <efilib.h>
#include "stdio.h"
#include "shell.h"
#include "smm.h"

EFI_STATUS
efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
  InitializeLib(image_handle, systab);

  // Print to shell
  putc = &shell_print;

  smm_unlock_smram();
  return EFI_SUCCESS;
}

