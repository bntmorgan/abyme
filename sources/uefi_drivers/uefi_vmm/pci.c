#include <efi.h>
#include <efilib.h>

#include "types.h"
#include "pci.h"

inline uint32_t pci_make_addr(uint32_t id) {
  return 0x80000000 | id;
}

/*uint8_t pci_readb(uint32_t id, uint32_t reg) {
  uint32_t addr = 0x80000000 | id | (reg & 0xfc);
  cpu_outportd(PCI_CONFIG_ADDR, addr);
  return cpu_inportb(PCI_CONFIG_DATA + (reg & 0x03));
}

void pci_writeb(uint32_t id, uint32_t reg, uint8_t data) {
  uint32_t addr = 0x80000000 | id | (reg & 0xfc);
  cpu_outportd(PCI_CONFIG_ADDR, addr);
  cpu_outportb(PCI_CONFIG_DATA + (reg & 0x03), data);
}*/
