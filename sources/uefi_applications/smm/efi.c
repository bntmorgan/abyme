#include <efi.h>
#include <efilib.h>
#include "efi/efiapi_1_1.h"

EFI_STATUS
efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
  InitializeLib(image_handle, systab);
  EFI_STATUS status;
  EFI_GUID smm_guid = EFI_SMM_ACCESS_PROTOCOL_GUID;
  EFI_SMM_ACCESS_PROTOCOL *smm_access;
  status = uefi_call_wrapper(systab->BootServices->LocateProtocol,
                             3,
                             &smm_guid,
                             NULL,
                             (void **)&smm_access);
  Print(L"SMM %d\n", status);
  EFI_GUID smm_base_guid = EFI_SMM_BASE_PROTOCOL_GUID;
  EFI_SMM_BASE_PROTOCOL *smm_base;
  status = uefi_call_wrapper(systab->BootServices->LocateProtocol,
                             3,
                             &smm_base_guid,
                             NULL,
                             (void **)&smm_base);
  Print(L"SMM %d\n", status);
  return EFI_SUCCESS;
}

