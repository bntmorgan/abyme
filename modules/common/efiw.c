#include "efiw.h"

EFI_STATUS efiw_status;

void *efi_allocate_pool(uint64_t size) {
  void *buf;
  // allocate the page tables 
  efiw_status = uefi_call_wrapper(ST->BootServices->AllocatePool, 3,
      EfiRuntimeServicesData, size, &buf); 
  switch (efiw_status) {
    case EFI_SUCCESS:
      break;
    case EFI_OUT_OF_RESOURCES:
      return NULL;
    case EFI_INVALID_PARAMETER:
      return NULL;
    case EFI_NOT_FOUND:
      return NULL;
    default:
      // Unrecognized error code
      return NULL;
  }
  return buf;
}
