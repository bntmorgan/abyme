#include <efi.h>
#include <efilib.h>

#include "efi/efi_82579LM.h"
#include "microudp.h"
#include "string.h"
#include "stdio.h"

#define BUF_SIZE 1000

// static uint8_t rb[BUF_SIZE];
static uint8_t sb[BUF_SIZE];
static protocol_82579LM *eth;

static uint32_t dst_ip = IP_TO_INT(192,168,0,1);
static uint32_t src_ip = IP_TO_INT(192,168,0,2);

static uint8_t mac_brd[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

void arp_request(void) {
  struct ethernet_header *eh = (struct ethernet_header *)&sb[0];
  struct arp_frame *af = (struct arp_frame*)&sb[0];
  // Init buffer
  memset(&sb[0], 0, BUF_SIZE);
  // Ethernet
  memcpy(&eh->destmac[0], &mac_brd[0], 6);
  memcpy(&eh->srcmac[0], eth->mac_addr, 6);
  eh->ethertype = htons(ETHERTYPE_ARP);
  // ARP
  af->hwtype = htons(ARP_HWTYPE);
  af->proto = htons(ARP_PROTO_IP);
  af->hwsize = ARP_HW_SIZE;
  af->protosize = ARP_PROTO_SIZE;
  af->opcode = ARP_OPCODE_REQUEST;
  memcpy(&af->sender_mac[0], eth->mac_addr, 6);
  af->sender_ip = src_ip;
  // we do not know target mac
  af->target_ip = dst_ip;
}

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image, systab);
  EFI_STATUS status;
  EFI_GUID guid_82579LM = EFI_PROTOCOL_82579LM_GUID;

  status = LibLocateProtocol(&guid_82579LM, (void **)&eth);
  if (EFI_ERROR(status)) {
    INFO("FAILED LOL LocateProtocol\n");
    return -1;
  }

  return EFI_SUCCESS;
}
