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
