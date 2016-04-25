#include "microudp.h"
#include "stdio.h"
#include "string.h"

uint8_t my_mac[6];
uint32_t my_ip;

/* ARP cache - one entry only */
uint8_t cached_mac[6];
uint32_t cached_ip;

struct pseudo_header {
	unsigned int src_ip;
	unsigned int dst_ip;
	unsigned char zero;
	unsigned char proto;
	unsigned short length;
} __attribute__((packed));

uint16_t ip_checksum(uint32_t r, void *buffer, uint32_t length, int32_t complete)
{
	unsigned char *ptr;
	int i;

	ptr = (unsigned char *)buffer;
	length >>= 1;

	for(i=0;i<length;i++)
		r += ((unsigned int)(ptr[2*i]) << 8)|(unsigned int)(ptr[2*i+1]) ;

	/* Add overflows */
	while(r >> 16)
		r = (r & 0xffff) + (r >> 16);

	if(complete) {
		r = ~r;
		r &= 0xffff;
		if(r == 0) r = 0xffff;
	}
	return r;
}

void microudp_start(uint8_t *macaddr, uint32_t ip)
{
	// Set MAC address
	memcpy(&my_mac[0], &macaddr[0], 6);
	
	// Set IP address
	my_ip = ip;
	
	// Define the ARP cache
	cached_ip = SERVER_IP;
	memcpy(&cached_mac[0], 0, 6);
}

void microudp_set_cache(uint8_t *macaddr) {
  uint32_t i;
	INFO("Client MAC address\n");
  for (i = 0; i < 6; i++) {
    printk("%02x ", macaddr[i]);
  }
  printk("\n");
	memcpy(&cached_mac[0], &macaddr[0], 6);
}

uint16_t microudp_fill_udp(union ethernet_buffer* buffer, uint16_t src_port, \
														uint16_t dst_port, /* XXX voir si cached IP */ \
														uint32_t dst_ip, uint8_t *data, uint32_t len) {
	struct pseudo_header h;
	uint32_t r;
	
	buffer->frame.contents.udp.ip.version = IP_IPV4;
	buffer->frame.contents.udp.ip.diff_services=0;
	buffer->frame.contents.udp.ip.total_length=htons(sizeof(struct udp_frame)+len);
	buffer->frame.contents.udp.ip.identification=0;//htons(0x2fff);
	buffer->frame.contents.udp.ip.fragment_offset=htons(IP_DONT_FRAGMENT);
	buffer->frame.contents.udp.ip.ttl=IP_TTL;
	buffer->frame.contents.udp.ip.proto=IP_PROTO_UDP;
	buffer->frame.contents.udp.ip.checksum=0;
	buffer->frame.contents.udp.ip.src_ip=my_ip;
	buffer->frame.contents.udp.ip.dst_ip=dst_ip;
	
	buffer->frame.contents.udp.udp.src_port = htons(src_port);
	buffer->frame.contents.udp.udp.dst_port = htons(dst_port);
	buffer->frame.contents.udp.udp.length = htons(sizeof(struct udp_header)+len);
	buffer->frame.contents.udp.udp.checksum=0;
	memcpy(&buffer->frame.contents.udp.payload[0], &data[0], len);
	buffer->frame.contents.udp.ip.checksum = htons(ip_checksum(0, &buffer->frame.contents.udp.ip, sizeof(struct ip_header), 1));

	// Checksum	
	h.proto = buffer->frame.contents.udp.ip.proto;
	h.src_ip = buffer->frame.contents.udp.ip.src_ip;
	h.dst_ip = buffer->frame.contents.udp.ip.dst_ip;
	h.length = buffer->frame.contents.udp.udp.length; 
	h.zero = 0;
	r = ip_checksum(0, &h, sizeof(struct pseudo_header), 0);
	if(len & 1) {
		buffer->frame.contents.udp.payload[len]=0;
		len++;
	}		
	r = ip_checksum(r, &buffer->frame.contents.udp.udp, sizeof(struct udp_header)+len, 1);
	buffer->frame.contents.udp.udp.checksum = htons(r);
	
	return (sizeof(struct udp_frame)+len);
}
