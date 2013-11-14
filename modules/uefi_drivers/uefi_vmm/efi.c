#include <efi.h>
#include <efilib.h>

#include "setup.h"

#include "cpu.h"
#include "cpuid.h"
#include "stdio.h"
#include "paging.h"

EFI_SYSTEM_TABLE *systab;

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *st) {

  // Save the uefi systab
  systab = st;

  // Initialize gnuefi lib
  InitializeLib(image_handle, st);
  INFO("VMM driver startup\n");
  INFO("main at %X\n", efi_main);

  INFO("Walk test\n");
  uint64_t cr3 = cpu_read_cr3();
  uint64_t linear = 0xf8000000;
  uint64_t e = 0;
  uint64_t a = 0;

  
  if (paging_walk(cr3, linear, &e, &a)) {
    INFO("ERROR walking address\n");
  } else {
    INFO("Virtual 0x%X is walked at 0x%X by the entry 0x%X\n", linear, a, e);
  }

  // Install smp, install vmm, activate ap cores, launch VM
  // bsp_main();

  return EFI_SUCCESS;
}
