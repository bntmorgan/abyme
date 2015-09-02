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

int efi_execute_image(EFI_HANDLE parent_image, uint8_t *buf, uint32_t size) {
  EFI_HANDLE image_handle;
  EFI_STATUS status;
  status = uefi_call_wrapper(ST->BootServices->LoadImage, 6, 0, parent_image, 0,
      buf, size, &image_handle);
  if (status) {
    return status;
  }
  status = uefi_call_wrapper(ST->BootServices->StartImage, 3, image_handle, 0,
      0);
  return status;
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
