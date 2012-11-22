#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <stdint.h>

typedef struct {
  uint64_t kernel_physical_start;
} kernel_info_t;

#endif
