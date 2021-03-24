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

#include "82579LM.h"
#include "debug_eth.h"
#include "api.h"
#include "pci.h"
#include "efi/efi_82579LM.h"
#include "stdio.h"
#include "shell.h"
#include "debug.h"

EFI_STATUS vmm_rt_unload (IN EFI_HANDLE image);

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image_handle, systab);

  // Print to shell
  putc = &shell_print;

#ifdef _QEMU
  qemu_send_address("82579LM.efi");
#endif

  INFO("Experimental Intel 82579LM Ethernet driver initialization\n");

  if (eth_setup()) {
    return EFI_NOT_FOUND;
  }
  if (eth_init()) {
    INFO("Error while initializing\n");
    return EFI_NOT_FOUND;
  }

  EFI_STATUS status;
//  // Add an unload handler
//  EFI_LOADED_IMAGE *image;
//  status = uefi_call_wrapper(BS->HandleProtocol, 4, &image_handle,
//      &LoadedImageProtocol, (void*)&image);
//  ASSERT (!EFI_ERROR(status));
//  image->Unload = vmm_rt_unload;
//
//  INFO("Unload handler added %x\n", (uintptr_t)image_handle);

  // Share the pci address in the uefi protocol
  pci_device_addr *pci_addr = eth_get_pci_addr();
  proto.pci_addr.bus = pci_addr->bus;
  proto.pci_addr.device = pci_addr->device;
  proto.pci_addr.function = pci_addr->function;
  proto.bar0 = (uintptr_t)bar0;

  // Add a protocol so someone can locate us
  status = uefi_call_wrapper(BS->InstallProtocolInterface, 4, &image_handle,
      &guid_82579LM, (EFI_INTERFACE_TYPE)NULL, &proto);
  ASSERT (!EFI_ERROR(status));

  // Copy the image handle for protocol uninstallation
  ih = image_handle;

  // eth_disable_debug();

  return status;
}

EFI_STATUS vmm_rt_unload (IN EFI_HANDLE image) {
  INFO("Driver unload :(\n");
  return (EFI_SUCCESS);
}
