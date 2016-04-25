#ifndef __ARP_H__
#define __ARP_H__

#include <efi.h>

#include "types.h"
#include "microudp.h"

uint32_t arp_request(union ethernet_buffer *buffer, uint32_t dst_ip);

#endif
