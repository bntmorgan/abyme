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

#include "efi/efi_82579LM.h"
#include "debug.h"

#include "stdio.h"
#include "shell.h"

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image, systab);
  EFI_STATUS status;
  protocol_82579LM *eth;
  EFI_GUID guid_82579LM = EFI_PROTOCOL_82579LM_GUID;

  // Print to shell
  putc = &shell_print;

  status = LibLocateProtocol(&guid_82579LM, (void **)&eth);
  if (EFI_ERROR(status)) {
    Print(L"FAILED LOL LocateProtocol\n");
    return -1;
  }

  uint32_t i;
  uint16_t len = 0x2000;
  uint8_t buf[len];
  uint32_t nb = 16;
  for (i = 0; i < nb; i++) {
    INFO("REceiving with new ehternet API !\n");
    uint32_t size = eth->eth_recv(buf, len, EFI_82579LM_API_BLOCK);
    dump(buf, 2, size, 8, (uint32_t)(uintptr_t)buf, 2, 0);
  }

  Print(L"Pointeur du service eth %x\n", eth);

  return EFI_SUCCESS;
}
