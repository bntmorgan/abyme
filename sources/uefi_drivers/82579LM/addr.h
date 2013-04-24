#ifndef __ADDR_H__
#define __ADDR_H__

#include <efi.h>

#include "types.h"

typedef struct _eth_addr {
  uint8_t n[6];
} __attribute__((packed)) eth_addr;

#endif
