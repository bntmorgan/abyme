#include "common/string_int.h"

#include "pmem_int.h"

void pmem_print_info(pmem_mmap_t *pmem_mmap) {
  INFO("mmap: at %08x with %d sections\n",
      (uint32_t) ((uint64_t) pmem_mmap), pmem_mmap->nb_area);
  for (uint8_t i = 0; i < pmem_mmap->nb_area; i++) {
    INFO("size=%2x addr=%08x%08x len=%08x%08x type=%01x\n",
        (uint32_t) (pmem_mmap->area[i].size),
        (uint32_t) (pmem_mmap->area[i].addr >> 32),
        (uint32_t) (pmem_mmap->area[i].addr & 0xffffffff),
        (uint32_t) (pmem_mmap->area[i].len >> 32),
        (uint32_t) (pmem_mmap->area[i].len & 0xffffffff),
        (uint32_t) (pmem_mmap->area[i].type));
  }
}
