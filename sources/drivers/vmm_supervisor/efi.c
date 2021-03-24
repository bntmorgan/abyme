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

#include "setup.h"

#include "cpu.h"
#include "cpuid.h"
#include "stdio.h"
#include "paging.h"
#include "efiw.h"

uint64_t vm_RIP;
uint64_t vm_RSP;
uint64_t vm_RBP;

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *st) {

  // Initialize gnuefi lib
  InitializeLib(image_handle, st);

  // desactivate interruptions
  __asm__ __volatile__("cli");

  INFO("VMM driver startup\n");
  INFO("main at %X\n", efi_main);

  // Set VM RIP, RSP and RBP
  vm_RIP = (uint64_t)&&vm_entrypoint;
  __asm__ __volatile__("mov %%rsp, %0" : "=m" (vm_RSP));
  __asm__ __volatile__("mov %%rbp, %0" : "=m" (vm_RBP));

  // Install smp, install vmm, activate ap cores, launch VM
  bsp_main();

  // do not write anything after this line,
  // this is where the vm begin his exectution
vm_entrypoint:

  return EFI_SUCCESS;
}
