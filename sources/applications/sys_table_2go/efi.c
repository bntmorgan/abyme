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

#define EFI_SYSTEM_TABLE_POINTER 0x80000000

EFI_STATUS
efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
  InitializeLib(image_handle, systab);
  EFI_SYSTEM_TABLE **global_systab = (EFI_SYSTEM_TABLE **)0x80000000;
  Print(L"BEFORE %08x\n", *global_systab);
  *global_systab = systab;
  Print(L"AFTER %08x\n", *global_systab);
  return EFI_SUCCESS;
}

