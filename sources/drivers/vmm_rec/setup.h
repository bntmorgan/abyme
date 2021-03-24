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

#ifndef __VMM_SETUP_H__
#define __VMM_SETUP_H__

#include "idt.h"
#include "vmx.h"
#include "gdt.h"

#define REBOOT \
  cpu_vmxoff(); \
  __asm__ __volatile__("pushw %%ax" : : \
      "a"(gdt_legacy_segment_descriptor << 3)); \
  setup_reboot(); \
  __asm__ __volatile__("add 0x2, %rsp");

struct setup_state {
  uint64_t protected_begin;
  uint64_t protected_end;
  uint64_t vm_RIP;
  uint64_t vm_RSP;
  uint64_t vm_RBP;
  uint64_t cr3;
  struct idt_ptr idt_ptr;
};

extern struct setup_state *setup_state;

void bsp_main(struct setup_state *state);

void vmm_setup(void);

void vmm_create_vmxon_region(void);

void vmm_vm_setup_and_launch();

extern void (*setup_reboot)(void);

#endif
