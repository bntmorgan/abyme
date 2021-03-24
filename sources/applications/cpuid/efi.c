/*
Copyright (C) 2021  Benoît Morgan

This file is part of abyme

abyme is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

abyme is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with abyme.  If not, see <http://www.gnu.org/licenses/>.
*/

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
