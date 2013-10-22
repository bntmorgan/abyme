#ifndef __82579LM_H__
#define __82579LM_H__

#include <efi.h>

#include "types.h"
#include "addr.h"
#include "pci.h"

// 
// Intel® 82579 Gigabit Ethernet PHY Datasheet v2.1
// 12.0 Intel® 6 Series Express Chipset MAC Programming Interface
//

//
// Pci bus / device / port
//

//
// Registers
//

#define REG_CTRL                        0x0000      // Device Control
#define REG_STATUS                      0x0008      // Status
#define REG_EERD                        0x0014      // EEPROM Read
#define REG_ICR                         0x00c0      // Interrupt Cause Read
#define REG_IMS                         0x00d0      // Interrupt Mask Set/Read
#define REG_RCTL                        0x0100      // Receive Control
#define REG_TCTL                        0x0400      // Transmit Control
#define REG_RDBAL                       0x2800      // Receive Descriptor Base Low
#define REG_RDBAH                       0x2804      // Receive Descriptor Base High
#define REG_RDLEN                       0x2808      // Receive Descriptor Length
#define REG_RDH                         0x2810      // Receive Descriptor Head
#define REG_RDT                         0x2818      // Receive Descriptor Tail
#define REG_TDBAL                       0x3800      // Transmit Descriptor Base Low
#define REG_TDBAH                       0x3804      // Transmit Descriptor Base High
#define REG_TDLEN                       0x3808      // Transmit Descriptor Length
#define REG_TDH                         0x3810      // Transmit Descriptor Head
#define REG_TDT                         0x3818      // Transmit Descriptor Tail
#define REG_MTA                         0x5200      // Multicast Table Array
#define REG_RAL                         0x5400      // Receive Address Low
#define REG_RAH                         0x5404      // Receive Address High

//
// Control register
//

#define CTRL_SLU                        (1 << 6)    // Set Link Up
#define CTRL_LCD_RST                    (1 << 31)   // LAN Connected device reset
#define CTRL_SWRST                      (1 << 26)  // Host sofware reset

// 
// RCTL Register
//

#define RCTL_EN                         (1 << 1)    // Receiver Enable
#define RCTL_SBP                        (1 << 2)    // Store Bad Packets
#define RCTL_UPE                        (1 << 3)    // Unicast Promiscuous Enabled
#define RCTL_MPE                        (1 << 4)    // Multicast Promiscuous Enabled
#define RCTL_LPE                        (1 << 5)    // Long Packet Reception Enable
#define RCTL_LBM_NONE                   (0 << 6)    // No Loopback
#define RCTL_LBM_PHY                    (3 << 6)    // PHY or external SerDesc loopback
#define RTCL_RDMTS_HALF                 (0 << 8)    // Free Buffer Threshold is 1/2 of RDLEN
#define RTCL_RDMTS_QUARTER              (1 << 8)    // Free Buffer Threshold is 1/4 of RDLEN
#define RTCL_RDMTS_EIGHTH               (2 << 8)    // Free Buffer Threshold is 1/8 of RDLEN
#define RCTL_MO_36                      (0 << 12)   // Multicast Offset - bits 47:36
#define RCTL_MO_35                      (1 << 12)   // Multicast Offset - bits 46:35
#define RCTL_MO_34                      (2 << 12)   // Multicast Offset - bits 45:34
#define RCTL_MO_32                      (3 << 12)   // Multicast Offset - bits 43:32
#define RCTL_BAM                        (1 << 15)   // Broadcast Accept Mode
#define RCTL_VFE                        (1 << 18)   // VLAN Filter Enable
#define RCTL_CFIEN                      (1 << 19)   // Canonical Form Indicator Enable
#define RCTL_CFI                        (1 << 20)   // Canonical Form Indicator Bit Value
#define RCTL_DPF                        (1 << 22)   // Discard Pause Frames
#define RCTL_PMCF                       (1 << 23)   // Pass MAC Control Frames
#define RCTL_SECRC                      (1 << 26)   // Strip Ethernet CRC

//
// TCTL Register
//

#define TCTL_EN                         (1 << 1)    // Transmit Enable
#define TCTL_PSP                        (1 << 3)    // Pad Short Packets
#define TCTL_CT_SHIFT                   4           // Collision Threshold
#define TCTL_COLD_SHIFT                 12          // Collision Distance
#define TCTL_SWXOFF                     (1 << 22)   // Software XOFF Transmission
#define TCTL_RTLC                       (1 << 24)   // Re-transmit on Late Collision

//
// Stat registers
//

#define STAT_CRCERRS                    0x4000
#define STAT_RXERRC                     0x400C
#define STAT_MPC                        0x4010
#define STAT_CEXTERR                    0x403C
#define STAT_XONRXC                     0x4048
#define STAT_XONTCX                     0x404C
#define STAT_XOFFRXC                    0x4050
#define STAT_XOFFTCX                    0x4054
#define STAT_FCRUC                      0x4058
#define STAT_GPRC                       0x4074
#define STAT_BPRC                       0x4078
#define STAT_MPRC                       0x407C
#define STAT_GPTC                       0x4080
#define STAT_GORCL                      0x4088
#define STAT_GORCH                      0x408C
#define STAT_GOTCL                      0x4090
#define STAT_GOTCH                      0x4094
#define STAT_RNBC                       0x40A0
#define STAT_RUC                        0x40A4
#define STAT_RFC                        0x40A8
#define STAT_ROC                        0x40AC
#define STAT_RJC                        0x40B0
#define STAT_MNGPRC                     0x40B4
#define STAT_MNGPDC                     0x40B8
#define STAT_MNGPTC                     0x40BC
#define STAT_TCBPD                      0x40D8
#define STAT_TORL                       0x40C0
#define STAT_TORH                       0x40C4
#define STAT_TOTL                       0x40C8
#define STAT_TOTH                       0x40CC
#define STAT_TPR                        0x40D0
#define STAT_TPT                        0x40D4
#define STAT_MPTC                       0x40F0
#define STAT_BPTC                       0x40F4
#define STAT_TSCTC                      0x40F8
#define STAT_IAC                        0x4100


//
// Buffer Sizes
//

#define RCTL_BSIZE_256                  (3 << 16)
#define RCTL_BSIZE_512                  (2 << 16)
#define RCTL_BSIZE_1024                 (1 << 16)
#define RCTL_BSIZE_2048                 (0 << 16)
#define RCTL_BSIZE_4096                 ((3 << 16) | (1 << 25))
#define RCTL_BSIZE_8192                 ((2 << 16) | (1 << 25))
#define RCTL_BSIZE_16384                ((1 << 16) | (1 << 25))

#define ETH_VENDOR_ID 0x8086 // Intel

#define ETH_DEVICE_ID 0x1502 // 82579LM

#define RX_DESC_COUNT                   32
#define TX_DESC_COUNT                   8

#define PACKET_SIZE                     2048

// ------------------------------------------------------------------------------------------------
// Receive Descriptor
typedef struct _recv_desc
{
    volatile uint64_t addr;
    volatile uint16_t len;
    volatile uint16_t checksum;
    volatile uint8_t status;
    volatile uint8_t errors;
    volatile uint16_t special;
} __attribute__((aligned(0x10))) __attribute__((packed)) recv_desc;

//
// Receive Status
//

#define RSTA_DD                         (1 << 0)    // Descriptor Done
#define RSTA_EOP                        (1 << 1)    // End of Packet
#define RSTA_IXSM                       (1 << 2)    // Ignore Checksum Indication
#define RSTA_VP                         (1 << 3)    // Packet is 802.1Q
#define RSTA_TCPCS                      (1 << 5)    // TCP Checksum Calculated on Packet
#define RSTA_IPCS                       (1 << 6)    // IP Checksum Calculated on Packet
#define RSTA_PIF                        (1 << 7)    // Passed in-exact filter

// 
// Transmit Descriptor
//

typedef struct _trans_desc
{
    volatile uint64_t addr;
    volatile uint16_t len;
    volatile uint8_t cso;
    volatile uint8_t cmd;
    volatile uint8_t status;
    volatile uint8_t css;
    volatile uint16_t special;
} __attribute__((aligned(0x10))) __attribute__((packed)) trans_desc;

//
// Transmit Command
//

#define CMD_EOP                         (1 << 0)    // End of Packet
#define CMD_IFCS                        (1 << 1)    // Insert FCS
#define CMD_IC                          (1 << 2)    // Insert Checksum
#define CMD_RS                          (1 << 3)    // Report Status
#define CMD_RPS                         (1 << 4)    // Report Packet Sent
#define CMD_VLE                         (1 << 6)    // VLAN Packet Enable
#define CMD_IDE                         (1 << 7)    // Interrupt Delay Enable

//
// Transmit Status
//

#define TSTA_DD                         (1 << 0)    // Descriptor Done
#define TSTA_EC                         (1 << 1)    // Excess Collisions
#define TSTA_LC                         (1 << 2)    // Late Collision
#define LSTA_TU                         (1 << 3)    // Transmit Underrun

//
// Buffers
//
#define NET_BUF_SIZE                    2048

//
// MTU
//

#define ETH_MTU 1500
#define ETH_LEN 1516

typedef struct _eth_header {
    eth_addr dst;
    eth_addr src;
    uint16_t type;
} __attribute__((packed)) eth_header;

uint8_t eth_get_device();
uint8_t eth_setup();
uint8_t eth_init();

void eth_send(const void *buf, uint16_t len, uint8_t block);
uint32_t eth_recv(void *buf, uint32_t len, uint8_t block);

static inline pci_device_addr *eth_get_pci_addr() {
  extern pci_device_addr addr;
  return &addr;
}

static inline eth_addr *eth_get_laddr() {
  extern eth_addr laddr;
  return &laddr;
}

static inline uint8_t *eth_get_bar0() {
  extern uint8_t *bar0;
  return bar0;
}

#endif
