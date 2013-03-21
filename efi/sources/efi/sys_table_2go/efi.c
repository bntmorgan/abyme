#include <efi.h>
#include <efilib.h>

#define EFI_SYSTEM_TABLE_POINTER 0x80000000

EFI_STATUS
efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
  EFI_SYSTEM_TABLE **global_systab = (EFI_SYSTEM_TABLE **)0x80000000;
  *global_systab = systab;
  Print(L"EFI_SYSTEM_TABLE IS INSTALLED AT %08x\n", EFI_SYSTEM_TABLE_POINTER);
  return EFI_SUCCESS;
}

