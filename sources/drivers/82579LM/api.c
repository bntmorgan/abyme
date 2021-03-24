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

#include "api.h"
#include "82579LM.h"
#include "efi/efi_82579LM.h"
#include "string.h"
#include "debug.h"
#include "stdio.h"

EFI_HANDLE ih;
EFI_GUID guid_82579LM = EFI_PROTOCOL_82579LM_GUID;

//
// Protocol structure definition
//
protocol_82579LM proto = {
  send,
  recv,
  eth_send,
  eth_recv,
  get_level,
  uninstall,
  .pci_addr = {
    0,
    0,
    0
  },
  0,
  ETH_MTU,
  {0, 0, 0, 0, 0, 0},
};

static uint32_t level = 0;

static inline uint16_t htons(uint16_t data) {
  return ((data >> 8) & 0xff) | ((data << 8) & 0xff00);
}

static inline uint16_t ntohs(uint16_t data) {
  return htons(data);
}

void _api_dump_eth_frame(void *frame, uint32_t payload_len) {
  // Addresses
  INFO("MAC Addresses\n");
  dump((void *)frame, 6, 12, 8, 0, 6, 0);
  // Type
  INFO("Type\n");
  dump(((void *)frame) + 12, 2, 2, 8, 0, 2, 0);
  // Payload
  INFO("Payload len %d\n", payload_len);
  // dump(((void *)frame) + sizeof(eth_header), 1, payload_len, 0, 1);
}

void test_send() {
  static uint8_t test_buf[0x100] = {'H', 'E', 'L', 'L', 'O'};
  INFO("Sending test frame\n");
  send(test_buf, 0x100, EFI_82579LM_API_BLOCK);
}

uint32_t send(const void *buf, uint32_t len, uint8_t flags) {
  static uint8_t frame[ETH_LEN];
  if (len > ETH_MTU) {
    return -1;
  }
  uint32_t size = len + sizeof(eth_header);
  eth_header *eh = (eth_header *)&frame[0];
  // XXX destination MAC address
  // eth_addr daddr = {.n = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
  eth_addr daddr = {.n = {0xb4, 0xb5, 0x2f, 0xfc, 0x28, 0xce}}; // eth0 tatooine
  //eth_addr daddr = {.n = {0xce, 0x28, 0xfc, 0x2f, 0xb5, 0xb4}};
  eth_addr *laddr = eth_get_laddr();
  uint32_t i;
  for (i = 0; i < sizeof(eth_addr); i++) {
    eh->src.n[i] = laddr->n[i];
    eh->dst.n[i] = daddr.n[i];
  }
  eh->type = htons(API_ETH_TYPE);
  // Go to the payload zone
  eh++;
  // Copy the payload into the frame
  memcpy(eh, (void *)buf, len);
  eth_send(frame, size, (flags & EFI_82579LM_API_BLOCK));
  // _api_dump_eth_frame((void *)frame, len);
  return len;
}

uint32_t recv(void *buf, uint32_t len, uint8_t flags) {
  static uint8_t frame[ETH_LEN];
	if (len > ETH_MTU) {
    return -1;
  }
  uint32_t size = len + sizeof(eth_header);
  uint32_t l = eth_recv(frame, size, (flags & EFI_82579LM_API_BLOCK));
  l -= sizeof(eth_header);
  if (l > len) {
    // Received packet is too long
    return -1;
  }
  // Get the header
  eth_header *eh = (eth_header *)frame; 
  _api_dump_eth_frame(frame, l);
  if (ntohs(eh->type) != API_ETH_TYPE){
    return -1;
  }
  memcpy(buf, frame + sizeof(eth_header), l);
  return l;
}

int get_level(void) {
  uint32_t l = level;
  level++;;
  return l;
}

// Used by the first hypervisor owning the network card
int uninstall(void) {
  return uefi_call_wrapper(BS->UninstallProtocolInterface, 3, ih,
      &guid_82579LM, &proto);
}
