#include "efiw.h"

EFI_STATUS efiw_status;

int efi_loaded_image_info(EFI_HANDLE image_handle, struct efi_loaded_image
    *eli) {
  EFI_LOADED_IMAGE *img;
  EFI_GUID guid = LOADED_IMAGE_PROTOCOL;

  efiw_status = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle, &guid,
      &img); 

  if (!EFI_ERROR (efiw_status)) {
    eli->start = (uint64_t)img->ImageBase;
    eli->end = eli->start + img->ImageSize;
  }
  return efiw_status;
}



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

void *efi_allocate_pages(uint64_t count) {
  EFI_PHYSICAL_ADDRESS addr;
  // allocate the page tables 
  efiw_status = uefi_call_wrapper(ST->BootServices->AllocatePages, 4,
      AllocateAnyPages, EfiRuntimeServicesData, count, &addr); 
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
  return (void *)addr;
}
