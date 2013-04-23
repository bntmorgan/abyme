#include <efi.h>
#include <efilib.h>

#include "types.h"
#include "pci.h"
#include "cpu.h"

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

uint8_t pci_get_device(uint32_t vendor_id, uint32_t device_id, pci_device_info *info, pci_device_addr *addr) {
  uint8_t ok = 0;
  uint16_t bus;
  uint8_t dev;
  for (bus = 0; bus < 256; bus++) {
    for (dev = 0; dev < 32; dev++) {
      uint32_t base_id = PCI_MAKE_ID(bus, dev, 0);
      uint8_t header_type = pci_readb(base_id, PCI_CONFIG_HEADER_TYPE);
      uint32_t func_count = header_type & PCI_TYPE_MULTIFUNC ? 8 : 1;
      uint32_t func;
      for (func = 0; func < func_count; func++) {
        uint32_t id = PCI_MAKE_ID(bus, dev, func);
        uint32_t vid = pci_readw(id, PCI_CONFIG_VENDOR_ID);
        uint32_t did = pci_readw(id, PCI_CONFIG_DEVICE_ID);
        if (vid != 0xffff) {
          Print(L"[%04x:%04x] %02x:%02x:%02x\n", vid, did, bus, dev, func);
        }
        if (vid == 0xffff || vid != vendor_id || did != device_id) {
          continue;
        } else {
          info->vendor_id = vid;
          info->device_id = did;
          info->prog_intf = pci_readb(id, PCI_CONFIG_PROG_INTF);
          info->sub_class = pci_readb(id, PCI_CONFIG_SUBCLASS);
          info->class_code = pci_readb(id, PCI_CONFIG_CLASS_CODE);
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
