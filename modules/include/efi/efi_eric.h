#ifndef __EFI_ERIC_H__
#define __EFI_ERIC_H__

#include <efi.h>

#define EFI_PROTOCOL_ERIC_GUID {0xcc2ac9d1, 0x14a9, 0x11d3,\
    {0x8e, 0x77, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3c}}; 

typedef struct _protocol_eric {
  struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
  } pci_addr;
  uint64_t bar0;
  uint64_t bar1;
  uint64_t bar2;
  uint64_t rom;
} protocol_eric;

#endif
