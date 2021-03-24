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

#ifndef __FLASH_H__
#define __FLASH_H__

#include "stdint.h"
#include "efi/efi_flash.h" 

enum flash_codes {
  FLASH_OK,
  FLASH_NOT_FOUND,
  FLASH_TIMEOUT,
  FLASH_UNSUPPORTED,
  FLASH_OUT_OF_RESOURCES,
  FLASH_ERROR
};

enum bbs_type {
  LPC,
  RES,
  PCI,
  SPI
};

#define FLASH_READ_SIZE 64

int flash_init(protocol_flash *proto);
int flash_read_block(uint32_t addr, uint32_t size, uint32_t *buf);
int flash_readd(uint32_t addr, uint32_t *buf);
void flash_cache_invalidate(void);

#endif//__FLASH_H__
