#include "efiapi.h"

#define DEBUG

EFI_S_T systab;

extern void efi_call3(void *a, void *b, void *c);
//extern void efi_call2(void *a, void *b, void *c);

void efi_output_string(uint16_t *string) {
  systab.OutputString(systab.ConOut, string);
}

void efi_install_systab(void *image, uint8_t *system_table) {
  systab.image = image;
  systab.ConOut = *((uint8_t **)(system_table + 0x40)); 
  uint8_t **bs = (uint8_t **) (system_table + 0x60);
  systab.LocateProtocol = (EFI_LOCATE_PROTOCOL) (*bs + 0x140);
  systab.InstallProtocolInterface = (EFI_INSTALL_PROTOCOL_INTERFACE) (*bs + 0x80);
  systab.OutputString = (EFI_TEXT_OUTPUT_STRING) (*(uint8_t **)(systab.ConOut + 0x08));
}

EFI_STATUS efi_main(EFI_HANDLE image, void *system_table) {
  efi_install_systab(image, (uint8_t *)system_table);
  efi_output_string(L"YOLO_VERSION\n");
  return 0;
}

#ifdef DEBUG
int main() {return 0;}
#endif
