#ifndef __MOD_H__
#define __MOD_H__

#include <stdint.h>

typedef struct {
  uint32_t mod_start;
  uint32_t mod_end;
  char cmdline[32];
} mod_info_t;

#endif
