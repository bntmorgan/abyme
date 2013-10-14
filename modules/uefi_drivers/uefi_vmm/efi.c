#include <efi.h>
#include <efilib.h>

#include "setup.h"

#include "cpu.h"
#include "cpuid.h"
#include "stdio.h"

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    {0x5B1B31A1, 0x9562, 0x11d2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B }}


EFI_SYSTEM_TABLE *systab;

// EFI_STATUS vmm_rt_unload (IN EFI_HANDLE image); 

// EFI_GUID vmm_driver_id = { 0xcc2ac9d1, 0x14a9, 0x11d3, { 0x8e, 0x77, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b }}; 

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *st) {

/*  EFI_LOADED_IMAGE *image;
  EFI_STATUS status;*/

  // Save the uefi systab
  systab = st;

  // Initialize gnuefi lib
  InitializeLib(image_handle, st);
  INFO("VMM driver startup\n");
  INFO("main at %X\n", efi_main);
  
  // Add an unload handler
  
  /*status = BS->HandleProtocol (image_handle, &LoadedImageProtocol, (void*)&image); 
  ASSERT (!EFI_ERROR(status)); 
  image->Unload = vmm_rt_unload; */

  // Add a protocol so someone can locate us 
  /*status = LibInstallProtocolInterfaces(&image_handle, vmm_driver_id, NULL, NULL); 
  ASSERT (!EFI_ERROR(status));*/

  // Save the current GDT
  //INFO("Current GDT save\n");
  cpuid_setup();
/*  vmem_save_gdt();
  
  vmm_setup();

  vmm_ept_setup(&ept_info);

  vmm_vm_setup_and_launch();
 */ 
  // Install and launch vmm
  vmm_main();
  INFO("CR0 before : %X\n", cpu_read_cr0());
  INFO("CR4 before : %X\n", cpu_read_cr4());

  EFI_GUID li_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
  EFI_LOADED_IMAGE *efi_loaded_image = 0; 
  EFI_STATUS status = uefi_call_wrapper(systab->BootServices->LocateProtocol,
      3,
      &li_guid,
      NULL,
      (void **)&efi_loaded_image);
  if (status == 0) {
    INFO("EFI_LOADED_IMAGE->ImageBase %X\n", efi_loaded_image->ImageBase);
    INFO("EFI_LOADED_IMAGE->ImageSize %X\n", efi_loaded_image->ImageSize);
    INFO("EFI_LOADED_IMAGE->ImageCodeType %X\n", efi_loaded_image->ImageCodeType);
    INFO("EFI_LOADED_IMAGE->ImageDataType %X\n", efi_loaded_image->ImageDataType);
  } else {
    INFO("LOLZ error code %X\n", status);
  }

  return EFI_SUCCESS;
}

/*EFI_STATUS vmm_rt_unload (IN EFI_HANDLE image) {
  INFO("Driver unload :(\n");
  return (EFI_SUCCESS);
}*/

