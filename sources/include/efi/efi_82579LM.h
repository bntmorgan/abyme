#ifndef __EFI_82579_H__
#define __EFI_82579_H__

#include <efi.h>

#define EFI_82579LM_API_BLOCK (1 << 0)

#define EFI_PROTOCOL_82579LM_GUID {0xcc2ac9d1, 0x14a9, 0x11d3,\
    {0x8e, 0x77, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}; 

typedef struct _protocol_82579LM {
  uint32_t (*send)(const void *, uint32_t, uint8_t);
  uint32_t (*revc)(void *, uint32_t, uint8_t);
} protocol_82579LM;

#endif
