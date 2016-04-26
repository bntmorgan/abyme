#include <efi.h>
#include <efilib.h>
#include "stdio.h"
#include "efi/efi_82579LM.h"
#include "string.h"
#include "microudp.h"

#define BUF_SIZE					1500

uint8_t buf2[BUF_SIZE];

void clear_buffer(union ethernet_buffer *buffer) {
	memset(&buf2[0], 0, 1500);
	buffer = (union ethernet_buffer *)&buf2[0];
}

void efi_start_send(union ethernet_buffer *buffer, protocol_82579LM *eth) {
	// Initialize mac address
	//efi_initialize_ethframe(buffer, eth);
	microudp_start(eth->mac_addr, SERVER_IP);

	uint8_t data[5] = {'c','a','c','a',0x0a};
	uint32_t len;
	while((len=microudp_fill(buffer, 6666, 6666, data, 5)) == 0) {
		len = microudp_start_arp(buffer, CLIENT_IP, ARP_OPCODE_REQUEST);
		INFO("Sending ARP with new ethernet API ! size : %d \n", len);
		eth->eth_send(buf2, len, 1);

		clear_buffer(buffer);

		// Wait for reply
		eth->eth_recv(buf2, 42, 1);

		microudp_handle_frame(buffer);

		clear_buffer(buffer);
	}

	INFO("Sending UDP with new ethernet API ! size : %d \n", len);
	eth->eth_send(buf2, len, 1);
}

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image, systab);
	EFI_STATUS status;
  protocol_82579LM *eth;
	uint16_t len;

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

	microudp_start(eth->mac_addr, SERVER_IP);

	// Wait loop
	while(1) {

		clear_buffer(buffer);
		// Wait for a request
		eth->eth_recv(buf2, 1500, 1);

		len = microudp_handle_frame(buffer);
		if (len != 0) {
			eth->eth_send(buf2, len, 1);
		}
	}

	return EFI_SUCCESS;
}
