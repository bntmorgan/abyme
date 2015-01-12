#ifndef __EPT_H__
#define __EPT_H__

/* TODO: pourquoi les deux ? */
#include <efi.h>
#include "types.h"

enum ept_error {
  EPT_OK,
  EPT_E_BAD_PARAMETER,
};

void ept_create_tables(void);

uint64_t ept_get_eptp(void);

#endif
