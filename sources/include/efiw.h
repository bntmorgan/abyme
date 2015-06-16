#ifndef __EFIW_H__
#define __EFIW_H__

#include <efi.h>
#include <efilib.h>

struct efi_loaded_image {
  uint64_t start;
  uint64_t end;
};

int efi_loaded_image_info(EFI_HANDLE image_handle, struct efi_loaded_image
    *eli);
void *efi_allocate_pool(uint64_t size);
void *efi_allocate_pages(uint64_t count);

extern EFI_STATUS efiw_status;

#endif//__EFIW_H__
