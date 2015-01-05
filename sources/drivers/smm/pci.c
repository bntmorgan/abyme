#include "pci.h"

inline uint8_t pci_mem_readb(uint8_t *addr) {
  return *addr;
}

inline uint16_t pci_mem_readw(uint8_t *addr) {
  return *((uint16_t*)addr);
}

inline uint32_t pci_mem_readd(uint8_t *addr) {
  return *((uint32_t*)addr);
}

inline void pci_mem_writeb(uint8_t *addr, uint8_t val) {
  *addr = val;
}

inline void pci_mem_writew(uint8_t *addr, uint16_t val) {
  *((uint16_t *)addr) = val;
}

inline void pci_mem_writed(uint8_t *addr, uint32_t val) {
  *((uint32_t *)addr) = val;
}

inline uint8_t *pci_mmio_address(uint8_t bus, uint8_t dev, uint8_t fun) {
  return (uint8_t *)((uint64_t)PCI_MMIO_BASE | (fun << 12) | (dev <<  15) | (bus << 23));
}
