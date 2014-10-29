#include "stdint.h"
#include "stdio.h"
#include "walk.h"

uint8_t walk_is_in_page(uint64_t l, uint64_t p, uint64_t size) {
  switch (size) {
    case WALK_1GB:
      return (p & ~(uint64_t)0x3fffffff) == (l & ~(uint64_t)0x3fffffff);
    case WALK_2MB:
      return (p & ~(uint64_t)0x1fffff) == (l & ~(uint64_t)0x1fffff);
    case WALK_4KO:
      return (p & ~(uint64_t)0xfff) == (l & ~(uint64_t)0xfff);
    default:
      return 0;
  }
}

uint64_t walk_long(uint64_t cr3, uint64_t linear, uint64_t *physical, uint64_t
    *size) {
  DBG("linear : 0x%016X\n", linear);
  DBG("cr3 : 0x%016X\n", cr3);
  uint64_t pml4 = cr3 & 0x000ffffffffff000;
  uint64_t pml4e = *(uint64_t*)(pml4 | ((linear >> 36) & (0x1ff << 3)));
  DBG("pml4e : 0x%016X\n", pml4e);
  if (!(pml4e & (1 << 0))) {
    return WALK_ERROR_PML4E_NOT_PRESENT;
  }
  uint64_t pdpt = pml4e & 0x000ffffffffff000;
  uint64_t pdpte = *(uint64_t*)(pdpt | ((linear >> 27) & (0x1ff << 3)));
  DBG("pdpte : 0x%016X\n", pdpte);
  if (!(pdpte & (1 << 0))) {
    return WALK_ERROR_PDPTE_NOT_PRESENT;
  } else if (pdpte & (1 << 7)) {
    if (physical) {
      *physical = (pdpte & 0x000fffffc0000000) | (linear & 0x000000003fffffff);
    }
    if (size) {
      *size = WALK_1GB;
    }
    return WALK_OK;
  }
  uint64_t pd = pdpte & 0x000ffffffffff000;
  uint64_t pde = *(uint64_t*)(pd | ((linear >> 18) & (0x1ff << 3)));
  DBG("pde : 0x%016X\n", pde);
  if (!(pde & (1 << 0))) {
    return WALK_ERROR_PDE_NOT_PRESENT;
  } else if (pde & (1 << 7)) {
    if (physical) {
      *physical = (pde & 0x000fffffffe00000) | (linear & 0x00000000001fffff);
    }
    if (size) {
      *size = WALK_2MB;
    }
    return WALK_OK;
  }
  uint64_t pt = pde & 0x000ffffffffff000;
  uint64_t pte = *(uint64_t*)(pt | ((linear >> 9) & (0x1ff << 3)));
  DBG("pte : 0x%016X\n", pte);
  if (!(pte & (1 << 0))) {
    return WALK_ERROR_PTE_NOT_PRESENT;
  }
  if (physical) {
    *physical = (pte & 0x000ffffffffff000) + (linear & 0x0000000000000fff);
  }
  if (size) {
    *size = WALK_4KO;
  }
  return WALK_OK;
}
