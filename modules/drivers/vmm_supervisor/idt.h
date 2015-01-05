#ifndef __IDT_H__
#define __IDT_H__

#include "types.h"

struct idt_ptr {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed));

#endif
