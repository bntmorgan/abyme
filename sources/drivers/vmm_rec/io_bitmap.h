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

extern uint8_t (*io_bitmap_a_pool)[0x1000];
extern uint8_t (*io_bitmap_b_pool)[0x1000];

extern uint8_t *io_bitmap_a_shadow;
extern uint8_t *io_bitmap_b_shadow;

void io_bitmap_setup(void);

void io_bitmap_set_for_port(uint64_t port);

void io_bitmap_clone_a_b(uint8_t *a, uint8_t *b);

void io_bitmap_or(uint8_t *a_dst, uint8_t *a_src, uint8_t *b_dst,
    uint8_t *b_src);

int io_bitmap_host_redirect(uint64_t port);

#endif
