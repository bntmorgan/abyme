#include <efi.h>
#include <efilib.h>

#include "types.h"
#include "pci.h"
#include "efi/efi_82579LM.h"

uint8_t protect;

extern protocol_82579LM *eth;

// XXX On considère value comme étant toujours sur 32 bits
uint8_t pci_no_protect_out(uint16_t port, uint32_t value) {
#ifdef _DEBUG_SERVER
  uint32_t pci_addr = pci_make_addr(PCI_MAKE_ID(eth->pci_addr.bus, eth->pci_addr.device, eth->pci_addr.function));
  if (port == PCI_CONFIG_ADDR) {
    if ((value & ~(0xff)) == pci_addr) {
      protect = 1;
    } else {
      protect = 0;
    }
    return 1;
  } else if (port == PCI_CONFIG_DATA) {
    return 1 - protect;
  }
#endif
  return 1;
}

uint8_t pci_no_protect_in(uint16_t port) {
#ifdef _DEBUG_SERVER
  if (port == PCI_CONFIG_ADDR) {
    return 1;
  } else if (port == PCI_CONFIG_DATA) {
    return 1 - protect;
  }
#endif
  return 1;
}

inline uint32_t pci_make_addr(uint32_t id) {
  return 0x80000000 | id;
}
