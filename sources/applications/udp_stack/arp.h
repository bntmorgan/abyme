#ifndef __ARP_H__
#define __ARP_H__

#include <efi.h>

#include "types.h"
#include "microudp.h"

uint16_t arp_request(union ethernet_buffer *buffer, uint32_t dst_ip);
uint16_t arp_reply(union ethernet_buffer *buffer, uint32_t dst_ip, uint8_t *mac);
#endif
