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

#define BUF_SIZE 1000

// static uint8_t rb[BUF_SIZE];
static uint8_t sb[BUF_SIZE];
static protocol_82579LM *eth;

static uint32_t dst_ip = IP_TO_INT(192,168,0,1);
static uint32_t src_ip = IP_TO_INT(192,168,0,2);

static uint8_t mac_brd[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

void arp_request(void) {
  struct ethernet_header *eh = (struct ethernet_header *)&sb[0];
  struct arp_frame *af = (struct arp_frame*)&sb[0];
  // Init buffer
  memset(&sb[0], 0, BUF_SIZE);
  // Ethernet
  memcpy(&eh->destmac[0], &mac_brd[0], 6);
  memcpy(&eh->srcmac[0], eth->mac_addr, 6);
  eh->ethertype = htons(ETHERTYPE_ARP);
  // ARP
  af->hwtype = htons(ARP_HWTYPE);
  af->proto = htons(ARP_PROTO_IP);
  af->hwsize = ARP_HW_SIZE;
  af->protosize = ARP_PROTO_SIZE;
  af->opcode = ARP_OPCODE_REQUEST;
  memcpy(&af->sender_mac[0], eth->mac_addr, 6);
  af->sender_ip = src_ip;
  // we do not know target mac
  af->target_ip = dst_ip;
}

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image, systab);
  EFI_STATUS status;
  EFI_GUID guid_82579LM = EFI_PROTOCOL_82579LM_GUID;

  // Print to shell
  putc = &shell_print;

  status = LibLocateProtocol(&guid_82579LM, (void **)&eth);
  if (EFI_ERROR(status)) {
    INFO("FAILED LOL LocateProtocol\n");
    return -1;
  }

  return EFI_SUCCESS;
}
