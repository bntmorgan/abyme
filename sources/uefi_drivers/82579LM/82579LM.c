#include <efi.h>
#include <efilib.h>

#include "82579LM.h"
#include "addr.h"
#include "cpu.h"
#include "string.h"
  
// PCI device information
pci_device_info info;
pci_device_addr addr;

// MAC address
eth_addr laddr;

// Transmission and receive descriptors
recv_desc rx_descs[RX_DESC_COUNT] __attribute__((aligned(0x10)));
trans_desc tx_descs[TX_DESC_COUNT] __attribute__((aligned(0x10)));

// BAR0
uint8_t *bar0;

uint8_t rx_bufs[RX_DESC_COUNT * NET_BUF_SIZE];
uint8_t tx_bufs[TX_DESC_COUNT * NET_BUF_SIZE];

void wait(uint64_t time) {
  uint64_t i;
  for (i = 0; i < time; i++) {
  }
}

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
  bar0 = (uint8_t *)bar.u.address;
  // Get the mac address
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
  Print(L"CTRL %x\n", cpu_mem_readd(bar0 + REG_CTRL));
  Print(L"CTRL %x\n", cpu_mem_readd(bar0 + REG_STATUS));
  // cpu_mem_writed(bar0 + REG_CTRL, cpu_mem_readd(bar0 + REG_CTRL) | CTRL_SLU);
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
    //Print(L"@0x%x rx_desc[0x%x]{addr: 0x%x, status: 0x%x}\n", rx_desc, i, rx_desc->addr, rx_desc->status);
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
    //Print(L"@0x%x tx_desc[0x%x]{addr: 0x%x, status: 0x%x}\n", tx_desc, i, tx_desc->addr, tx_desc->status);
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

inline void eth_wait_tx(uint8_t idx) {
  trans_desc *tx_desc = tx_descs + idx;
  // Wait until the packet is send
  while (!(tx_desc->status & 0xf)) {
    wait(1000000);
  }
}

void eth_send(const void *buf, uint16_t len, uint8_t block) {
  // Current transmition descriptor index
  static uint8_t idx = 0;
  // Wait for the precedent descriptor being ready
  eth_wait_tx(idx);
  // Copy the buf into the tx_buf
  trans_desc *tx_desc = tx_descs + idx;
  uint8_t *b = tx_bufs + (idx * NET_BUF_SIZE);
  memcpy(b, (void *)buf, len);
  // Write new tx descriptor
  tx_desc->addr = (uint64_t)(uintptr_t)b;
  tx_desc->len = len;
  tx_desc->cmd = CMD_EOP | CMD_IFCS | CMD_RS;
  tx_desc->status = 0;
  // Increment the current tx decriptor
  idx = (idx + 1) & (TX_DESC_COUNT - 1);
  cpu_mem_writed(bar0 + REG_TDT, idx);
  if (block) {
    eth_wait_tx(idx);
  }
}

inline void eth_wait_rx(uint8_t idx) {
  recv_desc *rx_desc = rx_descs + idx;
  // Wait until the packet is send
  while (!(rx_desc->status & RSTA_DD)) {
    wait(1000000);
  }
}

uint32_t eth_recv(const void *buf, uint32_t len, uint8_t block) {
  // Current receive descriptor index
  static uint8_t idx = 0;
  // Wait for a packet
  if (block) {
    eth_wait_rx(idx);
  }
  // Copy the buf into the rx_buf
  recv_desc *rx_desc = rx_descs + idx;
  uint32_t l = 0;
  while ((rx_desc->status & RSTA_DD) && (l < len)) {
    if (rx_desc->errors) {
      Print(L"Packet Error: (0x%x)\n", rx_desc->errors);
    } else {
      uint8_t *b = rx_bufs + (idx * NET_BUF_SIZE);
      uint32_t len = rx_desc->len;
      memcpy((void *)buf, b, len);
      // desc->addr = (u64)(uintptr_t)buf->start;
      Print(L"Received %x%x%x%x\n", *((uint32_t *)buf + 0), *((uint32_t *)buf + 1),
          *((uint32_t *)buf + 2), *((uint32_t *)buf + 3));
      buf = (uint8_t *)buf + len;
      l += len;
    }
    rx_desc->status = 0;
    cpu_mem_writed(bar0 + REG_RDT, idx);
    idx = (idx + 1) & (RX_DESC_COUNT - 1);
  }
  return l; 
}

uint8_t eth_get_device() {
  return pci_get_device(ETH_VENDOR_ID, ETH_DEVICE_ID, &info, &addr);
}
