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

#ifndef __MICROUDP_H__
#define __MICROUDP_H__

// Note : Milkymist : https://github.com/m-labs/milkymist/blob/master/software/libnet/microudp.c
#include <efi.h>

#include "types.h"

#define IP_TO_INT(_a_, _b_, _c_, _d_) \
  (((_d_) << 24) | ((_c_) << 16) | ((_b_) << 8) | ((_a_) << 0))

#define ETHERTYPE_ARP           0x0806  // Code ethertype pour ARP
#define ETHERTYPE_IP            0x0800  // Code ethertype pour IP

#define SERVER_IP               IP_TO_INT(192,168,0,2)
#define CLIENT_IP               IP_TO_INT(192,168,0,1)

#define ETHERNET_SIZE 1532

struct ethernet_header {
  //uint8_t preamble[8];
  uint8_t destmac[6];
  uint8_t srcmac[6];
  uint16_t ethertype;
} __attribute__((packed));

#define ARP_HWTYPE              0x0001  // Hardware type : 0x0001 pour Ethernet
#define ARP_PROTO_IP            0x0800  // Protocol type : 0x0800 pour IP

#define ARP_HW_SIZE             0x06    // Size of hardware address
#define ARP_PROTO_SIZE          0x04    // Size of network address

#define ARP_OPCODE_REQUEST      0x0001  // Operation Code request
#define ARP_OPCODE_REPLY        0x0002  // Operation Code reply

// Define ARP frame
struct arp_frame {
  uint16_t hwtype;
  uint16_t proto;
  uint8_t hwsize;
  uint8_t protosize;
  uint16_t opcode;
  uint8_t sender_mac[6];
  uint32_t sender_ip;
  uint8_t target_mac[6];
  uint32_t target_ip;
} __attribute__((packed));

#define IP_IPV4                 0x45    // IPv4 version + Header length
#define IP_DONT_FRAGMENT        0x4000  // Flags don't fragment
#define IP_TTL                  64      // Time to live
#define IP_PROTO_UDP            0x11    // UDP above
#define IP_PROTO_ICMP           0x01    // ICMP above

// Define IP header
struct ip_header {
  uint8_t version;
  uint8_t diff_services;
  uint16_t total_length;
  uint16_t identification;
  uint16_t fragment_offset;
  uint8_t ttl;
  uint8_t proto;
  uint16_t checksum;
  uint32_t src_ip;
  uint32_t dst_ip;
} __attribute__((packed));

// Define UDP header
struct udp_header {
  uint16_t src_port;
  uint16_t dst_port;
  uint16_t length;
  uint16_t checksum;
} __attribute__((packed));

// Define UDP frame
struct udp_frame {
  struct ip_header ip;
  struct udp_header udp;
  uint8_t payload[];
} __attribute__((packed));

struct icmp_frame {
  struct ip_header ip;
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint8_t data[4];
  uint8_t payload[];
} __attribute__((packed));

struct ethernet_frame {
  struct ethernet_header eth_header;
  union {
    struct arp_frame arp;
    struct udp_frame udp;
    struct icmp_frame icmp;
  } contents;
} __attribute__((packed));

union ethernet_buffer {
  struct ethernet_frame frame;
  uint8_t raw[1532];
};

static inline uint16_t htons(uint16_t data) {
  return ((data >> 8) & 0xff) | ((data << 8) & 0xff00);
}

static inline uint16_t ntohs(uint16_t data) {
  return htons(data);
}

uint16_t ip_checksum(uint32_t r, void *buffer, uint32_t length, int32_t complete);

void print_arpframe(struct arp_frame arp);
void print_ethframe(struct ethernet_header eth_head);

// Initialize ARP cache and server configuration
void microudp_start(uint8_t *macaddr, uint32_t ip);

// Fill packet to send if Client MAC address is known
uint16_t microudp_fill(union ethernet_buffer* buffer, uint16_t src_port,
                           uint16_t dst_port, uint8_t *data, uint32_t len);

// Fill the ARP cache
void microudp_set_cache(union ethernet_buffer *buffer);

// Fill ARP frame for request
uint16_t microudp_start_arp(union ethernet_buffer *buffer, uint32_t ip,
                            uint16_t opcode);

uint16_t microudp_handle_frame(union ethernet_buffer* buffer);
#endif//__MICROUDP_H__
