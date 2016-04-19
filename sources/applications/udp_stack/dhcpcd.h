#ifndef __DHCPCD_H__
#define __DHCPCD_H__

#include <efi.h>

#include "types.h"

#define DHCP_OPE												0x01
#define DHCP_HWTYPE											0x01
#define DHCP_HW_SIZE										0x06
#define DHCP_HOPS												0x00

#define DHCP_FILE												"\0"

#define DHCP_PORT_CLIENT								0x0044
#define DHCP_PORT_SERVEUR								0x0043

#define MAGIC_COOKIE										0x63825363

#define DHCP_CODEOPT										0x35
#define DHCP_CODEEND										0xFF
#define DHCP_LENOPT											0x01

#define DHCP_DISCOVER										0x01
#define DHCP_OFFER											0x02
#define DHCP_REQUEST										0x03
#define DHCP_ACK												0x05


struct dhcp_frame {
	uint8_t op;
	uint8_t hwtype;
	uint8_t hlen;
	uint8_t hops;
	uint32_t xid;
	uint16_t secs;
	uint16_t flags;
	uint32_t ciaddr;
	uint32_t yiaddr;
	uint32_t siaddr;
	uint32_t giaddr;
	uint8_t chaddr[6];
	uint8_t sname[64];
	uint8_t file[128];
	uint8_t options[576];
} __attribute__((packed));

void dhcpcd_send_discover();

#endif//__DHCPCD_H__
