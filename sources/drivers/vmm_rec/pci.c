#include <efi.h>
#include <efilib.h>

#include "stdio.h"
#include "types.h"
#include "pci.h"
#include "cpu.h"
#include "efi/efi_82579LM.h"
#ifdef _DEBUG_SERVER
#include "debug_server/debug_server.h"
#endif

uint8_t protect;

extern protocol_82579LM *eth;

uint64_t mmconfig;

void pci_init(void) {
  mmconfig = pci_mmconfig_base();
}

uint8_t pci_readb(uint32_t id, uint32_t reg) {
  uint32_t addr = 0x80000000 | id | (reg & 0xfc);
  cpu_outportd(PCI_CONFIG_ADDR, addr);
  return cpu_inportb(PCI_CONFIG_DATA + (reg & 0x03));
}

uint16_t pci_readw(uint32_t id, uint32_t reg) {
  uint32_t addr = 0x80000000 | id | (reg & 0xfc);
  cpu_outportd(PCI_CONFIG_ADDR, addr);
  return cpu_inportw(PCI_CONFIG_DATA + (reg & 0x02));
}

uint32_t pci_readd(uint32_t id, uint32_t reg) {
  uint32_t addr = 0x80000000 | id | (reg & 0xfc);
  cpu_outportd(PCI_CONFIG_ADDR, addr);
  return cpu_inportd(PCI_CONFIG_DATA);
}

void pci_writeb(uint32_t id, uint32_t reg, uint8_t data) {
    uint32_t addr = 0x80000000 | id | (reg & 0xfc);
    cpu_outportd(PCI_CONFIG_ADDR, addr);
    cpu_outportb(PCI_CONFIG_DATA + (reg & 0x03), data);
}

void pci_writew(uint32_t id, uint32_t reg, uint16_t data) {
    uint32_t addr = 0x80000000 | id | (reg & 0xfc);
    cpu_outportd(PCI_CONFIG_ADDR, addr);
    cpu_outportw(PCI_CONFIG_DATA + (reg & 0x02), data);
}

void pci_writed(uint32_t id, uint32_t reg, uint32_t data) {
    uint32_t addr = 0x80000000 | id | (reg & 0xfc);
    cpu_outportd(PCI_CONFIG_ADDR, addr);
    cpu_outportd(PCI_CONFIG_DATA, data);
}

// XXX On considère value comme étant toujours sur 32 bits
uint8_t pci_no_protect_out(uint16_t port, uint32_t value) {
#ifdef _DEBUG_SERVER
  if (debug_server) {
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
  }
#endif
  return 1;
}

uint8_t pci_no_protect_in(uint16_t port) {
#ifdef _DEBUG_SERVER
  if (debug_server) {
    if (port == PCI_CONFIG_ADDR) {
      return 1;
    } else if (port == PCI_CONFIG_DATA) {
      return 1 - protect;
    }
  }
#endif
  return 1;
}

uint64_t pci_mmconfig_base(void) {
  uint8_t MMCONFIG_length;
  uint16_t MMCONFIG_mask;
  uint64_t MMCONFIG;

  /* MMCONFIG_length : bit 1:2 of PCIEXBAR (offset 60) */
  MMCONFIG_length = (pci_readb(0,0x60) & 0x6) >> 1;

  /* MMCONFIG corresponds to bits 38 to 28 of the pci base address
     MMCONFIG_length decrease to 27 or 26 the lsb of MMCONFIG */
  switch (MMCONFIG_length) {
    case 0:
      MMCONFIG_mask = 0xF07F;
      break;
    case 1:
      MMCONFIG_mask = 0xF87F;
      break;
    case 2:
      MMCONFIG_mask = 0xFC7F;
      break;
    default:
      panic("Bad MMCONFIG Length\n");
  }

  /* MMCONFIG : bit 38:28-26 of PCIEXBAR (offset 60) -> 14:4-2 of PCIEXBAR + 3 */
  MMCONFIG = pci_readw(0,0x63) & MMCONFIG_mask;
  return ((uint64_t)MMCONFIG << 16);
}

uint16_t pci_mm_readw(uint32_t id, uint32_t reg) {
  return *(uint16_t*)(mmconfig + id + reg);
}

uint32_t pci_mm_readd(uint32_t id, uint32_t reg) {
  return *(uint32_t*)(mmconfig + id + reg);
}

uint64_t pci_mm_readq(uint32_t id, uint32_t reg) {
  return *(uint64_t*)(mmconfig + id + reg);
}

uint16_t pci_bar_readw(uint64_t bar, uint32_t reg) {
  return *(uint16_t*)(bar + reg);
}

uint32_t pci_bar_readd(uint64_t bar, uint32_t reg) {
  return *(uint32_t*)(bar + reg);
}

uint64_t pci_bar_readq(uint64_t bar, uint32_t reg) {
  return *(uint64_t*)(bar + reg);
}
