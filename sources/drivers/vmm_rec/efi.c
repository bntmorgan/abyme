#include <efi.h>
#include <efilib.h>

#include "setup.h"

#include "cpu.h"
#include "idt.h"
#include "cpuid.h"
#include "stdio.h"
#include "paging.h"
#include "debug.h"

extern uint8_t _protected_begin;
extern uint8_t _protected_end;

// Create setup state
struct setup_state state;

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *st) {

  // Initialize gnuefi lib
  InitializeLib(image_handle, st);

  // XXX stdio putc pointer to QEMU
  extern void (*putc)(uint8_t);
  putc = &qemu_putc;

  qemu_send_address("vmm_rec_none.efi");

  // Set VM RIP, RSP and RBP
  state.vm_RIP = (uint64_t)&&vm_entrypoint;
  __asm__ __volatile__("mov %%rsp, %0" : "=m" (state.vm_RSP));
  __asm__ __volatile__("mov %%rbp, %0" : "=m" (state.vm_RBP));
  state.protected_begin = (uint64_t) &_protected_begin;
  state.protected_end = (uint64_t) &_protected_end;
  state.cr3 = cpu_read_cr3();
  cpu_read_idt(&state.idt_ptr);

  // Install smp, install vmm, activate ap cores, launch VM
  bsp_main(&state);

  // do not write anything after this line,
  // this is where the vm begin his exectution
vm_entrypoint:

  return EFI_SUCCESS;
}
