/*
 * Copyright (C) 2011 Canonical
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#include <efi.h>
#include <efilib.h>

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
//  unsigned int i;
//  unsigned int j;
  InitializeLib(image, systab);
  uefi_call_wrapper(systab->ConOut->OutputString, 2, systab->ConOut, L"Hello World\n\r");
//  for (i = 0; i < 0xffffffff; i++) {
//    for (j = 0; j < 0xffffffff; j++);
//    uefi_call_wrapper(systab->ConOut->OutputString, 2, systab->ConOut, L".");
//  }
  return EFI_SUCCESS;
}
