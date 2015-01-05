#include <efi.h>
#include <efilib.h>

#include "pci.h"
#include "efi/efi_eric.h"
#include "debug.h"
#include "stdio.h"

// 
// stdio putc pointer
//
extern void (*putc)(uint8_t);

void put_nothing(uint8_t c) {}

void eric_disable_debug(void){
  putc = &put_nothing;
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image_handle, systab);

  EFI_STATUS status = EFI_SUCCESS;
  uint32_t id;
  pci_device_addr addr = {0x00, 0x1c, 0x04};

  INFO("Experimental Intel 82579LM Ethernet driver initialization\n");

  id = PCI_MAKE_ID(addr.bus, addr.device, addr.function);

  // Powerup PIO & MMI_extO
  pci_writew(id, PCI_CONFIG_COMMAND, 0x7);
  pci_writeb(id, PCI_CONFIG_INTERRUPT_LINE, 0x5);
  
  INFO("STATUS 0x%04x\n", pci_readw(id, PCI_CONFIG_STATUS));
  INFO("COMMAND 0x%04x\n", pci_readw(id, PCI_CONFIG_COMMAND));

  return status;
}
