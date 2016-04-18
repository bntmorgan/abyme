#ifndef __microupd_h__
#define __microupd_h__

#include <efi.h>

#include "types.h"

#define ETHERTYPE_ARP 					0x0806 	// Code ethertype pour ARP
#define ETHERTYPE_IP 						0x0800 	// Code ethertype pour IP

#define ARP_HWTYPE_ETHERNET 		0x0001	// Hardware type : 0x0001 pour Ethernet
#define ARP_PROTO_IP        		0x0800	// Protocol type : 0x0800 pour IP

#define ARP_HW_SIZE							0x06		// Size of hardware address
#define ARP_PROTO_SIZE					0x04		// Size of network address

// #define ARP_OPCODE_REQUEST  	0x0001	// Operation Code request
#define ARP_OPCODE_REPLY    		0x0002	// Operation Code reply

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
	uint8_t padding[18];
} __attribute__((packed)) arp_frame;

#define IP_IPV4									0x45		// IPv4 version + Header length
#define IP_DONT_FRAGMENT				0x4000	// Flags don't fragment
#define IP_TTL									64			// Time to live
#define IP_PROTO_UDP						0x11		// UDP above

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
} __attribute__((packed)) ip_header;

// Define UDP header
struct udp_header {
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t length;
	uint16_t checksum;
} __attribute__((packed)) udp_header;

// Define UDP frame
struct udp_frame {
	ip_header ip;
	udp_header udp;
	char payload[];
} __attribute__((packed)) udp_frame;

/* ARP cache - one entry only */
static uint8_t cached_mac[6];
static uint32_t cached_ip;

