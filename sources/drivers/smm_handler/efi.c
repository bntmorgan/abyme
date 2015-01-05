#include <efi.h>
#include <efilib.h>

EFI_STATUS
InitializeChild (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
  InitializeLib(image_handle, systab);

  return EFI_SUCCESS;
}

EFI_DRIVER_ENTRY_POINT(InitializeChild);
