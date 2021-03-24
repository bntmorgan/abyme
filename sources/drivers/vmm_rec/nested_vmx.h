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

#ifndef __NESTED_VMX_H__
#define __NESTED_VMX_H__

#include "vmm.h"
#include "ept.h"

enum NESTED_STATE {
  NESTED_DISABLED,
  NESTED_HOST_RUNNING,
  NESTED_GUEST_RUNNING
};

#define NESTED_READ_ONLY_DATA_FIELDS \
          GUEST_PHYSICAL_ADDRESS, \
          GUEST_PHYSICAL_ADDRESS_HIGH, \
          VM_INSTRUCTION_ERROR, \
          VM_EXIT_REASON, \
          VM_EXIT_INTR_INFO, \
          VM_EXIT_INTR_ERROR_CODE, \
          IDT_VECTORING_INFO_FIELD, \
          IDT_VECTORING_ERROR_CODE, \
          VM_EXIT_INSTRUCTION_LEN, \
          VMX_INSTRUCTION_INFO, \
          EXIT_QUALIFICATION, \
          IO_RCX, \
          IO_RSI, \
          IO_RDI, \
          IO_RIP, \
          GUEST_LINEAR_ADDRESS

extern uint8_t level;

void nested_recover_state(void);

void nested_vmxon(uint8_t *vmxon_guest);

void nested_vmclear(uint8_t *shadow_vmcs);

void nested_guest_vmptrld(uint8_t *shadow_vmcs, struct registers *gr);

void nested_vmptrld(uint8_t *shadow_vmcs, struct registers *gr);

void nested_vmlaunch(struct registers *guest_regs);

void nested_vmresume(struct registers *guest_regs);

void nested_vmxoff(struct registers *guest_regs);

uint64_t nested_vmread(uint64_t field);

void nested_vmwrite(uint64_t field, uint64_t value);

void nested_load_guest(void);

void nested_load_host(void);

void nested_vmx_shadow_bitmap_init(void);

void nested_vmptrst(uint8_t **shadow_vmcs, struct registers *gr);

#endif
