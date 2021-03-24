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

#include "shell.h"

#include <efi.h>
#include <efilib.h>

void shell_print(uint8_t value) {
  uint32_t v = value & 0x000000ff;
  uefi_call_wrapper(ST->ConOut->OutputString, 2, ST->ConOut, (uint16_t*)&v);
  if (value == '\n') {
    v = '\r' & 0x000000ff;
    uefi_call_wrapper(ST->ConOut->OutputString, 2, ST->ConOut, (CHAR16*)&v);
  }
}
