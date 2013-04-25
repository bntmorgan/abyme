#include <efi.h>
#include <efilib.h>

#include "82579LM.h"
#include "debug_eth.h"
#include "api.h"
#include "efi/efi_82579LM.h"

//
// Protocol structure definition
//
protocol_82579LM proto = {
  send,
  recv
};

EFI_STATUS vmm_rt_unload (IN EFI_HANDLE image); 
EFI_GUID vmm_driver_id = { 0xcc2ac9d1, 0x14a9, 0x11d3, { 0x8e, 0x77, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b }}; 

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image_handle, systab);
  Print(L"Experimental Intel 82579LM Ethernet driver initialization\n\r");
  if(eth_setup() == -1) {
    return EFI_NOT_FOUND;
  }
  uint32_t i;
  uint16_t len = 0x100;
  uint8_t buf[0x100];
  for (i = 0; i < len ; i++) {
    buf[i] = 0xca;
  }
  uint32_t nb = 16;
  for (i = 0; i < nb; i++) {
    send(buf, len, API_BLOCK);
  }
  debug_print_reg_stat();
  
  // Add an unload handler
  EFI_STATUS status;
  EFI_LOADED_IMAGE *image;
  status = uefi_call_wrapper(BS->HandleProtocol, 4, &image_handle, &LoadedImageProtocol, (void*)&image); 
  ASSERT (!EFI_ERROR(status)); 
  image->Unload = vmm_rt_unload;

  Print(L"Unload handler added %x\n", (uint64_t)image_handle);

  // Add a protocol so someone can locate us 
  status = uefi_call_wrapper(BS->InstallProtocolInterface, 4, &image_handle, vmm_driver_id, NULL, &proto);
  ASSERT (!EFI_ERROR(status));

  return EFI_SUCCESS;
}

EFI_STATUS vmm_rt_unload (IN EFI_HANDLE image) {
  Print(L"Driver unload :(\n");
  return (EFI_SUCCESS);
}
