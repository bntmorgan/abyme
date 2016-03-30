#include "stdio.h"
#include "debug_protocol.h"
#include "efi/efi_82579LM.h"
#include "efiw.h"
#include "string.h"

#define NETBOOT_PAGES 250

uint8_t *b;
uint8_t *sb;
uint8_t *rb;

protocol_82579LM *eth;

void netboot(EFI_HANDLE parent_image) {
  uint64_t length = 0;
  uint64_t total = 0;
  // Info message buffer
  b = efi_allocate_pages(NETBOOT_PAGES);
  rb = efi_allocate_pages(1);
  sb = efi_allocate_pages(1);

  EFI_STATUS status;
  EFI_GUID guid_82579LM = EFI_PROTOCOL_82579LM_GUID;

  // Get ethernet driver protocol interface
  status = LibLocateProtocol(&guid_82579LM, (void **)&eth);
  if (EFI_ERROR(status)) {
    INFO("FAILED LOL LocateProtocol\n");
  } else {
    INFO("Netboot server enabled\n");
    printk("NETBOOT SERVER INIT : ETH BAR0 %X\n", eth->bar0);
  }

  // Send the netboot message
  message_netboot *nm = (message_netboot*)&sb[0];
  nm->type = MESSAGE_NETBOOT;
  nm->vmid = 0;

  eth->send(sb, sizeof(message_netboot), EFI_82579LM_API_BLOCK);

  INFO("Downloading ...\n");
  // Receiving the image
  while(1) {
    eth->recv(rb, eth->mtu, EFI_82579LM_API_BLOCK);
    message *rm = (message*)rb;
    if (rm->type != MESSAGE_MEMORY_WRITE) {
      ERROR("YOLO OWNED downloading failed");
    }
    message_memory_write *mw = (message_memory_write*)rm;
    length = mw->length;
    // INFO("Receiving 0x%016X bytes\n", length);
    // Copy the data
    memcpy(&b[total], rb + sizeof(message_memory_write), length);
    // Increment the index
    total += length;
    if (total > (NETBOOT_PAGES << 12)) {
      ERROR("No more space in the downloading buffer\n");
    }
    // Send the commit message
    message_commit *mc = (message_commit*)&sb[0];
    mc->type = MESSAGE_COMMIT;
    mc->vmid = 0;
    mc->ok = 1;
    eth->send(sb, sizeof(message_commit), EFI_82579LM_API_BLOCK);
    if (length == 0) {
      break;
    }
  }
  INFO("Downloading is finished : 0x%016X bytes\n", total);
  INFO("Executing...\n");

  status = efi_execute_image(parent_image, b, total);
  if (status) {
    ERROR("YOLO Failed to load the image BOLLOCKS!\n");
  }
}
