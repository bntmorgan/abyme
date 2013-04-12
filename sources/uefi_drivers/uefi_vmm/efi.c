#include <efi.h>
#include <efilib.h>

#include "vmm_setup.h"

#include "cpu.h"
#include "cpuid.h"
#include "stdio.h"

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

  return EFI_SUCCESS;
}

/*EFI_STATUS vmm_rt_unload (IN EFI_HANDLE image) {
  INFO("Driver unload :(\n");
  return (EFI_SUCCESS);
}*/

