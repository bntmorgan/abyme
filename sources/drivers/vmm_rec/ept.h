#ifndef __EPT_H__
#define __EPT_H__

/* TODO: pourquoi les deux ? */
#include <efi.h>
#include "types.h"

/**
 * Physical memory under 4 GB is 4k id mapped, other is 2mb
 */
#define EPTN 4

/**
 * < 4GB pool context size
 */
#define CTXN 9

enum ept_error {
  EPT_OK,
  EPT_E_BAD_PARAMETER,
};

void ept_create_tables(void);
void ept_cache(void);
int ept_iterate(uint64_t eptp, int (*cb)(uint64_t *, uint64_t, uint8_t));
void ept_set_ctx(uint8_t c);

uint64_t ept_get_eptp(void);
void ept_perm(uint64_t address, uint32_t pages, uint8_t rights, uint8_t c);
void ept_check_mapping(void);

//
// Paging masks
// And Macros
//

// PML4E
#define EPT_PML4E_P            (3 <<  0)

#endif
