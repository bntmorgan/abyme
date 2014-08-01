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

// General registers
#define REG_CTRL                        0x0000      // Device Control
#define REG_STATUS                      0x0008      // Status
#define REG_STRAP                       0x000c      // Strapping option register
#define REG_EERD                        0x0014      // EEPROM Read
#define REG_CTRL_EXT                    0x0018      // Extended control device register
#define REG_MDIC                        0x0020      // MDI Control Register
#define REG_FEXTNVM4                    0x0024      // Future Extended4 NVM Register
#define REG_FEXTNVM                     0x0028      // Future Extended NVM Register
#define REG_FEXT                        0x002C      // Future Extended Register
#define REG_FEXTNVM2                    0x0030      // Future Extended2 NVM Register
#define REG_KUMCTRLSTA                  0x0034      // Kumeran control and status registers
#define REG_BUSNUM                      0x0038      // Device and bus number
#define REG_LTRV                        0x00f8      // Latency Tolerance Reporting Value
#define REG_LPIC                        0x00fc      // Low Power Idle Control
#define REG_FCTTV                       0x0170      // Flow Control Transmit Timer Value
#define REG_FCRTV                       0x5f40      // Flow Control Refresh Threshold Value
#define REG_EXTCNF_CTRL                 0x0f00      // Extended Configuration Control
#define REG_EXTCNF_SIZE                 0x0f08      // Extended Configuration Size
#define REG_PHY_CTRL                    0x0f10      // PHY Control Register
#define REG_PCIEANACFG                  0x0f18      // PCIE Analog Configuration
#define REG_PBA                         0x1000      // Packet Buffer Allocation
#define REG_PBS                         0x1008      // Packet Buffer Allocation
#define REG_DCR                         0x5B00      // DMA Control Register

// Interrupt registers
#define REG_ICR                         0x00c0      // Interrupt Cause Read
#define REG_ITR                         0x00c4      // Interrupt Throttling Register
#define REG_ICS                         0x00c8      // Interrupt Cause Set Register
#define REG_IMS                         0x00d0      // Interrupt Mask Set/Read
#define REG_IMC                         0x00d8      // Interrupt Mask Clear
#define REG_IAM                         0x00e0      // Interrupt Acknoledge Auto

// Receive registers
#define REG_RCTL                        0x0100      // Receive Control
#define REG_RCTL1                       0x0104      // Receive Control 1
#define REG_ERT                         0x2008      // Early receive control
#define REG_RDBAL                       0x2800      // Receive Descriptor Base Low
#define REG_RDBAH                       0x2804      // Receive Descriptor Base High
#define REG_RDLEN                       0x2808      // Receive Descriptor Length
#define REG_RDH                         0x2810      // Receive Descriptor Head
#define REG_RDT                         0x2818      // Receive Descriptor Tail
#define REG_PSRCTL                      0x2170      // + n*0x4 [n=0..1] Packet Split Receive Control Register
#define REG_FCRTL                       0x2160      // Flow Control Receive Threshold Low
#define REG_FCRTH                       0x2168      // Flow Control Receive Threshold High
#define REG_RDTR                        0x2820      // + n*0x100[n=0..1] Interrupt Delay Timer (Packet Timer)
#define REG_RXDCTL                      0x2828      // + n*0x100[n=0..1] Receive Descriptor Control
#define REG_RADV                        0x282C      // Receive Interrupt Absolute Delay Timer
#define REG_RSRPD                       0x2C00      // Receive Small Packet Detect Interrupt
#define REG_RAID                        0x2C08      // Receive ACK Interrupt Delay Register
#define REG_CPUVEC                      0x2C10      // CPU Vector Register
#define REG_RXCSUM                      0x5000      // Receive Checksum Control
#define REG_RFCTL                       0x5008      // Receive Filter Control Register
#define REG_MTA                         0x5200      // MTA[31:0] Multicast Table Array
#define REG_RAL                         0x5400      // + 8*n (n=0...6) // Receive Address Low
#define REG_RAH                         0x5404      // + 8*n (n=0...6) // Receive Address High
#define REG_SRAL                        0x5438      // + 8*n (n=0...3) // Shared Receive Address Low
#define REG_SRAH                        0x543C      // + 8*n (n=0...2) // Shared Receive Address High 0...2
#define REG_SHRAH                       0x5454      // Shared Receive Address High 3
#define REG_MRQC                        0x5818      // Multiple Receive Queues Command Register
#define REG_RSSIM                       0x5864      // RSS Interrupt Mask Register
#define REG_RSSIR                       0x5868      // RSS Interrupt Request Register
#define REG_RETA                        0x5C00      // + 4*n (n=0...31) Redirection Table
#define REG_RSSRK                       0x5C80      // + 4*n (n=0...9) Random Key Register

// Transmit registers
#define REG_TCTL                        0x0400      // Transmit Control
#define REG_TIPG                        0x0410      // Transmit IPG Register
#define REG_AIT                         0x0458      // Adaptive IFS Throttle
#define REG_TDBAL                       0x3800      // Transmit Descriptor Base Low
#define REG_TDBAH                       0x3804      // Transmit Descriptor Base High
#define REG_TDLEN                       0x3808      // Transmit Descriptor Length
#define REG_TDH                         0x3810      // Transmit Descriptor Head
#define REG_TDT                         0x3818      // Transmit Descriptor Tail
#define REG_ARC                         0x3840      // Transmit Arbitration Count
#define REG_IDV                         0x3820      // Transmit Interrupt Delay Value
#define REG_TXDCTL                      0x3828      // Transmit Desciptor Control
#define REG_TDAV                        0x382C      // Transmit Absolute Interrupt Delay Value

// Statistic registers
#define REG_CRCERRS                     0x4000      // CRC Error Count RO
#define REG_ALGNERRC                    0x4004      // Alignment Error Count RO
#define REG_RXERRC                      0x400C      // RX Error Count RO
#define REG_MPC                         0x4010      // Missed Packets Count RO
#define REG_CEXTERR                     0x403C      // Carrier Extension Error Count RO
#define REG_RLEC                        0x4040      // Receive Length Error Count RO
#define REG_XONRXC                      0x4048      // XON Received Count RO
#define REG_XONTXC                      0x404C      // XON Transmitted Count RO
#define REG_XOFFRXC                     0x4050      // XOFF Received Count RO
#define REG_XOFFTXC                     0x4054      // XOFF Transmitted Count RO
#define REG_FCRUC                       0x4058      // FC Received Unsupported Count RO
#define REG_GPRC                        0x4074      // Good Packets Received Count RO
#define REG_BPRC                        0x4078      // Broadcast Packets Received Count RO
#define REG_MPRC                        0x407C      // Multicast Packets Received Count RO
#define REG_GPTC                        0x4080      // Good Packets Transmitted Count RO
#define REG_GORCL                       0x4088      // Good Octets Received Count Low RO
#define REG_GORCH                       0x408C      // Good Octets Received Count High RO
#define REG_GOTCL                       0x4090      // Good Octets Transmitted Count Low RO
#define REG_GOTCH                       0x4094      // Good Octets Transmitted Count High RO
#define REG_RNBC                        0x40A0      // Receive No Buffers Count RO
#define REG_RUC                         0x40A4      // Receive Undersize Count RO
#define REG_RFC                         0x40A8      // Receive Fragment Count RO
#define REG_ROC                         0x40AC      // Receive Oversize Count RO
#define REG_RJC                         0x40B0      // Receive Jabber Count RO
#define REG_MNGPRC                      0x40B4      // Management Packets Received Count RO
#define REG_MNGPDC                      0x40B8      // Management Packets Dropped Count RO
#define REG_MNGPTC                      0x40BC      // Management Packets Transmitted Count RO
#define REG_TCBPD                       0x40D8      // Tx Circuit Breaker Packets Dropped RO
#define REG_TORL                        0x40C0      // Total Octets Received Low RO
#define REG_TORH                        0x40C4      // Total Octets Received High RO
#define REG_TOTL                        0x40C8      // Total Octets Transmitted RO
#define REG_TOTH                        0x40CC      // Total Octets Transmitted RO
#define REG_TPR                         0x40D0      // Total Packets Received RO
#define REG_TPT                         0x40D4      // Total Packets Transmitted RO
#define REG_MPTC                        0x40F0      // Multicast Packets Transmitted Count RO
#define REG_BPTC                        0x40F4      // Broadcast Packets Transmitted Count RO
#define REG_TSCTC                       0x40F8      // TCP Segmentation Context Transmitted Count RO
#define REG_IAC                         0x4100      // Interrupt Assertion Count RO

//
// Bitmaps
//

// Control register
#define CTRL_SLU                        (1 << 6)    // Set Link Up
#define CTRL_LCD_RST                    (1 << 31)   // LAN Connected device reset
#define CTRL_SWRST                      (1 << 26)   // Host sofware reset

// IMC Register
#define IMC_RXT0                        (1 << 7)    // Clears mask for Receiver
                                                    // Timer Interrupt
#define IMC_MDAC                        (1 << 9)    // Clears mask for MDIO
                                                    // Access Complete Interrupt
#define IMC_PHYINT                      (1 << 12)   // Clears PHY Interrupt
#define IMC_LSECPN                      (1 << 14)   // Clears the MACsec Packet
                                                    // Number Interrupt
#define IMC_SRPD                        (1 << 16)   // Clears mask for Small
                                                    // Receive Packet Detect
                                                    // Interrupt
#define IMC_ACK                         (1 << 17)   // Clears the mask for
                                                    // Receive ACK frame detect
                                                    // Interrupt
#define IMC_MNG                         (1 << 18)   // Clears mask for the
                                                    // Manageability Event Interrupt

// RCTL Register
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

// TCTL Register
#define TCTL_EN                         (1 << 1)    // Transmit Enable
#define TCTL_PSP                        (1 << 3)    // Pad Short Packets
#define TCTL_CT_SHIFT                   4           // Collision Threshold
#define TCTL_COLD_SHIFT                 12          // Collision Distance
#define TCTL_SWXOFF                     (1 << 22)   // Software XOFF Transmission
#define TCTL_RTLC                       (1 << 24)   // Re-transmit on Late Collision

// Extended configuration control register
#define EXTCNF_CTRL_SWFLAG              (1 << 5)
#define EXTCNF_CNTRL_GATEPHYCONF        (1 << 7)

// Extended control register
#define STATUS_LAN_INIT_DONE            (1 << 9)

// Extended control register
#define CTRL_EXT_DRV_LOAD               (1 << 28)   // Driver loaded


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

//#define ETH_DEVICE_ID 0x1502 // 82579LM
#define ETH_DEVICE_ID 0x153A // I217-LM

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
void eth_print_registers();
void eth_print_registers_general();
void eth_print_registers_interrupt();
void eth_print_registers_receive();
void eth_print_registers_transmit();

void eth_send(const void *buf, uint16_t len, uint8_t block);
uint32_t eth_recv(void *buf, uint32_t len, uint8_t block);

void eth_disable_debug(void);

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
