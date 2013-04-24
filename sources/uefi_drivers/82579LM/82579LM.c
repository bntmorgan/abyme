#include <efi.h>
#include <efilib.h>

#include "82579LM.h"
#include "pci.h"
#include "addr.h"
#include "cpu.h"
  
pci_device_info info;
pci_device_addr addr;

recv_desc rx_descs[RX_DESC_COUNT];
trans_desc tx_descs[TX_DESC_COUNT];

uint8_t rx_bufs[RX_DESC_COUNT * NET_BUF_SIZE];
uint8_t tx_bufs[RX_DESC_COUNT * NET_BUF_SIZE];

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
  // Set Link Up
  cpu_mem_writed(bar0 + REG_CTRL, cpu_mem_readd(bar0 + REG_CTRL) | CTRL_SLU);
  // Clear Multicast Table Array
  int32_t i;
  for (i = 0; i < 128; ++i) {
    cpu_mem_writed(bar0 + REG_MTA + (i * 4), 0);
  }
  // Clear all interrupts
  cpu_mem_readd(bar0 + REG_ICR);
  // Receive setup
  for (i = 0; i < RX_DESC_COUNT; i++) {
    uint8_t *buf = rx_bufs + (i * NET_BUF_SIZE);
    recv_desc *rx_desc = rx_descs + i;
    rx_desc->addr = (uint64_t)(uintptr_t)buf;
    rx_desc->status = 0;
    Print(L"rx_desc[0x%x]{addr: 0x%x, status: 0x%x}\n", i, rx_desc->addr, rx_desc->status);
  }
  cpu_mem_writed(bar0 + REG_RDBAL, (uintptr_t)rx_descs);
  cpu_mem_writed(bar0 + REG_RDBAH, (uintptr_t)rx_descs >> 32);
  cpu_mem_writed(bar0 + REG_RDLEN, RX_DESC_COUNT * 16);
  cpu_mem_writed(bar0 + REG_RDH, 0);
  cpu_mem_writed(bar0 + REG_RDT, RX_DESC_COUNT - 1);
  cpu_mem_writed(bar0 + REG_RCTL, RCTL_EN | RCTL_SBP | RCTL_UPE | RCTL_MPE
      | RCTL_LBM_NONE | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC | RCTL_BSIZE_2048);
  // Transmit setup
  for (i = 0; i < TX_DESC_COUNT; i++) {
    trans_desc *tx_desc = tx_descs + i;
    tx_desc->status = TSTA_DD;      // mark descriptor as 'complete'
    Print(L"tx_desc[0x%x]{status: 0x%x}\n", i, tx_desc->status);
  }
  cpu_mem_writed(bar0 + REG_TDBAL, (uintptr_t)tx_descs);
  cpu_mem_writed(bar0 + REG_TDBAH, (uintptr_t)tx_descs >> 32);
  cpu_mem_writed(bar0 + REG_TDLEN, TX_DESC_COUNT * 16);
  cpu_mem_writed(bar0 + REG_TDH, 0);
  cpu_mem_writed(bar0 + REG_TDT, 0);
  cpu_mem_writed(bar0 + REG_TCTL, TCTL_EN | TCTL_PSP | (15 << TCTL_CT_SHIFT)
      | (64 << TCTL_COLD_SHIFT) | TCTL_RTLC);
  return 0;
}

uint8_t eth_get_device() {
  return pci_get_device(ETH_VENDOR_ID, ETH_DEVICE_ID, &info, &addr);
}
