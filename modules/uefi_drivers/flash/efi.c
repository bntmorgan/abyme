#include <efi.h>
#include <efilib.h>
#include "flash.h"

#include "efi/efi_flash.h" 

// 
// stdio putc pointer
//
extern void (*putc)(uint8_t);

void put_nothing(uint8_t c) {}

void disable_debug(void){
  putc = &put_nothing;
}

EFI_STATUS rt_unload (IN EFI_HANDLE image);

protocol_flash proto = {
  0,
  0,
  0,
  flash_readd
};

EFI_GUID guid = EFI_PROTOCOL_FLASH_GUID;

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image_handle, systab);
  EFI_STATUS status;
  EFI_LOADED_IMAGE *image;

  int ret = flash_init(&proto);

  switch (ret) {
    case FLASH_NOT_FOUND:
      status = EFI_NOT_FOUND;
      break;
    case FLASH_TIMEOUT:
      status = EFI_TIMEOUT;
      break;
    case FLASH_UNSUPPORTED:
      status = EFI_UNSUPPORTED;
      break;
    case FLASH_OUT_OF_RESOURCES:
      status = EFI_SUCCESS;
      break;
    case FLASH_ERROR:
      status = EFI_DEVICE_ERROR;
      break;
    default: // FLASH_OK
      status = EFI_SUCCESS;
  }

  // Add an unload handler
  status = uefi_call_wrapper(BS->HandleProtocol, 4, &image_handle,
      &LoadedImageProtocol, (void*)&image);
  ASSERT (!EFI_ERROR(status));
  image->Unload = rt_unload;

  // Add a protocol so someone can locate us
  status = uefi_call_wrapper(BS->InstallProtocolInterface, 4, &image_handle,
      &guid, (EFI_INTERFACE_TYPE)NULL, &proto);
  ASSERT (!EFI_ERROR(status));

  // eric_disable_debug();

  return status;
}

EFI_STATUS rt_unload (IN EFI_HANDLE image) {
  return (EFI_SUCCESS);
}
