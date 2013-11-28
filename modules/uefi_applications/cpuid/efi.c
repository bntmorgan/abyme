#include <efi.h>
#include <efilib.h>

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab) {
  InitializeLib(image, systab);

	uint64_t v;
	uint64_t b;
	uint64_t c;
	uint64_t d;
	v = 0x99999999;
	Print(L"v=%lx\n", v);
	__asm__ __volatile__("cpuid" : "=a"(v), "=b"(b), "=c"(c), "=d"(d) : "a"(v));
	Print(L"v=%lx\n", v);
	Print(L"b=%lx\n", b);
	Print(L"c=%lx\n", c);
	Print(L"d=%lx\n", d);
  Print(L"%4s%4s%4s\n", (char *)&b, (char *)&d, (char *)&c);

  return EFI_SUCCESS;
}
