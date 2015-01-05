#include <efi.h>
#include <efilib.h>

#define EFI_SYSTEM_TABLE_POINTER 0x80000000

EFI_STATUS
efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
  InitializeLib(image_handle, systab);
  EFI_SYSTEM_TABLE **global_systab = (EFI_SYSTEM_TABLE **)0x80000000;
  Print(L"BEFORE %08x\n", *global_systab);
  *global_systab = systab;
  Print(L"AFTER %08x\n", *global_systab);
  return EFI_SUCCESS;
}

