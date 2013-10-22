#include "paging.h"
#include "stdio.h"

struct paging_ia32e {
  uint64_t PML4[512]      __attribute__((aligned(0x1000)));
  uint64_t PDPT[512][512] __attribute__((aligned(0x1000)));
} __attribute__((aligned(8)));

struct paging_ia32e paging_ia32e;

void paging_setup_host_paging(void) {
  /* TODO: pcide dans le registre cr4 */
  INFO("SETTING HOST UP PAGING\n");
  uint64_t i;
  uint64_t j;
  for (i = 0; i < 512; i++) {
    paging_ia32e.PML4[i] = ((uint64_t) &paging_ia32e.PDPT[i][0]) | 0x7;
    for (j = 0; j < 512; j++) {
      paging_ia32e.PDPT[i][j] = ((i * 512 + j) << 30) | (1 << 7) | 0x7;
//printk("%08X \n", paging_ia32e.PDPT[i][j]);
//if (j == 4) {
//while (1);
//}
    }
  }
//while (1);
}

uint64_t paging_get_host_cr3(void) {
  return (uint64_t) &paging_ia32e.PML4[0];
}
