#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "types.h"

/**
 * Fields is the address of a 16 bits fields structure
 */
void dump(void *fields, uint32_t fds, uint32_t fdss, uint32_t offset, uint32_t step);

#endif

