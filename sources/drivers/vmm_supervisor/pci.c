/*
Copyright (C) 2021  Benoît Morgan

This file is part of abyme

abyme is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

abyme is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with abyme.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <efi.h>
#include <efilib.h>

#include "types.h"
#include "pci.h"
#include "cpu.h"
#include "efi/efi_82579LM.h"

uint8_t protect;

extern protocol_82579LM *eth;

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
