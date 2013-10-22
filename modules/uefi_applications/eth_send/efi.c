#include <efi.h>
#include <efilib.h>

#include "efi/efi_82579LM.h"

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image, systab);
  EFI_STATUS status;
  protocol_82579LM *eth;
  EFI_GUID guid_82579LM = EFI_PROTOCOL_82579LM_GUID; 

  status = LibLocateProtocol(&guid_82579LM, (void **)&eth);
  if (EFI_ERROR(status)) {
    Print(L"FAILED LOL LocateProtocol\n");
    return -1;
  }

  uint32_t i;
  uint16_t len = 0x100;
  uint8_t buf[0x100];
  for (i = 0; i < len ; i++) {
    buf[i] = 0xca;
  }

  // TODO Pourquoi uefi_call_wrapper ne marche pas ??
  //uefi_call_wrapper(eth->send, 3, buf, len, 0);
  eth->send(buf, len, 0);

  Print(L"Pointeur du service eth %x\n", eth);

  return EFI_SUCCESS;
}
