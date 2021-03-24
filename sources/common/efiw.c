/*
Copyright (C) 2021  Benoît Morgan

This file is part of abyme

abyme is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

abyme is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with abyme.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "efiw.h"

EFI_STATUS efiw_status;

int efi_loaded_image_info(EFI_HANDLE image_handle, struct efi_loaded_image
    *eli) {
  EFI_LOADED_IMAGE *img;
  EFI_GUID guid = LOADED_IMAGE_PROTOCOL;

  efiw_status = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle, &guid,
      &img);

  if (!EFI_ERROR(efiw_status)) {
    eli->start = (uint64_t)img->ImageBase;
    eli->end = eli->start + img->ImageSize;
    eli->options = img->LoadOptions;
    eli->options_size = img->LoadOptionsSize;
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

int efi_load_file(EFI_HANDLE parent_image, uint8_t *buf, uint32_t size) {
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

void efi_free_pool(void *pool) {
  efiw_status = uefi_call_wrapper(ST->BootServices->FreePool, 1, pool);
}

void efi_reset_system(void) {
  efiw_status = uefi_call_wrapper(ST->RuntimeServices->ResetSystem, 4,
      EfiResetCold, EFI_SUCCESS, 0, NULL);
}

void *efi_allocate_pages_at(void *addr, uint64_t count, uint8_t type) {
  // allocate the page tables
  efiw_status = uefi_call_wrapper(ST->BootServices->AllocatePages, 4,
      AllocateAddress, type, count, &addr);
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

void *efi_allocate_low_pages(uint64_t count) {
  EFI_PHYSICAL_ADDRESS addr = 0x10000;
  // allocate the page tables
  efiw_status = uefi_call_wrapper(ST->BootServices->AllocatePages, 4,
      AllocateMaxAddress, EfiRuntimeServicesData, count, &addr);
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
