#ifndef __VMEM_H__
#define __VMEM_H__

#include <stdint.h>

#define VMEM_PDPT_PS_1G 128

typedef struct {
  uint32_t base;
  uint32_t limit;
  uint8_t access;
  uint8_t granularity;
} gdt_entry_t;

typedef struct {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed)) gdt_pm_ptr_t;

typedef struct {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed)) gdt_lm_ptr_t;

typedef struct {
  uint16_t limit0;
  uint16_t base0;
  uint8_t base1;
  uint8_t attr0;
  uint8_t limit1 : 4;
  uint8_t attr1 : 4;
  uint8_t base2;
} __attribute__((packed)) seg_desc_t;

typedef struct {
  uint8_t gdt_desc[5 * 8] __attribute((aligned(8)));
  uint64_t PML4[512] __attribute__((aligned(0x1000)));
  uint64_t PDPT_PML40[512] __attribute__((aligned(0x1000)));
  uint64_t PD_PDPT0_PML40[512] __attribute__((aligned(0x1000)));
} __attribute__((aligned(8))) vmem_info_t;

#endif
