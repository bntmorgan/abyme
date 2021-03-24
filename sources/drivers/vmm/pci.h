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

#ifndef __PCI_H__
#define __PCI_H__

#include <efi.h>

#define PCI_MAKE_ID(bus, dev, func)     (((bus) << 16) | ((dev) << 11) | ((func) << 8))
#define PCI_MAKE_MMCONFIG(bus, dev, func)     ((((bus) << 8) | ((dev) << 3) | (func)) << 12) 

// 
// I/O Ports
//
#define PCI_CONFIG_ADDR                 0xcf8
#define PCI_CONFIG_DATA                 0xcfc

static inline uint32_t pci_make_addr(uint32_t id) {
  return 0x80000000 | id;
}
uint8_t pci_no_protect_out(uint16_t port, uint32_t value);
uint8_t pci_no_protect_in(uint16_t port);

uint8_t pci_readb(uint32_t id, uint32_t reg);
uint16_t pci_readw(uint32_t id, uint32_t reg);
uint32_t pci_readd(uint32_t id, uint32_t reg);
void pci_writeb(uint32_t id, uint32_t reg, uint8_t data);
void pci_writew(uint32_t id, uint32_t reg, uint16_t data);
void pci_writed(uint32_t id, uint32_t reg, uint32_t data);

#endif
