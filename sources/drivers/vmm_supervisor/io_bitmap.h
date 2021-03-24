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

#ifndef __IO_BITMAP_H__
#define __IO_BITMAP_H__

#include <efi.h>
#include "types.h"

void io_bitmap_setup(void);

void io_bitmap_set_for_port(uint64_t port);

uint64_t io_bitmap_get_ptr_a(void);

uint64_t io_bitmap_get_ptr_b(void);

#endif
