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

#include "stdio.h"
#include "string.h"
#include "microudp.h"
#include "arp.h"
#include "icmp.h"

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

uint16_t ip_checksum(uint32_t r, void *buffer, uint32_t length,
    int32_t complete) {
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

void print_arpframe(struct arp_frame arp) {
  printk("ARP(0x%016X)\n", arp);
  printk("  hwtype(0x%04x)\n", htons(arp.hwtype));
  printk("  proto(0x%04x)\n", htons(arp.proto));
  printk("  hwsize(0x%02x)\n", arp.hwsize);
  printk("  protosize(0x%02x)\n", arp.protosize);
  printk("  opcode(0x%04x)\n", htons(arp.opcode));
  printk("  sender_mac(0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x)\n",
            arp.sender_mac[0], arp.sender_mac[1], arp.sender_mac[2],
            arp.sender_mac[3], arp.sender_mac[4], arp.sender_mac[5]);
  printk("  sender_ip(0x%08x)\n", arp.sender_ip);
  printk("  target_mac(0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x)\n",
            arp.target_mac[0], arp.target_mac[1], arp.target_mac[2],
            arp.target_mac[3], arp.target_mac[4], arp.target_mac[5]);
  printk("  target_ip(0x%08x)\n", arp.target_ip);
}

void print_ethframe(struct ethernet_header eth_head) {
  printk("Ethernet(0x%016X)\n", eth_head);
  printk("  destmac(0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x)\n",
            eth_head.destmac[0], eth_head.destmac[1], eth_head.destmac[2],
            eth_head.destmac[3], eth_head.destmac[4], eth_head.destmac[5]);
  printk("  srcmac(0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x)\n",
            eth_head.srcmac[0], eth_head.srcmac[1], eth_head.srcmac[2],
            eth_head.srcmac[3], eth_head.srcmac[4], eth_head.srcmac[5]);
  printk("  ethertype(0x%04x)\n", htons(eth_head.ethertype));
}

void microudp_start(uint8_t *macaddr, uint32_t ip)
{
  uint32_t i;
  // Set MAC address
  memcpy(&my_mac[0], &macaddr[0], 6);

  // Set IP address
  my_ip = ip;

  // Define the ARP cache
  cached_ip = 0;
  for (i = 0; i < 6; i++) {
    cached_mac[i] = 0xff;
  }
}

void microudp_initialize_ethframe(union ethernet_buffer* buffer) {
  //uint32_t i;
  // We copy source mac address
  memcpy(&buffer->frame.eth_header.srcmac[0], &my_mac[0], 6);
  // Dst address
  memcpy(&buffer->frame.eth_header.destmac[0], &cached_mac[0], 6);
  //INFO("Dest mac address\n");
  /*for (i = 0; i < 6; i++) {
    printk("%02x ", buffer->frame.eth_header.destmac[i]);
  }
  printk("\n");

  // Src address
  //INFO("Source mac address\n");
  for (i = 0; i < 6; i++) {
    printk("%02x ", buffer->frame.eth_header.srcmac[i]);
  }
  printk("\n");*/
}

void microudp_set_cache(union ethernet_buffer *buffer) {
  /*uint32_t i;
  //INFO("New client MAC address\n");
  for (i = 0; i < 6; i++) {
    printk("%02x ", buffer->frame.contents.arp.sender_mac[i]);
  }
  printk("\n");*/

  // Save the Client MAC
  memcpy(&cached_mac[0], &buffer->frame.contents.arp.sender_mac[0], 6);
}

uint16_t microudp_start_arp(union ethernet_buffer *buffer, uint32_t ip,
                            uint16_t opcode) {
  uint16_t len;
  //INFO("START ARP\n");

  // Save the target IP
  cached_ip = ip;

  // Initialize ethernet frame
  microudp_initialize_ethframe(buffer);

  if(opcode==ARP_OPCODE_REQUEST) {
    len = arp_request(buffer, ip);
  } else {
    len = arp_reply(buffer, ip, cached_mac);
  }
  return len+sizeof(struct ethernet_header);
}

uint16_t microudp_start_icmp(union ethernet_buffer *buffer, uint8_t type) {
  uint16_t len;

  //INFO("Start ICMP\n");

  // Initialize ethernet frame
  microudp_initialize_ethframe(buffer);

  if(type==0) {
    len = icmp_reply(buffer);
  } else {
    len = 0;
  }
  return len;
}

uint16_t microudp_fill(union ethernet_buffer* buffer, uint16_t src_port,
                           uint16_t dst_port, uint8_t *data, uint32_t len) {
  struct pseudo_header h;
  uint32_t r;

  if((cached_mac[0] == 0xff) && (cached_mac[1] == 0xff) && (cached_mac[2] == 0xff)
    && (cached_mac[3] == 0xff) && (cached_mac[4] == 0xff) && (cached_mac[5] == 0xff)) {
    //INFO("No address MAC\n");
    return 0;
  }

  //INFO("Address MAC found, ok to send\n");
  microudp_initialize_ethframe(buffer);

  // Add the ethertype
  buffer->frame.eth_header.ethertype = htons(ETHERTYPE_IP);

  // Fill IP header
  buffer->frame.contents.udp.ip.version = IP_IPV4;
  buffer->frame.contents.udp.ip.diff_services=0;
  buffer->frame.contents.udp.ip.total_length=htons(sizeof(struct udp_frame)+len);
  buffer->frame.contents.udp.ip.identification=0;//htons(0x2fff);
  buffer->frame.contents.udp.ip.fragment_offset=htons(IP_DONT_FRAGMENT);
  buffer->frame.contents.udp.ip.ttl=IP_TTL;
  buffer->frame.contents.udp.ip.proto=IP_PROTO_UDP;
  buffer->frame.contents.udp.ip.checksum=0;
  buffer->frame.contents.udp.ip.src_ip=my_ip;
  buffer->frame.contents.udp.ip.dst_ip=cached_ip;

  // Fill UDP header
  buffer->frame.contents.udp.udp.src_port = htons(src_port);
  buffer->frame.contents.udp.udp.dst_port = htons(dst_port);
  buffer->frame.contents.udp.udp.length = htons(sizeof(struct udp_header)+len);
  buffer->frame.contents.udp.udp.checksum=0;
  memcpy(&buffer->frame.contents.udp.payload[0], &data[0], len);

  // Checksum IP
  buffer->frame.contents.udp.ip.checksum = htons(ip_checksum(0, &buffer->frame.contents.udp.ip, sizeof(struct ip_header), 1));

  // Checksum UDP
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

  return (sizeof(struct udp_frame)+len+sizeof(struct ethernet_header));
}

uint16_t microudp_handle_frame(union ethernet_buffer *in,
    union ethernet_buffer *out) {
  uint16_t len = 0;

  // Test if request receive
  if(in->frame.eth_header.ethertype == htons(ETHERTYPE_ARP) &&
    in->frame.contents.arp.opcode == htons(ARP_OPCODE_REQUEST) &&
    in->frame.contents.arp.target_ip == SERVER_IP &&
    in->frame.contents.arp.sender_ip == CLIENT_IP) {

    //INFO("ARP request receive for us\n");

    microudp_set_cache(in);

    if (out == NULL) {
      ERROR("Critical error : unexpected output frame\n");
    }

    len = microudp_start_arp(out, CLIENT_IP, ARP_OPCODE_REPLY);

  // Test if reply receive
  } else if(in->frame.eth_header.ethertype == htons(ETHERTYPE_ARP) &&
    in->frame.contents.arp.opcode == htons(ARP_OPCODE_REPLY) &&
    in->frame.contents.arp.target_ip == SERVER_IP &&
    in->frame.contents.arp.sender_ip == CLIENT_IP)  {

    microudp_set_cache(in);

  // Test if icmp echo request receive
  } else if (in->frame.eth_header.ethertype == htons(ETHERTYPE_IP) &&
            in->frame.contents.icmp.type == 0x08) {

    //INFO("ICMP request\n");

    if (out == NULL) {
      ERROR("Critical error : unexpected output frame\n");
    }

    len = microudp_start_icmp(out, 0);

  // Everything else for now is WTF
  } else {
    // INFO("UNSUPPORTED protocol\n");
  }

  return len;
}
