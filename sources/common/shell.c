#include "shell.h"

#include <efi.h>
#include <efilib.h>

void shell_print(uint8_t value) {
  uint32_t v = value & 0x000000ff;
  uefi_call_wrapper(ST->ConOut->OutputString, 2, ST->ConOut, (uint16_t*)&v);
  if (value == '\n') {
    v = '\r' & 0x000000ff;
    uefi_call_wrapper(ST->ConOut->OutputString, 2, ST->ConOut, (CHAR16*)&v);
  }
}
