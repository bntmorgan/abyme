#include "pmem.h"

#include "stdio.h"

void pmem_print_info(pmem_mmap_t *pmem_mmap) {
  INFO("mmap: at %08x with %d sections\n",
      (uint32_t) ((uint64_t) pmem_mmap), pmem_mmap->nb_area);
  for (uint8_t i = 0; i < pmem_mmap->nb_area; i++) {
    INFO("size=%2x addr=%08X len=%08X type=%01x\n",
        (uint32_t) pmem_mmap->area[i].size,
        (uint64_t) pmem_mmap->area[i].addr,
        (uint64_t) pmem_mmap->area[i].len,
        (uint32_t) pmem_mmap->area[i].type);
  }
}
