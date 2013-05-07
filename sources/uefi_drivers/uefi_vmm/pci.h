#ifndef __PCI_H__
#define __PCI_H__

#include <efi.h>

#define PCI_MAKE_ID(bus, dev, func)     ((bus) << 16) | ((dev) << 11) | ((func) << 8)

// 
// I/O Ports
//
#define PCI_CONFIG_ADDR                 0xcf8
#define PCI_CONFIG_DATA                 0xcfc

inline uint32_t pci_make_addr(uint32_t id);

#endif
