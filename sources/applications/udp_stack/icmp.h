#ifndef __ICMP_H__
#define __ICMP_H__

#include <efi.h>

#include "types.h"
#include "microudp.h"

uint16_t icmp_request(union ethernet_buffer *buffer);
uint16_t icmp_reply(union ethernet_buffer *buffer);

#endif
