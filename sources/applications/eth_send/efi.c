#include <efi.h>
#include <efilib.h>

#include "efi/efi_82579LM.h"
#include "microudp.h"
#include "string.h"
#include "stdio.h"

#define BUF2_SIZE 1000

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image, systab);
  EFI_STATUS status;
  protocol_82579LM *eth;
  uint32_t i;
  uint16_t len = 0x100;
  uint8_t buf[0x100];
  uint8_t buf2[BUF2_SIZE];
  struct ethernet_header *frame = (struct ethernet_header *)&buf2[0];
  uint8_t *data = &buf2[0] + sizeof(struct ethernet_header);
  EFI_GUID guid_82579LM = EFI_PROTOCOL_82579LM_GUID;

  status = LibLocateProtocol(&guid_82579LM, (void **)&eth);
  if (EFI_ERROR(status)) {
    INFO("FAILED LOL LocateProtocol\n");
    return -1;
  }

  for (i = 0; i < len ; i++) {
    buf[i] = 0xca;
  }

  // TODO Pourquoi uefi_call_wrapper ne marche pas ??
  //uefi_call_wrapper(eth->send, 3, buf, len, 0);
  INFO("Sending with old ehternet API !\n");
  eth->send(buf, len, 0);

  // We copy source mac address
  memcpy(&frame->srcmac[0], &eth->mac_addr[0], 6);
  // Dst address
  INFO("Dest mac address\n");
  for (i = 0; i < 6; i++) {
    frame->destmac[i] = 0xff;
    printk("%02x ", frame->destmac[i]);
  }
  printk("\n");

  INFO("Source mac address\n");
  for (i = 0; i < 6; i++) {
    printk("%02x ", frame->srcmac[i]);
  }
  printk("\n");

  frame->ethertype = htons(0xb00b);

  // We put some data
  for (i = 0; i < BUF2_SIZE - sizeof(struct ethernet_header); i++) {
    data[i] = 0xca;
  }

  INFO("Sending with new ehternet API !\n");
  eth->eth_send(buf2, BUF2_SIZE, 1);

  INFO("Pointeur du service eth %x\n", eth);

  return EFI_SUCCESS;
}
