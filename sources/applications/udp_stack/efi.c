#include <efi.h>
#include <efilib.h>
#include "stdio.h"
#include "microudp.h"

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image, systab);
	
	microudp_start(macaddr, 0xc0a80001); 
  INFO("Hello World\n");

  return EFI_SUCCESS;
}
