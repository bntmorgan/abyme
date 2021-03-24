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

#ifndef __ARP_H__
#define __ARP_H__

#include <efi.h>

#include "types.h"
#include "microudp.h"

uint16_t arp_request(union ethernet_buffer *buffer, uint32_t dst_ip);
uint16_t arp_reply(union ethernet_buffer *buffer, uint32_t dst_ip, uint8_t *mac);
#endif
