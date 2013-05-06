#include <efi.h>
#include <efilib.h>

#include "api.h"
#include "82579LM.h"
#include "efi/efi_82579LM.h"
#include "string.h"
#include "debug.h"

static inline uint16_t htons(uint16_t data) {
  return ((data >> 8) & 0xff) | ((data << 8) & 0xff00);
}

static inline uint16_t ntohs(uint16_t data) {
  return htons(data);
}

void _api_dump_eth_frame(void *frame, uint32_t payload_len) {
  // Addresses
  Print(L"MAC Addresses\n");
  dump((void *)frame, 6, 12, 0, 6);
  // Type
  Print(L"Type\n");
  dump(((void *)frame) + 12, 2, 2, 0, 2);
  // Payload
  Print(L"Payload\n");
  dump(((void *)frame) + sizeof(eth_header), 4, payload_len, 0, 4);
}

uint32_t send(const void *buf, uint32_t len, uint8_t flags) {
  static uint8_t frame[ETH_LEN];
  if (len > ETH_MTU) {
    return -1;
  }
  uint32_t size = len + sizeof(eth_header);
  eth_header *eh = (eth_header *)&frame[0];
  eth_addr daddr = {.n = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
  eth_addr *laddr = eth_get_laddr();
  uint32_t i;
  for (i = 0; i < sizeof(eth_addr); i++) {
    eh->src.n[i] = laddr->n[i];
    eh->dst.n[i] = daddr.n[i];
  }
  eh->type = htons(API_ETH_TYPE);
  eh++;
  // Copy the payload into the frame
  memcpy(eh, (void *)buf, len);
  eth_send(frame, size, (flags & EFI_82579LM_API_BLOCK));
  return len;
}

uint32_t recv(void *buf, uint32_t len, uint8_t flags) {
  static uint8_t frame[ETH_LEN];
  if (len > ETH_MTU) {
    return -1;
  }
  uint32_t size = len + sizeof(eth_header);
  uint32_t l = eth_recv(frame, size, (flags & EFI_82579LM_API_BLOCK));
  l -= sizeof(eth_header);
  if (l > len) {
    // Received packet is too long
    return -1;
  }
  // Get the header
  eth_header *eh = (eth_header *)frame; 
  // _api_dump_eth_frame(frame, l);
  if (ntohs(eh->type) != API_ETH_TYPE){
    return -1;
  }
  memcpy(buf, frame + sizeof(eth_header), l);
  return l;
}