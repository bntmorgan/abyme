#ifndef __SMP_H__
#define __SMP_H__

#include <efi.h>
#include "gdt.h"

struct ap_param {
  // 0x00
  struct gdt_ptr_32 gdt_ptr_pm;
  // 0x06
  struct gdt_ptr_64 gdt_ptr_lm;
  // 0x10
  uint32_t cr3_lm;
  // 0x14
  uint64_t vmm_next;
} __attribute__((packed));

void smp_setup(void);
void smp_activate_cores(void);

#endif//__SMP_H__
