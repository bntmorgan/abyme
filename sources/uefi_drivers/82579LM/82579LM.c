#include <efi.h>
#include <efilib.h>

#include "82579LM.h"
#include "pci.h"
#include "addr.h"
#include "cpu.h"
  
pci_device_info info;
pci_device_addr addr;

uint8_t eth_setup() {
  // Get device info, bus address and function
  if (eth_get_device() == -1) {
    Print(L"LOLZ owned no ethernet controller found\n");
  } else {
    Print(L"LOLZY Intel 82579LM ethernet controller found at %02x:%02x:%02x\n", addr.bus, addr.device, addr.function);
  }
  return eth_init();
}

uint8_t eth_init() {
  uint32_t id = PCI_MAKE_ID(addr.bus, addr.device, addr.function);
  Print(L"Initializing ethernet\n");
  pci_bar bar;
  pci_get_bar(&bar, id, 0);
  if (bar.flags & PCI_BAR_IO) {
    // Only Memory Mapped I/O supported
    return -1;
  }
  uint8_t *bar0 = (uint8_t *)bar.u.address;
  // Get the mac address
  eth_addr laddr;
  uint32_t laddr_l = cpu_mem_readd(bar0 + REG_RAL);
  if (!laddr_l) {
    // We don't support EEPROM registers
    Print(L"EEPROM registers unsupported\n");
    return -1;
  }
  uint32_t laddr_h = cpu_mem_readd(bar0 + REG_RAH);
  laddr.n[0] = (uint8_t)(laddr_l >> 0);
  laddr.n[1] = (uint8_t)(laddr_l >> 8);
  laddr.n[2] = (uint8_t)(laddr_l >> 16);
  laddr.n[3] = (uint8_t)(laddr_l >> 24);
  laddr.n[4] = (uint8_t)(laddr_h >> 0);
  laddr.n[5] = (uint8_t)(laddr_h >> 8);
  Print(L"MAC addr %02x:%02x:%02x:%02x:%02x:%02x\n", laddr.n[0], 
      laddr.n[1], laddr.n[2], laddr.n[3], laddr.n[4], laddr.n[5]);
  return 0;
}

uint8_t eth_get_device() {
  return pci_get_device(ETH_VENDOR_ID, ETH_DEVICE_ID, &info, &addr);
}
