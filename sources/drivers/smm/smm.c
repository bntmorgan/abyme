/*
Copyright (C) 2021  Benoît Morgan

This file is part of abyme

abyme is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

abyme is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with abyme.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "smm.h"
#include "msr.h"
#include "pci.h"
#include "stdio.h"

uint8_t smm_unlock_smram() {
  uint8_t *addr = pci_mmio_address(0, 0, 0); 
  uint8_t smramc = pci_mem_readb(addr + PCI_OFFSET_SMRAMC);
  uint8_t d_open = (smramc & PCI_SMRAMC_D_OPEN) != 0;
  uint8_t d_lck = (smramc & PCI_SMRAMC_D_LCK) != 0;
  uint8_t d_cls = (smramc & PCI_SMRAMC_D_CLS) != 0;
  uint8_t g_smrame = (smramc & PCI_SMRAMC_G_SMRAME) != 0;
  uint8_t c_base_seg = smramc & PCI_SMRAMC_C_BASE_SEG;
  INFO("Memory controller PCI configuration space address 0x%016X\n", addr);
  INFO("d_open %d, d_lck %d, d_cls %d, g_smrame %d, c_base_seg %d\n", d_open,
      d_lck, d_cls, g_smrame, c_base_seg);
  if (d_lck) {
    INFO("SMRAM is locked... too late\n");
    return 1;
  }
  if (!d_open) {
    INFO("We set the SMM mode open for EVERYONE who has the physical access !\n");
    smramc |= PCI_SMRAMC_D_OPEN;
    pci_mem_writeb(addr + PCI_OFFSET_SMRAMC, smramc);
  }
  return 0;
}

uint64_t smm_get_smbase() {
  return msr_read(MSR_ADDRESS_IA32_SMBASE);
}
