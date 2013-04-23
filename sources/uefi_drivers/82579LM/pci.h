#ifndef __PCI_H__
#define __PCI_H__

#include <efi.h>

#define PCI_MAKE_ID(bus, dev, func)     ((bus) << 16) | ((dev) << 11) | ((func) << 8)

// 
// I/O Ports
//
#define PCI_CONFIG_ADDR                 0xcf8
#define PCI_CONFIG_DATA                 0xcfc

//
// Header Type
//

#define PCI_TYPE_MULTIFUNC              0x80
#define PCI_TYPE_GENERIC                0x00
#define PCI_TYPE_PCI_BRIDGE             0x01
#define PCI_TYPE_CARDBUS_BRIDGE         0x02

//
// PCI Configuration Registers
//

#define PCI_CONFIG_VENDOR_ID            0x00
#define PCI_CONFIG_DEVICE_ID            0x02
#define PCI_CONFIG_COMMAND              0x04
#define PCI_CONFIG_STATUS               0x06
#define PCI_CONFIG_REVISION_ID          0x08
#define PCI_CONFIG_PROG_INTF            0x09
#define PCI_CONFIG_SUBCLASS             0x0a
#define PCI_CONFIG_CLASS_CODE           0x0b
#define PCI_CONFIG_CACHELINE_SIZE       0x0c
#define PCI_CONFIG_LATENCY              0x0d
#define PCI_CONFIG_HEADER_TYPE          0x0e
#define PCI_CONFIG_BIST                 0x0f

typedef struct _pci_device_addr
{
    uint8_t bus;
    uint8_t device;
    uint8_t function;
} pci_device_addr;

typedef struct _pci_device_info
{
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t sub_class;
    uint8_t prog_intf;
} pci_device_info;

uint8_t pci_get_device(uint32_t vendor_id, uint32_t device_id, pci_device_info *info, pci_device_addr *addr);

#endif
