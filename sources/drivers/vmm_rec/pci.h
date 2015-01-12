#ifndef __PCI_H__
#define __PCI_H__

#include <efi.h>

#define PCI_MAKE_ID(bus, dev, func)     (((bus) << 16) | ((dev) << 11) | ((func) << 8))
#define PCI_MAKE_MMCONFIG(bus, dev, func)     ((((bus) << 8) | ((dev) << 3) | (func)) << 12) 

// 
// I/O Ports
//
#define PCI_CONFIG_ADDR                 0xcf8
#define PCI_CONFIG_DATA                 0xcfc

inline uint32_t pci_make_addr(uint32_t id);
uint8_t pci_no_protect_out(uint16_t port, uint32_t value);
uint8_t pci_no_protect_in(uint16_t port);

uint8_t pci_readb(uint32_t id, uint32_t reg);
uint16_t pci_readw(uint32_t id, uint32_t reg);
uint32_t pci_readd(uint32_t id, uint32_t reg);
void pci_writeb(uint32_t id, uint32_t reg, uint8_t data);
void pci_writew(uint32_t id, uint32_t reg, uint16_t data);
void pci_writed(uint32_t id, uint32_t reg, uint32_t data);

#endif
