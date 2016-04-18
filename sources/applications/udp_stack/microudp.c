#include "microupd.h"


static unsigned char my_mac[6];
// IP add by dhcp discover
static unsigned int my_ip;

/* ARP cache - one entry only */
static uint8_t cached_mac[6];
static uint32_t cached_ip;

static void fill_eth_header(ethernet_header *h, const uint8_t *destmac,
														const uint8_t *srcmac, uint16_t ethertype) {
	int i;
	for(i=0;i<6;i++)
		h->destmac[i] = destmac[i];
	for(i=0;i<6;i++)
		h->srcmac[i] = srcmac[i];
	h->ethertype = ethertype;
}

static void process_arp(void) {
	if(rxbuffer->frame.contents.arp.opcode == ARP_OPCODE_REQUEST) {
		if(rxbuffer->frame.contents.arp.target_ip == my_ip) {
			int i;	
			fill_eth_header(&txbuffer->frame.eth_header,
				rxbuffer->frame.contents.arp.sender_mac,
				my_mac,
				ETHERTYPE_ARP);
			txlen = 68;
			txbuffer->frame.contents.arp.hwtype = ARP_HWTYPE_ETHERNET;
			txbuffer->frame.contents.arp.proto = ARP_PROTO_IP;
			txbuffer->frame.contents.arp.hwsize = 6;
			txbuffer->frame.contents.arp.protosize = 4;
			txbuffer->frame.contents.arp.opcode = ARP_OPCODE_REPLY;
			txbuffer->frame.contents.arp.sender_ip = my_ip;
			for(i=0;i<6;i++)
				txbuffer->frame.contents.arp.sender_mac[i] = my_mac[i];
			txbuffer->frame.contents.arp.target_ip = rxbuffer->frame.contents.arp.sender_ip;
			for(i=0;i<6;i++)
				txbuffer->frame.contents.arp.target_mac[i] = rxbuffer->frame.contents.arp.sender_mac[i];
			send_packet();
		}
		return;
	}
}

void microupd_start(uint8_t *mac_addr, uint32_t ip) {
	for(i=0;i<6;i++)
		my_mac[i] = mac_addr[i];
	my_ip = ip;

	cached_ip = 0;
	for(i=0;i<6;i++)
		cached_mac[i] = 0;	
}
