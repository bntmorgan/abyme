#include <efi.h>
#include <efilib.h>

#include "setup.h"

#include "cpu.h"
#include "cpuid.h"
#include "stdio.h"

EFI_SYSTEM_TABLE *systab;

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *st) {

  // Save the uefi systab
  systab = st;

  // Initialize gnuefi lib
  InitializeLib(image_handle, st);
  INFO("VMM driver startup\n");
  INFO("main at %X\n", efi_main);

  // cpuid features
  cpuid_setup();

  // Install smp, install vmm, activate ap cores, launch VM
  bsp_main();

  return EFI_SUCCESS;
}
