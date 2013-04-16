#ifndef __PAGING_H__
#define __PAGING_H__

#include <efi.h>
#include "types.h"

void paging_setup_host_paging(void);

uint64_t paging_get_host_cr3(void);

#endif
