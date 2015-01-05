#ifndef __PCI_H__
#define __PCI_H__

#include "types.h"

//
// PCI MMIO Configuration space
//
#define PCI_MMIO_BASE 0xf8000000

//
// Memory controller configuration registers
//
#define PCI_OFFSET_SMRAMC 0x88

//
// Memory controller SMRAMC bitmaps
//
#define PCI_SMRAMC_D_OPEN       (1 << 6)
#define PCI_SMRAMC_D_CLS        (1 << 5)
#define PCI_SMRAMC_D_LCK        (1 << 4)
#define PCI_SMRAMC_G_SMRAME     (1 << 3)
#define PCI_SMRAMC_C_BASE_SEG   (7 << 0)
/*typedef union {
  uint8_t raw;
  struct {
    uint8_t c_base_seg:3; 
    uint8_t g_smrame:1;
    uint8_t d_lck:1;
    uint8_t d_cls:1;
    uint8_t 
  }
} pci_smramc;*/

inline uint8_t pci_mem_readb(uint8_t *addr);
inline uint16_t pci_mem_readw(uint8_t *addr);
inline uint32_t pci_mem_readd(uint8_t *addr);
inline void pci_mem_writeb(uint8_t *addr, uint8_t val);
inline void pci_mem_writew(uint8_t *addr, uint16_t val);
inline void pci_mem_writed(uint8_t *addr, uint32_t val);

inline uint8_t *pci_mmio_address(uint8_t bus, uint8_t dev, uint8_t fun);

#endif//__PCI_H__
