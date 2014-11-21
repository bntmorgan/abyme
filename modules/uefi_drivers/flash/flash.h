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
