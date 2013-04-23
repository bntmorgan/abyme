#include <efi.h>
#include <efilib.h>

#include "82579LM.h"

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image, systab);
  Print(L"Experimental Intel 82579LM Ethernet driver initialization\n\r");
  if(eth_setup()) {
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
}
