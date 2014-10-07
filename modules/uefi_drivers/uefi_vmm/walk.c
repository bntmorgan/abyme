#include "stdint.h"
#include "stdio.h"
#include "walk.h"

uint64_t walk_long(uint64_t cr3, uint64_t linear, uint64_t *physical) {
  uint64_t pml4 = cr3 & 0x000ffffffffff000;
  uint64_t pml4e = *(uint64_t*)(pml4 | ((linear >> 36) & (0x1ff << 3)));
  if (!(pml4e & (1 << 0))) {
    return WALK_ERROR_PML4E_NOT_PRESENT;
  }
  uint64_t pdpt = pml4e & 0x000ffffffffff000;
  uint64_t pdpte = *(uint64_t*)(pdpt | ((linear >> 27) & (0x1ff << 3)));
  if (!(pdpte & (1 << 0))) {
    return WALK_ERROR_PDPTE_NOT_PRESENT;
  } else if (pdpte & (1 << 7)) {
    *physical = (pdpte & 0x000fffffc0000000) | (linear & 0x000000003fffffff);
    return WALK_OK;
  }
  uint64_t pd = pdpte & 0x000ffffffffff000;
  uint64_t pde = *(uint64_t*)(pd | ((linear >> 18) & (0x1ff << 3)));
  if (!(pde & (1 << 0))) {
    return WALK_ERROR_PDE_NOT_PRESENT;
  } else if (pde & (1 << 7)) {
    *physical = (pde & 0x000fffffffe00000) | (linear & 0x00000000001fffff);
    return WALK_OK;
  }
  uint64_t pt = pde & 0x000ffffffffff000;
  uint64_t pte = *(uint64_t*)(pt | ((linear >> 9) & (0x1ff << 3)));
  if (!(pte & (1 << 0))) {
    return WALK_ERROR_PTE_NOT_PRESENT;
  }
  *physical = (pte & 0x000ffffffffff000) + (linear & 0x0000000000000fff);
  return WALK_OK;
}
