#include "dmar.h"
#include "efiw.h"
#include "stdio.h"
#include "string.h"

void dmar_init(void) {
  uint64_t i, j;

  INFO("INIT DMAR for Intel 4th Gen core family\n");

  struct dmar_pages *dp = efi_allocate_pages(
      sizeof(struct dmar_pages) >> 12);

  // Initialisation of translation tables
  memset(dp, 0, sizeof(struct dmar_pages));

  for (i = 0; i < 256; i++) {
    dp->root_table[i].cpt = ((uint64_t)&dp->context_table[i][0]) >> 12;
    for(j = 0; j < 256; i++) {
      dp->context_table[i][j].slptptr = ((uint64_t)&dp->pml4[0]) >> 12;
    }
  }
}
