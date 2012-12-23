#include "pmem.h"

#include "stdio.h"
#include "string.h"

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

void pmem_fix_info(pmem_mmap_t *pmem_mmap, uint64_t vmm_start) {
  uint8_t target_area = 0;

  /* Find the memory area containing vmm */
  for (uint8_t i = 0; i < pmem_mmap->nb_area; i++) {
    if (pmem_mmap->area[i].type == 1 && vmm_start >= pmem_mmap->area[i].addr && vmm_start < pmem_mmap->area[i].addr + pmem_mmap->area[i].len) {
      target_area = i;
      break;
    }
  }

  /* If it is not the last area, shift the next areas */
  if (target_area != pmem_mmap->nb_area - 1) {
    uint32_t size = (pmem_mmap->nb_area - target_area - 1) * sizeof(pmem_area_t);
    uint8_t tmp[size];
    memcpy(tmp, &pmem_mmap->area[target_area+1], size);
    memcpy(&pmem_mmap->area[target_area+2], tmp, size);
  }

  /* Exclude vmm from the memory area */
  uint64_t target_area_end = pmem_mmap->area[target_area].addr + pmem_mmap->area[target_area].len;
  uint64_t diff = target_area_end - vmm_start;
  pmem_mmap->area[target_area].len -= diff;

  /* Create a new reserved memory area for vmm */
  pmem_mmap->area[target_area+1].addr = pmem_mmap->area[target_area].addr + pmem_mmap->area[target_area].len;
  pmem_mmap->area[target_area+1].len = diff;
  pmem_mmap->area[target_area+1].type = 2;
  pmem_mmap->nb_area++;
}
