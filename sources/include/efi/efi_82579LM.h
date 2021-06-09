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

#ifndef __EFI_82579_H__
#define __EFI_82579_H__

#include <efi.h>

#define EFI_PROTOCOL_82579LM_GUID {0xcc2ac9d1, 0x14a9, 0x11d3,\
    {0x8e, 0x77, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

#define EFI_82579LM_API_BLOCK (1 << 0)

typedef struct _debug_message_vmexit_notification {
  uint32_t type;
  uint32_t exit_reason;
} debug_message_vmexit_notification;

typedef union _debug_message {
  uint32_t type;
} debug_message;

typedef struct _protocol_82579LM {
  // Deprecated
  uint32_t (*send)(const void *, uint32_t, uint8_t);
  uint32_t (*recv)(void *, uint32_t, uint8_t);
  // Ethernet send and receive
  void (*eth_send)(const void *buf, uint16_t len, uint8_t block);
  int (*eth_recv)(void *buf, uint32_t len, uint8_t block);
  int (*get_level)(void);
  int (*uninstall)(void);
  struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
  } pci_addr;
  uint64_t bar0;
  uint64_t mtu;
  uint8_t  mac_addr[6];
} protocol_82579LM;

extern uint8_t *bar0;

#endif
