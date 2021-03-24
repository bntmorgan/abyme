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

#ifndef __EFI_FLASH_H__
#define __EFI_FLASH_H__

#include <efi.h>

#define EFI_PROTOCOL_FLASH_GUID {0xcc2ac9d1, 0x14a9, 0x11d3,\
    {0x8e, 0x77, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3d}};

typedef struct _protocol_flash {
  unsigned char *init_flash;
  unsigned int bios_base;
  unsigned int bios_limit;
  int (*readd) (unsigned int addr, unsigned int *buf);
  void (*cache_invalidate) (void);
} protocol_flash;

#endif
