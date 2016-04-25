#include <efi.h>
#include <efilib.h>
#include "stdio.h"
#include "efi/efi_82579LM.h"
#include "string.h"
#include "microudp.h"
#include "arp.h"

#define BUF_SIZE					1500

uint8_t buf2[BUF_SIZE];
uint8_t buf1[BUF_SIZE];

void efi_initialize_ethframe(union ethernet_buffer* buffer, \
														 protocol_82579LM *eth) {
  uint32_t i;
  // We copy source mac address
  memcpy(&buffer->frame.eth_header.srcmac[0], &eth->mac_addr[0], 6);
  // Dst address
	INFO("Dest mac address\n");
	for (i = 0; i < 6; i++) {
  	buffer->frame.eth_header.destmac[i] = 0xff;
  	printk("%02x ", buffer->frame.eth_header.destmac[i]);
	}
	printk("\n");
	
	// Src address
  INFO("Source mac address\n");
  for (i = 0; i < 6; i++) {
    printk("%02x ", buffer->frame.eth_header.srcmac[i]);
  }
  printk("\n");
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

uint32_t efi_start_arp(union ethernet_buffer *buffer, protocol_82579LM *eth) {
	// Send an ARP request
  uint32_t len = arp_request(buffer, CLIENT_IP);

	INFO("Sending with new ethernet API ! %d \n", len);
	eth->eth_send(buf2, len+sizeof(struct ethernet_header), 1);
	
	// Wait for the reply
  memset(&buf2[0], 0, 1500);
	buffer = (union ethernet_buffer *)&buf2[0];

	eth->eth_recv(buf2, 42, 1);
	print_ethframe(buffer->frame.eth_header);
	print_arpframe(buffer->frame.contents.arp);
  INFO("RECEIVE\n");

	// Set the cache	
	microudp_set_cache(buffer->frame.contents.arp.sender_mac);	
	
	return 0;
}

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image, systab);
	EFI_STATUS status;
  protocol_82579LM *eth;
	
	// Initialize sending buffer
  memset(&buf2[0], 0, 1500);

	// Initialize ethernet buffer
	union ethernet_buffer *buffer = (union ethernet_buffer *)&buf2[0];
  
	// Identify the ethernet protocol
	EFI_GUID guid_82579LM = EFI_PROTOCOL_82579LM_GUID;
	
	// Locate it
  status = LibLocateProtocol(&guid_82579LM, (void **)&eth);
  if (EFI_ERROR(status)) {
    INFO("FAILED LOL LocateProtocol\n");
    return -1;
  }
	
	// Initialize mac address
	efi_initialize_ethframe(buffer, eth);
		
	// Start dhcp client
	efi_start_arp(buffer, eth);	

  return EFI_SUCCESS;
}
