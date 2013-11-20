#include <efi.h>
#include <efilib.h>

#include "setup.h"

#include "cpu.h"
#include "cpuid.h"
#include "stdio.h"
#include "paging.h"
#include "efiw.h"

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *st) {
  // Initialize gnuefi lib
  InitializeLib(image_handle, st);
  
  paging_ia32e = efi_allocate_pool(sizeof(struct paging_ia32e));
  INFO("Pointer allocated 0x%016X\n", (uintptr_t)&paging_ia32e);

  INFO("VMM driver startup\n");
  INFO("main at %X\n", efi_main);

  // Install smp, install vmm, activate ap cores, launch VM
  bsp_main();

  return EFI_SUCCESS;
}
