/*
Copyright (C) 2021  Benoît Morgan

This file is part of abyme

abyme is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

abyme is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with abyme.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <efi.h>
#include "types.h"

void itoa(int8_t *dst, uint8_t base, int32_t value);
//uint64_t atoi_hexa(char *s);
//uint64_t pow(uint64_t number, uint64_t p);

#define MIN(X,Y) (((X)<(Y))?(X):(Y))

#endif
