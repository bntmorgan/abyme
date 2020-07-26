#include <efi.h>
#include <efilib.h>

#include "stdio.h"
#include "shell.h"
#include "netboot.h"

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *st) {

  // Initialize gnuefi lib
  InitializeLib(image_handle, st);

  // Print to shell
  putc = &shell_print;

  INFO("YOLO\n");
  netboot(image_handle);

  return EFI_SUCCESS;
}
