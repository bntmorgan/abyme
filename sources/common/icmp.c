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

#include "microudp.h"
#include "string.h"
#include "stdio.h"
#include "icmp.h"

uint16_t icmp_reply(union ethernet_buffer *buffer) {
	INFO("Send ICMP reply\n");

	buffer->frame.eth_header.ethertype = htons(ETHERTYPE_IP);

	// Fill ip header
	buffer->frame.contents.icmp.ip.version = IP_IPV4;
	buffer->frame.contents.icmp.ip.diff_services=0;
	buffer->frame.contents.icmp.ip.total_length=htons(sizeof(struct icmp_frame)+56);
	buffer->frame.contents.icmp.ip.identification=0;//htons(0x2fff);
	buffer->frame.contents.icmp.ip.fragment_offset=htons(IP_DONT_FRAGMENT);
	buffer->frame.contents.icmp.ip.ttl=IP_TTL;
	buffer->frame.contents.icmp.ip.proto=IP_PROTO_ICMP;
	buffer->frame.contents.icmp.ip.checksum=0;
	buffer->frame.contents.icmp.ip.dst_ip=buffer->frame.contents.icmp.ip.src_ip;
	buffer->frame.contents.icmp.ip.src_ip=SERVER_IP;

	// Checksum
	buffer->frame.contents.icmp.ip.checksum = htons(ip_checksum(0, &buffer->frame.contents.icmp.ip, sizeof(struct ip_header), 1));

	// Fill icmp frame
	buffer->frame.contents.icmp.type = 0;
	buffer->frame.contents.icmp.code = 0;
	buffer->frame.contents.icmp.checksum = 0;

	return sizeof(struct icmp_frame)+sizeof(struct ethernet_header)+56;
}
