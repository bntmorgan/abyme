#ifndef __EFIW_H__
#define __EFIW_H__

#include <efi.h>
#include <efilib.h>

void *efi_allocate_pool(uint64_t size);

extern EFI_STATUS efiw_status;

#endif//__EFIW_H__
