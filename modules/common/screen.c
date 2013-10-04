#include "screen.h"
#include "systab.h"

#include <efi.h>

void screen_print(uint8_t value) {
  uint32_t v = value & 0x000000ff;
  uefi_call_wrapper(systab->ConOut->OutputString, 2, systab->ConOut, (uint16_t*)&v);
  if (value == '\n') {
    v = '\r' & 0x000000ff;
    uefi_call_wrapper(systab->ConOut->OutputString, 2, systab->ConOut, &v);
  }
}
