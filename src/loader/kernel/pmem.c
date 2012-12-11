#include "pmem.h"

#include "stdio.h"
#include "string.h"

pmem_mmap_t pmem_mmap;

void pmem_setup(multiboot_info_t *multiboot_info) {
  if (multiboot_info->mmap_length > sizeof(pmem_mmap)) {
    ERROR("too many information in the memory mapping\n");
  }
  /*
   * Copy memory mapping information.
   */
  uint8_t *multiboot_mmap_end = ((uint8_t *) multiboot_info->mmap_addr) + multiboot_info->mmap_length;
  multiboot_memory_map_t *multiboot_mmap_ptr = (multiboot_memory_map_t *) multiboot_info->mmap_addr;
  pmem_mmap.nb_area = 0;
  while ((uint8_t *) multiboot_mmap_ptr < multiboot_mmap_end) {
    pmem_mmap.area[pmem_mmap.nb_area].size = multiboot_mmap_ptr->size;
    pmem_mmap.area[pmem_mmap.nb_area].addr = multiboot_mmap_ptr->addr;
    pmem_mmap.area[pmem_mmap.nb_area].len = multiboot_mmap_ptr->len;
    pmem_mmap.area[pmem_mmap.nb_area].type = multiboot_mmap_ptr->type;
    pmem_mmap.nb_area = pmem_mmap.nb_area + 1;
    multiboot_mmap_ptr = (multiboot_memory_map_t *) (((uint8_t *) multiboot_mmap_ptr)
        + multiboot_mmap_ptr->size + sizeof(multiboot_mmap_ptr->size));
  }
}

/*
 * Return the address of the end of the last usable memory area, large enough for
 * size bytes. This address must be aligned. Return 0 if none.
 */
uint64_t pmem_get_aligned_memory_at_end_of_free_area(uint32_t size, uint32_t alignment) {
  for (uint8_t i = 0; i < pmem_mmap.nb_area; i++) {
    uint8_t j = pmem_mmap.nb_area - i - 1;
    if ((pmem_mmap.area[j].type == 1) && (pmem_mmap.area[j].len > size)) {
      uint64_t value = ~((((uint64_t) 1) << alignment) - 1);
      value = ((uint64_t) (pmem_mmap.area[j].addr + pmem_mmap.area[j].len - size)) & value;
      if (value > pmem_mmap.area[j].addr) {
        return value;
      }
    }
  }
  return 0;
}

void pmem_copy_info(pmem_mmap_t *dest) {
  INFO("copy from %08x to %08x\n", &pmem_mmap, dest);
  memcpy(dest, &pmem_mmap, sizeof(pmem_mmap));
}

void pmem_print_info(multiboot_info_t *multiboot_info) {
  INFO("mmap: at %08x with a size of %08x bytes and %d sections\n",
      (unsigned int) multiboot_info->mmap_addr,
      (unsigned int) multiboot_info->mmap_length,
      pmem_mmap.nb_area);
  for (uint8_t i = 0; i < pmem_mmap.nb_area; i++) {
    INFO("size=%2x addr=%08x len=%08x type=%01x\n",
        (uint32_t) pmem_mmap.area[i].size,
        (uint32_t) pmem_mmap.area[i].addr,
        (uint32_t) pmem_mmap.area[i].len,
        (uint32_t) pmem_mmap.area[i].type);
  }
}
