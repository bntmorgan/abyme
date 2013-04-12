#ifndef __INFO_H__
#define __INFO_H__

#include <efi.h>
#include "types.h"

/*
typedef union {
  uint64_t q;
  struct {
    uint64_t b0:8;
    uint64_t b1:8;
    uint64_t b2:8;
    uint64_t b3:8;
    uint64_t b4:8;
    uint64_t b5:8;
    uint64_t b6:8;
    uint64_t b7:8;
  };
} mtrr_qw;

typedef struct {
  mtrr_qw fix64K_00000;
  mtrr_qw fix16K_80000;
  mtrr_qw fix16K_A0000;
  mtrr_qw fix4K_C0000;
  mtrr_qw fix4K_C8000;
  mtrr_qw fix4K_D0000;
  mtrr_qw fix4K_D8000;
  mtrr_qw fix4K_E0000;
  mtrr_qw fix4K_E8000;
  mtrr_qw fix4K_F0000;
  mtrr_qw fix4K_F8000;
} __attribute__((packed)) mtrr_fixed_t;
*/

typedef struct {
  uint32_t size;
  uint64_t addr;
  uint64_t len;
  uint32_t type;
} __attribute__((packed)) pmem_area_t;

typedef struct {
  pmem_area_t area[32];
  uint32_t nb_area;
} __attribute__((packed)) pmem_mmap_t;

#define VMEM_PDPT_PS_1G 128
#define VMEM_PDPT_PS_2M 128

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
  uint8_t gdt_desc[5 * 8]      __attribute((aligned(8)));
  uint64_t PML4[512]           __attribute__((aligned(0x1000)));
  uint64_t PDPT_PML40[512]     __attribute__((aligned(0x1000)));
  uint64_t PD_PDPT_PML40[512][512] __attribute__((aligned(0x1000)));
} __attribute__((aligned(8))) vmem_info_t;

#define VMM_STACK_SIZE 0x400000

typedef struct {
  uint64_t PML4[512]           __attribute__((aligned(0x1000)));
  uint64_t PDPT_PML40[512]     __attribute__((aligned(0x1000)));
  uint64_t PD_PDPT_PML40[512][512] __attribute__((aligned(0x1000)));
  uint64_t PT_PD0[512] __attribute__((aligned(0x1000)));
} __attribute__((aligned(8))) ept_info_t;

typedef struct {
  pmem_mmap_t pmem_mmap;
  vmem_info_t vmem_info;
  ept_info_t ept_info;
  uint32_t vmm_physical_start;
  uint32_t vmm_physical_end;
  uint32_t vmm_stack;
  uint32_t rm_kernel_start;
  uint32_t rm_kernel_size;
} vmm_info_t;

#endif
