#include <efi.h>
#include <efilib.h>

#include "82579LM.h"
#include "debug_eth.h"
#include "api.h"

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image, systab);
  Print(L"Experimental Intel 82579LM Ethernet driver initialization\n\r");
  if(eth_setup() == -1) {
    return EFI_NOT_FOUND;
  }
  uint32_t i;
  uint16_t len = 0x100;
  uint8_t buf[0x100];
  for (i = 0; i < len ; i++) {
    buf[i] = 0xca;
  }
  uint32_t nb = 16;
  for (i = 0; i < nb; i++) {
    send(buf, len, API_BLOCK);
  }
  debug_print_reg_stat();
  return EFI_SUCCESS;
}
