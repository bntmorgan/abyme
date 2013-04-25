#ifndef __EFI_82579_H__
#define __EFI_82579_H__

#include <efi.h>

typedef struct _protocol_82579LM {
  uint32_t (*send)(const void *, uint32_t, uint8_t);
  uint32_t (*revc)(void *, uint32_t, uint8_t);
} protocol_82579LM;

#endif
