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

#include "pci.h"
#include "efi/efi_eric.h"
#include "debug.h"
#include "stdio.h"
#include "shell.h"

// 
// stdio putc pointer
//
extern void (*putc)(uint8_t);

void put_nothing(uint8_t c) {}

void eric_disable_debug(void){
  putc = &put_nothing;
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image_handle, systab);

  // Print to shell
  putc = &shell_print;

  EFI_STATUS status = EFI_SUCCESS;
  uint32_t id;
  pci_device_addr addr = {0x00, 0x1c, 0x04};

  INFO("Experimental Intel root port driver initialization\n");

  id = PCI_MAKE_ID(addr.bus, addr.device, addr.function);

  // Powerup PIO & MMI_extO
  pci_writew(id, PCI_CONFIG_COMMAND, 0x7);
  // pci_writeb(id, PCI_CONFIG_INTERRUPT_LINE, 0x5);
  
  INFO("STATUS 0x%04x\n", pci_readw(id, PCI_CONFIG_STATUS));
  INFO("COMMAND 0x%04x\n", pci_readw(id, PCI_CONFIG_COMMAND));

  return status;
}
