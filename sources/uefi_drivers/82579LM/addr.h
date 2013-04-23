#ifndef __ADDR_H__
#define __ADDR_H__

#include <efi.h>

#include "types.h"

typedef struct _eth_addr {
  uint8_t n[6];
} __attribute__((packed)) eth_addr;

#define ETH_ADDR_STRING_SIZE            18
typedef struct _ipv4_addr {
  union {
    uint8_t n[4];
    uint32_t bits;
  } u;
}  __attribute__((packed)) ipv4_addr;

#endif
