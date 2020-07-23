#ifndef __EFIW_H__
#define __EFIW_H__

#include <efi.h>
#include <efilib.h>

struct efi_loaded_image {
  uint64_t start;
  uint64_t end;
  CHAR16 *options;
  uint64_t options_size;
};

int efi_loaded_image_info(EFI_HANDLE image_handle, struct efi_loaded_image
    *eli);
void *efi_allocate_pool(uint64_t size);
void efi_free_pool(void *pool);
void *efi_allocate_pages(uint64_t count);
void *efi_allocate_pages_at(void *addr, uint64_t count);
void *efi_allocate_low_pages(uint64_t count);
int efi_execute_image(EFI_HANDLE parent_image, uint8_t *buf, uint32_t size);
void efi_reset_system(void);

extern EFI_STATUS efiw_status;

#endif//__EFIW_H__
