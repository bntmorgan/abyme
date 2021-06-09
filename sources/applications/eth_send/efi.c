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
#include "microudp.h"
#include "string.h"
#include "stdio.h"
#include "shell.h"

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image, systab);
  EFI_STATUS status;
  protocol_82579LM *eth;
  char buf[ETHERNET_SIZE];
  EFI_GUID guid_82579LM = EFI_PROTOCOL_82579LM_GUID;
  uint32_t len;
  union ethernet_buffer *eb = (union ethernet_buffer *)&buf[0];

  // Print to shell
  putc = &shell_print;

  status = LibLocateProtocol(&guid_82579LM, (void **)&eth);
  if (EFI_ERROR(status)) {
    INFO("FAILED LOL LocateProtocol\n");
    return -1;
  }

  INFO("Pointeur du service eth %x\n", eth);

  // Initialize microUDP
  microudp_start(eth->mac_addr, SERVER_IP);

  // Start with ARP request
  INFO("ARP who-has 192.168.0.1 sent\n");
  memset(&buf[0], 0, ETHERNET_SIZE);
  len = microudp_start_arp(eb, CLIENT_IP, ARP_OPCODE_REQUEST);
  dump(buf, 2, len, 8, (uint32_t)(uintptr_t)buf, 2, 0);
  eth->eth_send(buf, len, 1);

  INFO("Wait for ARP is-at reply\n");
  memset(&buf[0], 0, ETHERNET_SIZE);
  eth->eth_recv(buf, ETHERNET_SIZE, 1);
  microudp_handle_frame(eb, NULL);

  INFO("DONE\n");

  return EFI_SUCCESS;
}
