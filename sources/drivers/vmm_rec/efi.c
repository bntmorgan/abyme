#include <efi.h>
#include <efilib.h>

#include "setup.h"

#include "cpu.h"
#include "cpuid.h"
#include "stdio.h"
#include "paging.h"

extern uint8_t _protected_begin;
extern uint8_t _protected_end;

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *st) {
  uint64_t vm_RIP;
  uint64_t vm_RSP;
  uint64_t vm_RBP;

  // Initialize gnuefi lib
  InitializeLib(image_handle, st);

  // Set VM RIP, RSP and RBP
  vm_RIP = (uint64_t)&&vm_entrypoint;
  __asm__ __volatile__("mov %%rsp, %0" : "=m" (vm_RSP));
  __asm__ __volatile__("mov %%rbp, %0" : "=m" (vm_RBP));

  // Create setup state
  struct setup_state state = {
    (uint64_t) &_protected_begin,
    (uint64_t) &_protected_end,
    vm_RIP,
    vm_RSP,
    vm_RBP
  };

  // Install smp, install vmm, activate ap cores, launch VM
  bsp_main(&state);

  // do not write anything after this line,
  // this is where the vm begin his exectution
vm_entrypoint:

  return EFI_SUCCESS;
}
