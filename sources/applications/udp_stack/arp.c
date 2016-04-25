#include "microudp.h"
#include "string.h"
#include "stdio.h"

uint32_t arp_request(union ethernet_buffer *buffer, uint32_t dst_ip) {

	buffer->frame.eth_header.ethertype = htons(ETHERTYPE_ARP);

	buffer->frame.contents.arp.hwtype = htons(ARP_HWTYPE);
	buffer->frame.contents.arp.proto = htons(ARP_PROTO_IP);
	buffer->frame.contents.arp.hwsize = ARP_HW_SIZE;
	buffer->frame.contents.arp.protosize = ARP_PROTO_SIZE;
	buffer->frame.contents.arp.opcode = htons(ARP_OPCODE_REQUEST);
	buffer->frame.contents.arp.hwsize = ARP_HW_SIZE;
	memcpy(&buffer->frame.contents.arp.sender_mac[0], &buffer->frame.eth_header.srcmac[0], 6);
	buffer->frame.contents.arp.sender_ip = SERVER_IP;
	// We do not know the target MAC
	memcpy(&buffer->frame.contents.arp.target_mac[0], 0, 6);
	buffer->frame.contents.arp.target_ip = dst_ip;

	return sizeof(struct arp_frame);
}
