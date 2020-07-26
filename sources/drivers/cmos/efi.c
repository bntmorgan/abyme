#include <efi.h>
#include <efilib.h>

#include "debug.h"
#include "stdio.h"
#include "shell.h"

// 
// stdio putc pointer
//
extern void (*putc)(uint8_t);

void put_nothing(uint8_t c) {}

void disable_debug(void){
  putc = &put_nothing;
}

void read_from_cmos (unsigned char data []) {
   unsigned char tvalue, index;
 
   for(index = 0; index < 128; index++) {
      __asm__ __volatile__(
         "cli;"             /* Disable interrupts*/
         /* since the 0x80 bit of al is not set, NMI is active */
         "out %%al, $0x70;"     /* Copy address to CMOS register*/
         /* some kind of real delay here is probably best */
         "in $0x71, %%al;"      /* Fetch 1 byte to al*/
         "sti;"             /* Enable interrupts*/
      : "=a"(tvalue) : "a"(index));
 
       data[index] = tvalue;
   }
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image_handle, systab);

  // Print to shell
  putc = &shell_print;

  EFI_STATUS status = EFI_SUCCESS;

  unsigned char data[128];
  int i;

  read_from_cmos(data);

  for (i = 0; i < 32; i += 4) {
    INFO("%02x%02x %02x%02x\n", data[i], data[i + 1], data[i + 2], data[i + 3]);
  }

  INFO("Experimental CMOS driver initialization\n");

  return status;
}
