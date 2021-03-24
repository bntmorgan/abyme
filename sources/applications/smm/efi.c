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

