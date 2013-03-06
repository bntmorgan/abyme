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
uint64_t pmem_get_aligned_memory_at_end_of_free_area(uint32_t size, uint32_t align, uint32_t pages_size) {
  /*
   * Important: all pages used for the allocation must lie all over the same memory area.
   */
  for (uint8_t i = 0; i < pmem_mmap.nb_area; i++) {
    uint8_t j = pmem_mmap.nb_area - i - 1;
    uint64_t area_start_align = pmem_mmap.area[j].addr;
    uint64_t area_end_align = pmem_mmap.area[j].addr + pmem_mmap.area[j].len;

    if (pmem_mmap.area[j].type == 1 && (area_end_align >> 32) == 0) {
      /*
       * The loader is executed in protected mode. The division for 64 bits number
       * is not available. So, we cast on 32 bits number without loss of information.
       */
      uint32_t tmp = ((uint32_t) area_end_align) % pages_size;
      area_end_align = area_end_align - tmp;
      uint64_t value = area_end_align - size;
      tmp = ((uint32_t) value) % align;
      value = value - tmp;
      tmp = ((uint32_t) value) % pages_size;
      value = value - tmp;
      /*
       * With pages_size == 0x1000 and align == 0x1000:
       *          0x1234         0x2000          0x5000        0x5234
       *      area_start_align    value       area_end_align  addr+len
       *    _________v______________v_____..._______v_____________v_______
       *             |              #               #             |
       *             |              #               #             |
       *    _________|______________#_____..._______#_____________|_______
       */
      if (area_start_align <= value) {
        INFO("Copying vmm at %X (end of area: %X)\n", value, area_end_align);
        //__asm__ __volatile__("xchg %bx, %bx");
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
