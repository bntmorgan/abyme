#ifndef __82579LM_H__
#define __82579LM_H__

#include <efi.h>

#include "types.h"

//
// Pci bus / device / port
//

//
// Registers
//

#define REG_CTRL                        0x0000      // Device Control
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

#define ETH_VENDOR_ID 0x8086 // Intel

#define ETH_DEVICE_ID 0x1503 // 82579LM

uint8_t eth_get_device();
uint8_t eth_setup();

#endif
