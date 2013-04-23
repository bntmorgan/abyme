#include <efi.h>
#include <efilib.h>
#include "82579LM.h"
#include "pci.h"
  
pci_device_info info;
pci_device_addr addr;

uint8_t eth_setup() {
  if (!eth_get_device()) {
    Print(L"LOLZ owned no ethernet controller found\n");
    return 0;
  } else {
    Print(L"LOLZY Intel 82579LM ethernet controller found at \n %x:%x:%x\n", addr.bus, addr.device, addr.function);
    return 1;
  }
}

uint8_t eth_get_device() {
  return pci_get_device(ETH_VENDOR_ID, ETH_DEVICE_ID, &info, &addr);
}
