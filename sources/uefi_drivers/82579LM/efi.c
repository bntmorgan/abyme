#include <efi.h>
#include <efilib.h>

#include "82579LM.h"
#include "debug.h"

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image, systab);
  Print(L"Experimental Intel 82579LM Ethernet driver initialization\n\r");
  if(eth_setup() == -1) {
    return EFI_NOT_FOUND;
  }
  uint16_t len = 500;
  uint8_t buf[500];
  uint8_t i;
  eth_header *eh = (eth_header *)&buf[0];
  eth_addr daddr = {.n = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
  eth_addr *laddr = eth_get_laddr();
  for (i = 0; i < 6; i++) {
    eh->src.n[i] = laddr->n[i];
    eh->dst.n[i] = daddr.n[i];
  }
  eh->type = 0x1234;
  eth_send(buf, len);
  debug_print_reg_stat();
  return EFI_SUCCESS;
}
