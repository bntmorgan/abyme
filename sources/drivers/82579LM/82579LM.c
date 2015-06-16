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
pci_device_info_ext info_ext;
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

// stdio putc pointer
extern void (*putc)(uint8_t);

void put_nothing(uint8_t c) {}

void eth_disable_debug(void){
  putc = &put_nothing;
}

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
#if 0
  INFO("We reset the ethernet controller to ensure having a clear configuration\n");
  uint16_t reg_ctrl = cpu_mem_readd(bar0 + REG_CTRL);
  // Reset the card
  reg_ctrl |= CTRL_SWRST | CTRL_LCD_RST;
  cpu_mem_writed(bar0 + REG_CTRL, reg_ctrl);
  wait(10000000);
  return 0;
#endif
  uint32_t reg_ctrl;
  // uint32_t reg_ctrl_ext;
  // uint32_t reg_status;
  // uint32_t extcnf_ctrl;
  INFO("We reset the ethernet controller to ensure having a clear configuration\n");
  // Remove the lan init done
  /*reg_status = cpu_mem_readd(bar0 + REG_STATUS);
  cpu_mem_writed(bar0 + REG_STATUS, reg_status & ~(STATUS_LAN_INIT_DONE));
  reg_ctrl = cpu_mem_readd(bar0 + REG_CTRL);
  cpu_mem_writed(bar0 + REG_CTRL, reg_ctrl);*/
  // Reset the card
  reg_ctrl = cpu_mem_readd(bar0 + REG_CTRL);
  cpu_mem_writed(bar0 + REG_CTRL, reg_ctrl | CTRL_SWRST);
  // Get control of the hardware
  /*reg_ctrl_ext = cpu_mem_readd(bar0 + REG_CTRL_EXT);
  cpu_mem_writed(bar0 + REG_CTRL_EXT, reg_ctrl_ext | CTRL_EXT_DRV_LOAD);*/
  wait(10000000);
  return 0;
}

// See 11.0 Initialization and Reset Operation
int eth_setup() {
  // Get device info, bus address and function
  if (eth_get_device()) {
    INFO("LOLZ owned no ethernet controller found\n");
    return -1;
  }
  // Get PCI info and info_ext
  INFO("LOLZY Intel 82579LM ethernet controller found at %02x:%02x:%02x\n",
      addr.bus, addr.device, addr.function);
  uint32_t id = PCI_MAKE_ID(addr.bus, addr.device, addr.function);
  // Powerup PIO & MMI_extO
  pci_writew(id, PCI_CONFIG_COMMAND, 0x7);
  pci_writeb(id, PCI_CONFIG_INTERRUPT_LINE, 0x5);
  pci_get_device_info(&info, id);
  pci_get_device_info_ext(&info_ext, id);
  pci_print_device_info(&info);
  pci_print_device_info_ext(&info_ext);
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
  if (!rx_descs || (((uintptr_t)rx_descs) & 0xf)) {
    INFO("Failed to allocate rx_descs or unaligned to 0x10 : 0x%016X lol %d\n", (uintptr_t)rx_descs);
    return -1;
  }
  tx_descs = efi_allocate_pages(1);
  // XXX
  if (!tx_descs || (((uintptr_t)tx_descs) & 0xf)) {
    INFO("Failed to allocate tx_bufs or unaligned to 0x10 : 0x%016X\n", (uintptr_t)tx_descs);
    return -1;
  }
  return 0;
}

uint8_t eth_set_memory_type() {
  /*INFO("Install the right memory type for rx_bufs...\n");
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
  }*/
  return 0;
}

int eth_init() {
  // uint32_t extcnf_ctrl;
  /*if (eth_set_memory_type()) {
    return -1;
  }*/
  // Change the cache policy for the buffers and descriptors with PAT
  INFO("Experimental Intel 82579LM Ethernet driver initialization\n\r");
  eth_disable_interrupts();
  eth_reset();
  eth_disable_interrupts();
  // Gain acces to the shared registers
  /*extcnf_ctrl = cpu_mem_readd(bar0 + REG_EXTCNF_CTRL);
  extcnf_ctrl &= ~EXTCNF_CNTRL_GATEPHYCONF;
  cpu_mem_writed(bar0 + REG_EXTCNF_CTRL, extcnf_ctrl | EXTCNF_CTRL_SWFLAG);*/
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
  cpu_mem_writed(bar0 + REG_TDBAH, 0);
  cpu_mem_writed(bar0 + REG_TDLEN, TX_DESC_COUNT * 16);
  cpu_mem_writed(bar0 + REG_TDH, 0);
  cpu_mem_writed(bar0 + REG_TDT, 0);
  cpu_mem_writed(bar0 + REG_TCTL, TCTL_EN | TCTL_PSP | (15 << TCTL_CT_SHIFT)
      | (64 << TCTL_COLD_SHIFT) | TCTL_RTLC);
  // Finally dump the registers
  /*eth_print_registers_general();
  eth_print_registers_interrupt();
  eth_print_registers_receive();
  eth_print_registers_transmit();*/
  // eth_print_registers_statistic();
  return 0;
}

static inline void eth_wait_tx(uint8_t idx) {
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

static inline void eth_wait_rx(uint8_t idx) {
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

int eth_get_device() {
  return pci_get_device(ETH_VENDOR_ID, ETH_DEVICE_ID, &addr);
}

void eth_print_registers() {
  INFO("CTRL 0x%08x\n", cpu_mem_readd(bar0 + REG_CTRL));
  INFO("STATUS 0x%08x\n", cpu_mem_readd(bar0 + REG_STATUS));
  INFO("STRAP 0x%08x\n", cpu_mem_readd(bar0 + REG_STRAP));
  INFO("EERD 0x%08x\n", cpu_mem_readd(bar0 + REG_EERD));
  INFO("CTRL_EXT 0x%08x\n", cpu_mem_readd(bar0 + REG_CTRL_EXT));
  INFO("MDIC 0x%08x\n", cpu_mem_readd(bar0 + REG_MDIC));

  INFO("Interrupt Control Register 0x%08x\n", cpu_mem_readd(bar0 + REG_ICR));
  INFO("Receive Control Register 0x%08x\n", cpu_mem_readd(bar0 + REG_RCTL));
  INFO("Transmit Control Register 0x%08x\n", cpu_mem_readd(bar0 + REG_TCTL));
  INFO("Extended Configuration Control register 0x%08x\n", cpu_mem_readd(bar0 + REG_EXTCNF_CTRL));
}

// s/#define \([^ ]*\).*\(0x[^ ]*\).*\(\/\/ .*\)/\1 \2 \3/gc
// s/#define \([^ ]*\).*\(0x[^ ]*\).*\(\/\/ .*\)/  INFO("\1 : 0x%08x\\n", cpu_mem_readd(bar0 \+ \2));/gc
// s/#define \([^ ]*\).*\(0x[^ ]*\).*\(\/\/ .*\)/  INFO("\1 : 0x%08x\\n", cpu_mem_readd(bar0 + \1));/gc
void eth_print_registers_general() {
  INFO("--== general registers ==--\n");
  INFO("REG_CTRL : 0x%08x\n", cpu_mem_readd(bar0 + REG_CTRL));
  INFO("REG_STATUS : 0x%08x\n", cpu_mem_readd(bar0 + REG_STATUS));
  INFO("REG_STRAP : 0x%08x\n", cpu_mem_readd(bar0 + REG_STRAP));
  INFO("REG_EERD : 0x%08x\n", cpu_mem_readd(bar0 + REG_EERD));
  INFO("REG_CTRL_EXT : 0x%08x\n", cpu_mem_readd(bar0 + REG_CTRL_EXT));
  INFO("REG_MDIC : 0x%08x\n", cpu_mem_readd(bar0 + REG_MDIC));
  INFO("REG_FEXTNVM4 : 0x%08x\n", cpu_mem_readd(bar0 + REG_FEXTNVM4));
  INFO("REG_FEXTNVM : 0x%08x\n", cpu_mem_readd(bar0 + REG_FEXTNVM));
  INFO("REG_FEXT : 0x%08x\n", cpu_mem_readd(bar0 + REG_FEXT));
  INFO("REG_FEXTNVM2 : 0x%08x\n", cpu_mem_readd(bar0 + REG_FEXTNVM2));
  INFO("REG_KUMCTRLSTA : 0x%08x\n", cpu_mem_readd(bar0 + REG_KUMCTRLSTA));
  INFO("REG_BUSNUM : 0x%08x\n", cpu_mem_readd(bar0 + REG_BUSNUM));
  INFO("REG_LTRV : 0x%08x\n", cpu_mem_readd(bar0 + REG_LTRV));
  INFO("REG_LPIC : 0x%08x\n", cpu_mem_readd(bar0 + REG_LPIC));
  INFO("REG_FCTTV : 0x%08x\n", cpu_mem_readd(bar0 + REG_FCTTV));
  INFO("REG_FCRTV : 0x%08x\n", cpu_mem_readd(bar0 + REG_FCRTV));
  INFO("REG_EXTCNF_CTRL : 0x%08x\n", cpu_mem_readd(bar0 + REG_EXTCNF_CTRL));
  INFO("REG_EXTCNF_SIZE : 0x%08x\n", cpu_mem_readd(bar0 + REG_EXTCNF_SIZE));
  INFO("REG_PHY_CTRL : 0x%08x\n", cpu_mem_readd(bar0 + REG_PHY_CTRL));
  INFO("REG_PCIEANACFG : 0x%08x\n", cpu_mem_readd(bar0 + REG_PCIEANACFG));
  INFO("REG_PBA : 0x%08x\n", cpu_mem_readd(bar0 + REG_PBA));
  INFO("REG_PBS : 0x%08x\n", cpu_mem_readd(bar0 + REG_PBS));
  INFO("REG_DCR : 0x%08x\n", cpu_mem_readd(bar0 + REG_DCR));
}

void eth_print_registers_interrupt() {
  INFO("--== interrupt registers ==--\n");
  INFO("REG_ICR : 0x%08x\n", cpu_mem_readd(bar0 + REG_ICR));
  INFO("REG_ITR : 0x%08x\n", cpu_mem_readd(bar0 + REG_ITR));
  INFO("REG_ICS : 0x%08x\n", cpu_mem_readd(bar0 + REG_ICS));
  INFO("REG_IMS : 0x%08x\n", cpu_mem_readd(bar0 + REG_IMS));
  INFO("REG_IMC : 0x%08x\n", cpu_mem_readd(bar0 + REG_IMC));
  INFO("REG_IAM : 0x%08x\n", cpu_mem_readd(bar0 + REG_IAM));
}

void eth_print_registers_receive() {
  INFO("--== receive registers ==--\n");
  INFO("REG_RCTL : 0x%08x\n", cpu_mem_readd(bar0 + REG_RCTL));
  INFO("REG_RCTL1 : 0x%08x\n", cpu_mem_readd(bar0 + REG_RCTL1));
  INFO("REG_ERT : 0x%08x\n", cpu_mem_readd(bar0 + REG_ERT));
  INFO("REG_RDBAL : 0x%08x\n", cpu_mem_readd(bar0 + REG_RDBAL));
  INFO("REG_RDBAH : 0x%08x\n", cpu_mem_readd(bar0 + REG_RDBAH));
  INFO("REG_RDLEN : 0x%08x\n", cpu_mem_readd(bar0 + REG_RDLEN));
  INFO("REG_RDH : 0x%08x\n", cpu_mem_readd(bar0 + REG_RDH));
  INFO("REG_RDT : 0x%08x\n", cpu_mem_readd(bar0 + REG_RDT));
  INFO("REG_PSRCTL : 0x%08x\n", cpu_mem_readd(bar0 + REG_PSRCTL));
  INFO("REG_FCRTL : 0x%08x\n", cpu_mem_readd(bar0 + REG_FCRTL));
  INFO("REG_FCRTH : 0x%08x\n", cpu_mem_readd(bar0 + REG_FCRTH));
  INFO("REG_RDTR : 0x%08x\n", cpu_mem_readd(bar0 + REG_RDTR));
  INFO("REG_RXDCTL : 0x%08x\n", cpu_mem_readd(bar0 + REG_RXDCTL));
  INFO("REG_RADV : 0x%08x\n", cpu_mem_readd(bar0 + REG_RADV));
  INFO("REG_RSRPD : 0x%08x\n", cpu_mem_readd(bar0 + REG_RSRPD));
  INFO("REG_RAID : 0x%08x\n", cpu_mem_readd(bar0 + REG_RAID));
  INFO("REG_CPUVEC : 0x%08x\n", cpu_mem_readd(bar0 + REG_CPUVEC));
  INFO("REG_RXCSUM : 0x%08x\n", cpu_mem_readd(bar0 + REG_RXCSUM));
  INFO("REG_RFCTL : 0x%08x\n", cpu_mem_readd(bar0 + REG_RFCTL));
  INFO("REG_MTA : 0x%08x\n", cpu_mem_readd(bar0 + REG_MTA));
  INFO("REG_RAL : 0x%08x\n", cpu_mem_readd(bar0 + REG_RAL));
  INFO("REG_RAH : 0x%08x\n", cpu_mem_readd(bar0 + REG_RAH));
  INFO("REG_SRAL : 0x%08x\n", cpu_mem_readd(bar0 + REG_SRAL));
  INFO("REG_SRAH : 0x%08x\n", cpu_mem_readd(bar0 + REG_SRAH));
  INFO("REG_SHRAH : 0x%08x\n", cpu_mem_readd(bar0 + REG_SHRAH));
  INFO("REG_MRQC : 0x%08x\n", cpu_mem_readd(bar0 + REG_MRQC));
  INFO("REG_RSSIM : 0x%08x\n", cpu_mem_readd(bar0 + REG_RSSIM));
  INFO("REG_RSSIR : 0x%08x\n", cpu_mem_readd(bar0 + REG_RSSIR));
  INFO("REG_RETA : 0x%08x\n", cpu_mem_readd(bar0 + REG_RETA));
  INFO("REG_RSSRK : 0x%08x\n", cpu_mem_readd(bar0 + REG_RSSRK));
}

void eth_print_registers_transmit() {
  INFO("--== transmit registers ==--\n");
  INFO("REG_TCTL : 0x%08x\n", cpu_mem_readd(bar0 + REG_TCTL));
  INFO("REG_TIPG : 0x%08x\n", cpu_mem_readd(bar0 + REG_TIPG));
  INFO("REG_AIT : 0x%08x\n", cpu_mem_readd(bar0 + REG_AIT));
  INFO("REG_TDBAL : 0x%08x\n", cpu_mem_readd(bar0 + REG_TDBAL));
  INFO("REG_TDBAH : 0x%08x\n", cpu_mem_readd(bar0 + REG_TDBAH));
  INFO("REG_TDLEN : 0x%08x\n", cpu_mem_readd(bar0 + REG_TDLEN));
  INFO("REG_TDH : 0x%08x\n", cpu_mem_readd(bar0 + REG_TDH));
  INFO("REG_TDT : 0x%08x\n", cpu_mem_readd(bar0 + REG_TDT));
  INFO("REG_ARC : 0x%08x\n", cpu_mem_readd(bar0 + REG_ARC));
  INFO("REG_IDV : 0x%08x\n", cpu_mem_readd(bar0 + REG_IDV));
  INFO("REG_TXDCTL : 0x%08x\n", cpu_mem_readd(bar0 + REG_TXDCTL));
  INFO("REG_TDAV : 0x%08x\n", cpu_mem_readd(bar0 + REG_TDAV));
}

void eth_print_registers_statistic() {
  INFO("--== statistic registers ==--\n");
  INFO("REG_CRCERRS : 0x%08x\n", cpu_mem_readd(bar0 + REG_CRCERRS));
  INFO("REG_ALGNERRC : 0x%08x\n", cpu_mem_readd(bar0 + REG_ALGNERRC));
  INFO("REG_RXERRC : 0x%08x\n", cpu_mem_readd(bar0 + REG_RXERRC));
  INFO("REG_MPC : 0x%08x\n", cpu_mem_readd(bar0 + REG_MPC));
  INFO("REG_CEXTERR : 0x%08x\n", cpu_mem_readd(bar0 + REG_CEXTERR));
  INFO("REG_RLEC : 0x%08x\n", cpu_mem_readd(bar0 + REG_RLEC));
  INFO("REG_XONRXC : 0x%08x\n", cpu_mem_readd(bar0 + REG_XONRXC));
  INFO("REG_XONTXC : 0x%08x\n", cpu_mem_readd(bar0 + REG_XONTXC));
  INFO("REG_XOFFRXC : 0x%08x\n", cpu_mem_readd(bar0 + REG_XOFFRXC));
  INFO("REG_XOFFTXC : 0x%08x\n", cpu_mem_readd(bar0 + REG_XOFFTXC));
  INFO("REG_FCRUC : 0x%08x\n", cpu_mem_readd(bar0 + REG_FCRUC));
  INFO("REG_GPRC : 0x%08x\n", cpu_mem_readd(bar0 + REG_GPRC));
  INFO("REG_BPRC : 0x%08x\n", cpu_mem_readd(bar0 + REG_BPRC));
  INFO("REG_MPRC : 0x%08x\n", cpu_mem_readd(bar0 + REG_MPRC));
  INFO("REG_GPTC : 0x%08x\n", cpu_mem_readd(bar0 + REG_GPTC));
  INFO("REG_GORCL : 0x%08x\n", cpu_mem_readd(bar0 + REG_GORCL));
  INFO("REG_GORCH : 0x%08x\n", cpu_mem_readd(bar0 + REG_GORCH));
  INFO("REG_GOTCL : 0x%08x\n", cpu_mem_readd(bar0 + REG_GOTCL));
  INFO("REG_GOTCH : 0x%08x\n", cpu_mem_readd(bar0 + REG_GOTCH));
  INFO("REG_RNBC : 0x%08x\n", cpu_mem_readd(bar0 + REG_RNBC));
  INFO("REG_RUC : 0x%08x\n", cpu_mem_readd(bar0 + REG_RUC));
  INFO("REG_RFC : 0x%08x\n", cpu_mem_readd(bar0 + REG_RFC));
  INFO("REG_ROC : 0x%08x\n", cpu_mem_readd(bar0 + REG_ROC));
  INFO("REG_RJC : 0x%08x\n", cpu_mem_readd(bar0 + REG_RJC));
  INFO("REG_MNGPRC : 0x%08x\n", cpu_mem_readd(bar0 + REG_MNGPRC));
  INFO("REG_MNGPDC : 0x%08x\n", cpu_mem_readd(bar0 + REG_MNGPDC));
  INFO("REG_MNGPTC : 0x%08x\n", cpu_mem_readd(bar0 + REG_MNGPTC));
  INFO("REG_TCBPD : 0x%08x\n", cpu_mem_readd(bar0 + REG_TCBPD));
  INFO("REG_TORL : 0x%08x\n", cpu_mem_readd(bar0 + REG_TORL));
  INFO("REG_TORH : 0x%08x\n", cpu_mem_readd(bar0 + REG_TORH));
  INFO("REG_TOTL : 0x%08x\n", cpu_mem_readd(bar0 + REG_TOTL));
  INFO("REG_TOTH : 0x%08x\n", cpu_mem_readd(bar0 + REG_TOTH));
  INFO("REG_TPR : 0x%08x\n", cpu_mem_readd(bar0 + REG_TPR));
  INFO("REG_TPT : 0x%08x\n", cpu_mem_readd(bar0 + REG_TPT));
  INFO("REG_MPTC : 0x%08x\n", cpu_mem_readd(bar0 + REG_MPTC));
  INFO("REG_BPTC : 0x%08x\n", cpu_mem_readd(bar0 + REG_BPTC));
  INFO("REG_TSCTC : 0x%08x\n", cpu_mem_readd(bar0 + REG_TSCTC));
  INFO("REG_IAC : 0x%08x\n", cpu_mem_readd(bar0 + REG_IAC));
}
