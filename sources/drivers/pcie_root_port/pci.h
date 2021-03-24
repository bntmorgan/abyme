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

#define PCI_MAKE_ID(bus, dev, func)     ((bus) << 16) | ((dev) << 11) | ((func) << 8)

// 
// I/O Ports
//
#define PCI_CONFIG_ADDR                 0xcf8
#define PCI_CONFIG_DATA                 0xcfc

//
// Header Type
//

#define PCI_TYPE_MULTIFUNC              0x80
#define PCI_TYPE_GENERIC                0x00
#define PCI_TYPE_PCI_BRIDGE             0x01
#define PCI_TYPE_CARDBUS_BRIDGE         0x02

//
// PCI Configuration Registers
//

#define PCI_CONFIG_VENDOR_ID            0x00
#define PCI_CONFIG_DEVICE_ID            0x02
#define PCI_CONFIG_COMMAND              0x04
#define PCI_CONFIG_STATUS               0x06
#define PCI_CONFIG_REVISION_ID          0x08
#define PCI_CONFIG_PROG_INTF            0x09
#define PCI_CONFIG_SUBCLASS             0x0a
#define PCI_CONFIG_CLASS_CODE           0x0b
#define PCI_CONFIG_CACHELINE_SIZE       0x0c
#define PCI_CONFIG_LATENCY_TIMER        0x0d
#define PCI_CONFIG_HEADER_TYPE          0x0e
#define PCI_CONFIG_BIST                 0x0f

//
// Type 0x00 (Generic) Configuration Registers
//

#define PCI_CONFIG_BAR0                 0x10
#define PCI_CONFIG_BAR1                 0x14
#define PCI_CONFIG_BAR2                 0x18
#define PCI_CONFIG_BAR3                 0x1c
#define PCI_CONFIG_BAR4                 0x20
#define PCI_CONFIG_BAR5                 0x24
#define PCI_CONFIG_CARDBUS_CIS_POINTER         0x28
#define PCI_CONFIG_SUBSYSTEM_VENDOR_ID         0x2c
#define PCI_CONFIG_SUBSYSTEM_ID                0x2e
#define PCI_CONFIG_EXPANSION_ROM_BASE_ADDRESS  0x30
#define PCI_CONFIG_CAPABILITY_POINTER          0x34
#define PCI_CONFIG_INTERRUPT_LINE              0x3c
#define PCI_CONFIG_INTERRUPT_PIN               0x3d
#define PCI_CONFIG_MIN_GNT                     0x3e
#define PCI_CONFIG_MAX_LATENCY                 0x3f

//
// PCI BAR
//

#define PCI_BAR_IO                      0x01
#define PCI_BAR_LOWMEM                  0x02
#define PCI_BAR_64                      0x04
#define PCI_BAR_PREFETCH                0x08

//
// High fields
//
#define PCI_CONFIG_CLIST1               0xc8
#define PCI_CONFIG_PMC                  0xca
#define PCI_CONFIG_PMCS                 0xcc
#define PCI_CONFIG_DR                   0xcf
#define PCI_CONFIG_CLIST2               0xd0
#define PCI_CONFIG_MCTL                 0xd2
#define PCI_CONFIG_MADDL                0xd4
#define PCI_CONFIG_MADDH                0xd8
#define PCI_CONFIG_MDAT                 0xdc
#define PCI_CONFIG_FLRCAP               0xe0
#define PCI_CONFIG_FLRCLV               0xe2
#define PCI_CONFIG_DEVCTRL              0xe4

typedef struct _pci_device_addr {
  uint8_t bus;
  uint8_t device;
  uint8_t function;
} pci_device_addr;

typedef struct _pci_device_info {
  uint16_t vendor_id;
  uint16_t device_id;
  uint16_t command;
  uint16_t status;
  uint8_t revision_id;
  uint8_t prog_intf;
  uint8_t sub_class;
  uint8_t class_code;
  uint8_t cacheline_size;
  uint8_t latency_timer;
  uint8_t header_type;
  uint8_t bist;
  uint32_t bar0;
  uint32_t bar1;
  uint32_t bar2;
  uint32_t bar3;
  uint32_t bar4;
  uint32_t bar5;
  uint32_t cardbus_cis_pointer;
  uint16_t subsystem_vendor_id;
  uint16_t subsystem_id;
  uint32_t expansion_rom_base_address;
  uint32_t capability_pointer:8;
  uint32_t :24;
  uint32_t :32;
  uint8_t interrupt_line;
  uint8_t interrupt_pin;
  uint8_t min_gnt;
  uint8_t max_latency;
} __attribute__((packed)) pci_device_info;

typedef struct _pci_device_info_ext {
  uint16_t clist1; // ro
  uint16_t pmc; // ro
  uint16_t pmcs; // rw
  uint8_t dr; // ro
  uint16_t clist2; // rw
  uint16_t mctl; // rw
  uint32_t maddl; // rw
  uint32_t maddh; // rw
  uint16_t mdat; // rw
  uint16_t flrcap; // ro
  uint16_t flrclv; // rw
  uint16_t devctrl; // rw
} __attribute__((packed)) pci_device_info_ext;

typedef struct _pci_bar {
  union {
    void *address;
    uint16_t port;
  } u;
  uint64_t size;
  uint32_t flags;
} pci_bar;

int pci_get_device(uint32_t vendor_id, uint32_t device_id, pci_device_addr *addr);
void pci_print_device_info_ext(pci_device_info_ext *info);
void pci_get_device_info_ext(pci_device_info_ext *info, uint32_t id);
void pci_print_device_info(pci_device_info *info);
void pci_get_device_info(pci_device_info *info, uint32_t id);
void pci_read_bar(uint32_t id, uint32_t index, uint32_t *address, uint32_t *mask);
void pci_get_bar(pci_bar *bar, uint32_t id, uint32_t index);

uint8_t pci_readb(uint32_t id, uint32_t reg);
uint16_t pci_readw(uint32_t id, uint32_t reg);
uint32_t pci_readd(uint32_t id, uint32_t reg);
void pci_writeb(uint32_t id, uint32_t reg, uint8_t data);
void pci_writew(uint32_t id, uint32_t reg, uint16_t data);
void pci_writed(uint32_t id, uint32_t reg, uint32_t data);

#endif
