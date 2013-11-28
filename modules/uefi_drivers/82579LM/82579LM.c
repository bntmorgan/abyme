#include <efi.h>
#include <efilib.h>

#include "82579LM.h"
#include "addr.h"
#include "cpu.h"
#include "efiw.h"
#include "string.h"
#include "debug_eth.h"
#include "stdio.h"
#include "cpuid.h"
#include "pat.h"
#include "mtrr.h"
#include "paging.h"
  
// PCI device information
pci_device_info info;
pci_device_addr addr;

// MAC address
eth_addr laddr;

// Transmission and receive descriptors
// recv_desc rx_descs[RX_DESC_COUNT] __attribute__((aligned(0x10)));
// trans_desc tx_descs[TX_DESC_COUNT] __attribute__((aligned(0x10)));
recv_desc *rx_descs;
trans_desc *tx_descs;

// BAR0
uint8_t *bar0;

// uint8_t rx_bufs[RX_DESC_COUNT * NET_BUF_SIZE];
// uint8_t tx_bufs[TX_DESC_COUNT * NET_BUF_SIZE];
uint8_t *rx_bufs;
uint8_t *tx_bufs;

void wait(uint64_t time) {
  uint64_t i;
  for (i = 0; i < time; i++) {
  }
}

uint8_t eth_set_bar0() {
  uint32_t id = PCI_MAKE_ID(addr.bus, addr.device, addr.function);
  pci_bar bar;
  pci_get_bar(&bar, id, 0);
  if (bar.flags & PCI_BAR_IO) {
    // Only Memory Mapped I/O supported
    INFO("Only Memory Mapped I/O supported\n");
    return -1;
  }
  bar0 = (uint8_t *)bar.u.address;
  return 0;
}

uint8_t eth_disable_interrupts() {
  uint16_t reg_imc = cpu_mem_readd(bar0 + REG_IMC);
  // Reset the card
  reg_imc |=  IMC_RXT0 | IMC_MDAC | IMC_PHYINT | IMC_LSECPN | IMC_SRPD | IMC_ACK |
    IMC_MNG;
  cpu_mem_writed(bar0 + REG_IMC, reg_imc);

  return 0;
}

uint8_t eth_reset() {
  INFO("We reset the ethernet controller to ensure having a clear configuration\n");
  uint16_t reg_ctrl = cpu_mem_readd(bar0 + REG_CTRL);
  // Reset the card
  reg_ctrl |= CTRL_SWRST | CTRL_LCD_RST;
  cpu_mem_writed(bar0 + REG_CTRL, reg_ctrl);
  wait(10000000);
  return 0;
}

void eth_print_registers() {
  INFO("CTRL 0x%08x\n", cpu_mem_readd(bar0 + REG_CTRL));
  INFO("STATUS 0x%08x\n", cpu_mem_readd(bar0 + REG_STATUS));
  INFO("Interrupt Control Register 0x%08x\n", cpu_mem_readd(bar0 + REG_ICR));
  INFO("Receive Control Register 0x%08x\n", cpu_mem_readd(bar0 + REG_RCTL));
  INFO("Transmit Control Register 0x%08x\n", cpu_mem_readd(bar0 + REG_TCTL));
}

// See 11.0 Initialization and Reset Operation
uint8_t eth_setup() {
  INFO("CPUID SETUP\n");
  cpuid_setup();
  mtrr_create_ranges();
  INFO("MTRR CREATE RANGES DONE\n");
  mtrr_print_ranges();
  INFO("MTRR PRINT RANGES DONE\n");
  INFO("PAT SETUP\n");
  pat_setup();
  // Get device info, bus address and function
  if (eth_get_device() == -1) {
    INFO("LOLZ owned no ethernet controller found\n");
    return -1;
  } else {
    INFO("LOLZY Intel 82579LM ethernet controller found at %02x:%02x:%02x\n", addr.bus, addr.device, addr.function);
  }
  if(eth_set_bar0()) {
    return -1;
  }
  // Allocate memory for rx tx buffers and descriptors
  rx_bufs = efi_allocate_pages((RX_DESC_COUNT * NET_BUF_SIZE) / 0x1000 + ((RX_DESC_COUNT * NET_BUF_SIZE) % 0x1000 != 0.0));
  if (!rx_bufs) {
    INFO("Failed to allocate rx_bufs\n");
    return -1;
  }
  tx_bufs = efi_allocate_pages((TX_DESC_COUNT * NET_BUF_SIZE) / 0x1000 + ((RX_DESC_COUNT * NET_BUF_SIZE) % 0x1000 != 0.0));
  if (!rx_bufs) {
    INFO("Failed to allocate tx_bufs\n");
    return -1;
  }
  // descs check alignement to 0x10
  // XXX
  rx_descs = efi_allocate_pages(1);
  if (!rx_descs || (((uint64_t)rx_descs) & 0xf)) {
    INFO("Failed to allocate rx_descs or unaligned to 0x10 : 0x%016X lol %d\n", (uint64_t)rx_descs);
    return -1;
  }
  tx_descs = efi_allocate_pages(1);
  // XXX
  if (!tx_descs || (((uint64_t)tx_descs) & 0xf)) {
    INFO("Failed to allocate tx_bufs or unaligned to 0x10 : 0x%016X\n", (uint64_t)tx_descs);
    return -1;
  }
  return 0; 
}

uint8_t eth_init() {
  // Change the cache policy for the buffers and descriptors with PAT
  INFO("Install the right memory type for rx_bufs...\n");
  if (pat_set_memory_type_range((uint64_t)rx_bufs, MEMORY_TYPE_UC, RX_DESC_COUNT
        * NET_BUF_SIZE)) {
    INFO("Failed to install the right memory type for rx_bufs...\n");
    return -1;
  }
  INFO("Install the right memory type for tx_bufs...\n");
  if (pat_set_memory_type_range((uint64_t)tx_bufs, MEMORY_TYPE_UC, TX_DESC_COUNT
        * NET_BUF_SIZE)) {
    INFO("Failed to install the right memory type for tx_bufs...\n");
    return -1;
  }
  // XXX
  INFO("Install the right memory type for rx_bufs...\n");
  if (pat_set_memory_type_range((uint64_t)rx_descs, MEMORY_TYPE_UC, 0x1000)) {
    INFO("Failed to install the right memory type for rx_bufs...\n");
    return -1;
  }
  // XXX
  INFO("Install the right memory type for tx_bufs...\n");
  if (pat_set_memory_type_range((uint64_t)tx_descs, MEMORY_TYPE_UC, 0x1000)) {
    INFO("Failed to install the right memory type for tx_bufs...\n");
    return -1;
  }
  INFO("Experimental Intel 82579LM Ethernet driver initialization\n\r");
  eth_disable_interrupts();
  eth_reset();
  eth_disable_interrupts();
  eth_print_registers();
  INFO("Initializing ethernet\n");
  // Get the mac address
  uint32_t laddr_l = cpu_mem_readd(bar0 + REG_RAL);
  if (!laddr_l) {
    // We don't support EEPROM registers
    INFO("EEPROM registers unsupported\n");
    return -1;
  }
  uint32_t laddr_h = cpu_mem_readd(bar0 + REG_RAH);
  laddr.n[0] = (uint8_t)(laddr_l >> 0);
  laddr.n[1] = (uint8_t)(laddr_l >> 8);
  laddr.n[2] = (uint8_t)(laddr_l >> 16);
  laddr.n[3] = (uint8_t)(laddr_l >> 24);
  laddr.n[4] = (uint8_t)(laddr_h >> 0);
  laddr.n[5] = (uint8_t)(laddr_h >> 8);
  INFO("MAC addr %02x:%02x:%02x:%02x:%02x:%02x\n", laddr.n[0], 
      laddr.n[1], laddr.n[2], laddr.n[3], laddr.n[4], laddr.n[5]);
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
    //INFO("@0x%x rx_desc[0x%x]{addr: 0x%x, status: 0x%x}\n", rx_desc, i, rx_desc->addr, rx_desc->status);
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
    //INFO("@0x%x tx_desc[0x%x]{addr: 0x%x, status: 0x%x}\n", tx_desc, i, tx_desc->addr, tx_desc->status);
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
    INFO("WAIIT lol\n");
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

uint32_t eth_recv(void *buf, uint32_t len, uint8_t block) {
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
      INFO("Packet Error: (0x%x)\n", rx_desc->errors);
    } else {
      uint8_t *b = rx_bufs + (idx * NET_BUF_SIZE);
      uint32_t len = rx_desc->len;
      memcpy((void *)buf, b, len);
      // desc->addr = (u64)(uintptr_t)buf->start;
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
