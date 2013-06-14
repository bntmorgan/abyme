#include <efi.h>
#include <efilib.h>

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image, systab);
  uefi_call_wrapper(systab->ConOut->OutputString, 2, systab->ConOut, L"Hello World\n\r");

  // LOL chargement du cr3
  __asm__ __volatile__(
    "mov %cr3, %rax;"
    "mov %rax, %cr3;"
  );

  return EFI_SUCCESS;
}
