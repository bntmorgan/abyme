#ifndef __EPT_H__
#define __EPT_H__

/* TODO: pourquoi les deux ? */
#include <efi.h>
#include "types.h"

void ept_create_tables(void);

uint64_t ept_get_eptp(void);

#endif
