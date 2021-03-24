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
#include "stdio.h"

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

void pci_get_device_info(pci_device_info *info, uint32_t id) {
  info->vendor_id = pci_readw(id, PCI_CONFIG_VENDOR_ID);
  info->device_id = pci_readw(id, PCI_CONFIG_DEVICE_ID);
  info->command = pci_readw(id, PCI_CONFIG_COMMAND);
  info->status = pci_readw(id, PCI_CONFIG_STATUS);

  info->revision_id = pci_readb(id, PCI_CONFIG_REVISION_ID);
  info->prog_intf = pci_readb(id, PCI_CONFIG_PROG_INTF);
  info->sub_class = pci_readb(id, PCI_CONFIG_SUBCLASS);
  info->class_code = pci_readb(id, PCI_CONFIG_CLASS_CODE);
  info->cacheline_size = pci_readb(id, PCI_CONFIG_CACHELINE_SIZE);
  info->latency_timer = pci_readb(id, PCI_CONFIG_LATENCY_TIMER);
  info->header_type = pci_readb(id, PCI_CONFIG_HEADER_TYPE);
  info->bist = pci_readb(id, PCI_CONFIG_BIST);

  info->bar0 = pci_readd(id, PCI_CONFIG_BAR0);
  info->bar1 = pci_readd(id, PCI_CONFIG_BAR1);
  info->bar2 = pci_readd(id, PCI_CONFIG_BAR2);
  info->bar3 = pci_readd(id, PCI_CONFIG_BAR3);
  info->bar4 = pci_readd(id, PCI_CONFIG_BAR4);
  info->bar5 = pci_readd(id, PCI_CONFIG_BAR5);
  info->cardbus_cis_pointer = pci_readd(id, PCI_CONFIG_CARDBUS_CIS_POINTER);

  info->subsystem_vendor_id = pci_readw(id, PCI_CONFIG_SUBSYSTEM_VENDOR_ID);
  info->subsystem_id = pci_readw(id, PCI_CONFIG_SUBSYSTEM_ID);

  info->expansion_rom_base_address = pci_readd(id, PCI_CONFIG_EXPANSION_ROM_BASE_ADDRESS);

  info->capability_pointer = pci_readb(id, PCI_CONFIG_CAPABILITY_POINTER);
  info->interrupt_line = pci_readb(id, PCI_CONFIG_INTERRUPT_LINE);
  info->interrupt_pin = pci_readb(id, PCI_CONFIG_INTERRUPT_PIN);
  info->min_gnt = pci_readb(id, PCI_CONFIG_MIN_GNT);
  info->max_latency = pci_readb(id, PCI_CONFIG_MAX_LATENCY);
}

void pci_get_device_info_ext(pci_device_info_ext *info, uint32_t id) {
  info->clist1 = pci_readw(id, PCI_CONFIG_CLIST1);
  info->pmc = pci_readw(id, PCI_CONFIG_PMC);
  info->pmcs = pci_readw(id, PCI_CONFIG_PMCS);

  info->dr = pci_readb(id, PCI_CONFIG_DR);

  info->clist2 = pci_readw(id, PCI_CONFIG_CLIST2);
  info->mctl = pci_readw(id, PCI_CONFIG_MCTL);

  info->maddl = pci_readd(id, PCI_CONFIG_MADDL);
  info->maddh = pci_readd(id, PCI_CONFIG_MADDH);

  info->mdat = pci_readw(id, PCI_CONFIG_MDAT);
  info->flrcap = pci_readw(id, PCI_CONFIG_FLRCAP);
  info->flrclv = pci_readw(id, PCI_CONFIG_FLRCLV);
  info->devctrl = pci_readw(id, PCI_CONFIG_DEVCTRL);
}

void pci_print_device_info(pci_device_info *info) {
  Print(L"info->vendor_id %04x\n", info->vendor_id);
  Print(L"info->device_id %04x\n", info->device_id);
  Print(L"info->command %04x\n", info->command);
  Print(L"info->status %04x\n", info->status);
  
  Print(L"info->revision_id %02x\n", info->revision_id);
  Print(L"info->prog_intf %02x\n", info->prog_intf);
  Print(L"info->sub_class %02x\n", info->sub_class);
  Print(L"info->class_code %02x\n", info->class_code);
  Print(L"info->cacheline_size %02x\n", info->cacheline_size);
  Print(L"info->latency_timer %02x\n", info->latency_timer);
  Print(L"info->header_type %02x\n", info->header_type);
  Print(L"info->bist %02x\n", info->bist);

  Print(L"info->bar0 %08x\n", info->bar0);
  Print(L"info->bar1 %08x\n", info->bar1);
  Print(L"info->bar2 %08x\n", info->bar2);
  Print(L"info->bar3 %08x\n", info->bar3);
  Print(L"info->bar4 %08x\n", info->bar4);
  Print(L"info->bar5 %08x\n", info->bar5);

  Print(L"info->cardbus_cis_pointer %08x\n", info->cardbus_cis_pointer);

  Print(L"info->subsystem_vendor_id %04x\n", info->subsystem_vendor_id);
  Print(L"info->subsystem_id %04x\n", info->subsystem_id);

  Print(L"info->expansion_rom_base_address %08x\n", info->expansion_rom_base_address);

  Print(L"info->capability_pointer %02x\n", info->capability_pointer);
  Print(L"info->interrupt_line %02x\n", info->interrupt_line);
  Print(L"info->interrupt_pin %02x\n", info->interrupt_pin);
  Print(L"info->min_gnt %02x\n", info->min_gnt);
  Print(L"info->max_latency %02x\n", info->max_latency);
}

void pci_print_device_info_ext(pci_device_info_ext *info) {
  Print(L"info->clist1 %04x\n", info->clist1); // pci_readw
  Print(L"info->pmc %04x\n", info->pmc); // pci_readw
  Print(L"info->pmcs %04x\n", info->pmcs); // pci_readw

  Print(L"info->dr %02x\n", info->dr); // pci_readb

  Print(L"info->clist2 %04x\n", info->clist2); // pci_readw
  Print(L"info->mctl %04x\n", info->mctl); // pci_readw

  Print(L"info->maddl %08x\n", info->maddl); // pci_readd
  Print(L"info->maddh %08x\n", info->maddh); // pci_readd

  Print(L"info->mdat %04x\n", info->mdat); // pci_readw
  Print(L"info->flrcap %04x\n", info->flrcap); // pci_readw
  Print(L"info->flrclv %04x\n", info->flrclv); // pci_readw
  Print(L"info->devctrl %04x\n", info->devctrl); // pci_readw
}

uint8_t pci_get_device(uint32_t vendor_id, uint32_t device_id, pci_device_addr *addr) {
  uint8_t ok = 0;
  uint16_t bus;
  uint8_t dev;
  for (bus = 0; bus < 256; bus++) {
    for (dev = 0; dev < 32 && ok == 0; dev++) {
      uint32_t base_id = PCI_MAKE_ID(bus, dev, 0);
      uint8_t header_type = pci_readb(base_id, PCI_CONFIG_HEADER_TYPE);
      uint32_t func_count = header_type & PCI_TYPE_MULTIFUNC ? 8 : 1;
      uint32_t func;
      for (func = 0; func < func_count; func++) {
        uint32_t id = PCI_MAKE_ID(bus, dev, func);
        uint32_t vid = pci_readw(id, PCI_CONFIG_VENDOR_ID);
        uint32_t did = pci_readw(id, PCI_CONFIG_DEVICE_ID);
        if (vid != 0xffff) {
          INFO("[%04x:%04x] %02x:%02x:%02x\n", vid, did, bus, dev, func);
        }
        if (vid == 0xffff || vid != vendor_id || did != device_id) {
          continue;
        } else {
          addr->bus = bus;
          addr->device = dev;
          addr->function = func;
          ok = 1;
          break;
        }
      }
    }
  }
  if (ok) {
    return 0;
  } else {
    return -1;
  }
}

void pci_read_bar(uint32_t id, uint32_t index, uint32_t *address, uint32_t *mask) {
  uint32_t reg = PCI_CONFIG_BAR0 + index * sizeof(uint32_t);
  // Get address
  *address = pci_readd(id, reg);
  // Find out size of the bar
  pci_writed(id, reg, 0xffffffff);
  *mask = pci_readd(id, reg);
  // Restore adddress
  pci_writed(id, reg, *address);
}

void pci_get_bar(pci_bar *bar, uint32_t id, uint32_t index) {
  // Read pci bar register
  uint32_t addressLow;
  uint32_t maskLow;
  pci_read_bar(id, index, &addressLow, &maskLow);

  if (addressLow & PCI_BAR_64)
  {
    // 64-bit mmio
    uint32_t addressHigh;
    uint32_t maskHigh;
    pci_read_bar(id, index + 1, &addressHigh, &maskHigh);

    bar->u.address = (void *)(((uintptr_t)addressHigh << 32) | (addressLow & ~0xf));
    bar->size = ~(((uint64_t)maskHigh << 32) | (maskLow & ~0xf)) + 1;
    bar->flags = addressLow & 0xf;
  }
  else if (addressLow & PCI_BAR_IO)
  {
    // i/o register
    bar->u.port = (uint16_t)(addressLow & ~0x3);
    bar->size = (uint16_t)(~(maskLow & ~0x3) + 1);
    bar->flags = addressLow & 0x3;
  }
  else
  {
    // 32-bit mmio
    bar->u.address = (void *)(uintptr_t)(addressLow & ~0xf);
    bar->size = ~(maskLow & ~0xf) + 1;
    bar->flags = addressLow & 0xf;
  }
}

uint64_t pci_mmio_get_base(void) {
  uint8_t MMCONFIG_length;
  uint16_t MMCONFIG_mask;

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
  uint16_t MMCONFIG = pci_readw(0,0x63) & MMCONFIG_mask;

  return ((uint64_t)MMCONFIG << 16);
}
