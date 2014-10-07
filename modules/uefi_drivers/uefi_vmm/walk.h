#ifndef __WALK_H__
#define __WALK_H__

enum walk_error {
  WALK_OK,
  WALK_ERROR_PML4E_NOT_PRESENT,
  WALK_ERROR_PDPTE_NOT_PRESENT,
  WALK_ERROR_PDE_NOT_PRESENT,
  WALK_ERROR_PTE_NOT_PRESENT
};

uint64_t walk_long(uint64_t cr3, uint64_t linear, uint64_t *physical);

#endif//__WALK_H__
