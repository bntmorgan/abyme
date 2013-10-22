#include <efi.h>
#include <efilib.h>

#include "82579LM.h"
#include "debug_eth.h"
#include "api.h"
#include "pci.h"
#include "efi/efi_82579LM.h"

//
// Protocol structure definition
//
protocol_82579LM proto = {
  send,
  recv,
  .pci_addr = {
    0,
    0,
    0
  },
  0,
  ETH_MTU
};

EFI_STATUS vmm_rt_unload (IN EFI_HANDLE image); 
EFI_GUID guid_82579LM = EFI_PROTOCOL_82579LM_GUID; 

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image_handle, systab);
  Print(L"Experimental Intel 82579LM Ethernet driver initialization\n\r");
  if (eth_setup() == -1) {
    return EFI_NOT_FOUND;
  }
  test_send();

  // Add an unload handler
  EFI_STATUS status;
  EFI_LOADED_IMAGE *image;
  status = uefi_call_wrapper(BS->HandleProtocol, 4, &image_handle, &LoadedImageProtocol, (void*)&image); 
  ASSERT (!EFI_ERROR(status)); 
  image->Unload = vmm_rt_unload;

  Print(L"Unload handler added %x\n", (uint64_t)image_handle);

  // Share the pci address in the uefi protocol
  pci_device_addr *pci_addr = eth_get_pci_addr();
  proto.pci_addr.bus = pci_addr->bus;
  proto.pci_addr.device = pci_addr->device;
  proto.pci_addr.function = pci_addr->function;
  proto.bar0 = (uint64_t)bar0;

  // Add a protocol so someone can locate us 
  status = uefi_call_wrapper(BS->InstallProtocolInterface, 4, &image_handle, &guid_82579LM, NULL, &proto);
  ASSERT (!EFI_ERROR(status));

  return status;
}

EFI_STATUS vmm_rt_unload (IN EFI_HANDLE image) {
  Print(L"Driver unload :(\n");
  return (EFI_SUCCESS);
}
