#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <efi.h>
#include "types.h"

void itoa(int8_t *dst, uint8_t base, int32_t value);
//uint64_t atoi_hexa(char *s);
//uint64_t pow(uint64_t number, uint64_t p);

#define MIN(X,Y) (((X)<(Y))?(X):(Y))

#endif
