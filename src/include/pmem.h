#ifndef __PMEM_H__
#define __PMEM_H__

#include <stdint.h>

typedef struct {
  uint32_t size;
  uint64_t addr;
  uint64_t len;
  uint32_t type;
} __attribute__((packed)) pmem_area_t;

typedef struct {
  pmem_area_t area[16];
  uint32_t nb_area;
} __attribute__((packed)) pmem_mmap_t;

#endif
