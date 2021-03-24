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
#include "nested_vmx.h"
#include "vmcs.h"
#include "vmx.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#ifdef _DEBUG_SERVER
#include "debug_server/debug_server.h"
#endif


uint8_t nested_state = NESTED_DISABLED;
uint8_t guest_vmcs[4096] __attribute((aligned(0x1000)));
static uint8_t* shadow_vmcs_ptr = NULL;

static uint64_t fields_values[NB_VMCS_FIELDS];
#define READ_VMCS_FIELDS(fields) \
  read_vmcs_fields(fields, fields_values, sizeof(fields)/sizeof(uint64_t))
#define WRITE_VMCS_FIELDS(fields) \
  write_vmcs_fields(fields, fields_values, sizeof(fields)/sizeof(uint64_t))

static inline void read_vmcs_fields(uint64_t* fields, uint64_t* values, uint8_t nb_fields);
static inline void write_vmcs_fields(uint64_t* fields, uint64_t* values, uint8_t nb_fields);

void nested_vmxon(uint8_t *vmxon_guest) {
  if (nested_state != NESTED_DISABLED) {
    panic("#!NESTED_VMXON not disabled\n");
  }
  // set guest carry flag to 0
  cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x1);

  // check if vmcs addr is aligned on 4Ko and check rev identifier
  if (((uint64_t)vmxon_guest & 0xfff) != 0){
    panic("#!NESTED_VMXON addr is not 4k aligned\n");
  } else if (*(uint32_t*)vmxon_guest != *(uint32_t*)vmcs0) {
    panic("#!NESTED_VMXON rev identifier is not valid\n");
  }

  // init guest VMCS
  memset((uint8_t *) guest_vmcs, 0, 0x1000);
  *(uint32_t*)guest_vmcs = *(uint32_t*)vmcs0;
  cpu_vmclear(guest_vmcs);

  nested_state = NESTED_HOST_RUNNING;
}

void nested_vmclear(uint8_t *shadow_vmcs) {
  cpu_vmclear(shadow_vmcs);
  // set guest carry and zero flag to 0
  cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x21);
}

void nested_vmptrld(uint8_t *shadow_vmcs) {
  shadow_vmcs_ptr = shadow_vmcs;
  // set guest carry and zero flag to 0
  cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x21);
}

void nested_vmlaunch(void) {
  static uint64_t ctrl_host_fields[] = { NESTED_CTRL_FIELDS, NESTED_HOST_FIELDS };
  uint64_t preempt_timer_value = cpu_vmread(VMX_PREEMPTION_TIMER_VALUE);

  // copy ctrl fields + host fields from vmcs0 to guest_vmcs
  READ_VMCS_FIELDS(ctrl_host_fields);
  cpu_vmptrld(guest_vmcs);
  WRITE_VMCS_FIELDS(ctrl_host_fields);

  // update vmx preemption timer
  cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE, preempt_timer_value);

  nested_state = NESTED_GUEST_RUNNING;
  cpu_vmlaunch();
}

uint64_t nested_vmread(uint64_t field) {
  uint64_t value;

  // Reading ro_data fields is done in guest_vmcs instead of shadow_vmcs
  if (((field >> 10) & 0x3) == 0x1) { // ro_data fields
    cpu_vmptrld(guest_vmcs);
  } else {
    cpu_vmptrld(shadow_vmcs_ptr);
  }

  value = cpu_vmread(field);

  cpu_vmptrld(vmcs0);

  // XXX We assume that everything went good : CF and ZF = 0
  cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x21);

  return value;
}

void nested_vmwrite(uint64_t field, uint64_t value) {
  cpu_vmptrld(shadow_vmcs_ptr);
  cpu_vmwrite(field, value);

  // We forward the modification in guest_vmcs if needed
  if ( (((field >> 10) & 0x3) == 0x2)     // all guest fields
    || ( (((field >> 10) & 0x3) == 0x0)   // several ctrl fields
      && ( (field == VM_ENTRY_CONTROLS)
        || (field == VM_ENTRY_INTR_INFO_FIELD)
        || (field == CR0_READ_SHADOW)
        || (field == CR4_READ_SHADOW)
        || (field == VIRTUAL_PROCESSOR_ID)))) {
    cpu_vmptrld(guest_vmcs);
    cpu_vmwrite(field, value);
  }

  cpu_vmptrld(vmcs0);

  // XXX We assume that everything went good : CF and ZF = 0
  cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x21);
}

void nested_load_host(void) {
  static uint64_t guest_fields[] = { NESTED_GUEST_FIELDS, VM_ENTRY_CONTROLS /* For IA32E_MODE_GUEST */ };
  static uint64_t host_fields[] = { NESTED_HOST_FIELDS };
  static uint64_t host_fields_dest[] = { NESTED_HOST_FIELDS_DEST };
  uint64_t preempt_timer_value = cpu_vmread(VMX_PREEMPTION_TIMER_VALUE);

  // copy all fields updated by vm_exit from guest_vmcs to shadow_vmcs
  READ_VMCS_FIELDS(guest_fields);
  cpu_vmptrld(shadow_vmcs_ptr);
  WRITE_VMCS_FIELDS(guest_fields);

  // restore host fields from shadow_vmcs to vmcs0
  READ_VMCS_FIELDS(host_fields);
  cpu_vmptrld(vmcs0);
  WRITE_VMCS_FIELDS(host_fields_dest);

  // update vmx preemption timer
  cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE, preempt_timer_value);

  nested_state = NESTED_HOST_RUNNING;
}

void nested_load_guest(void) {
  uint64_t preempt_timer_value = cpu_vmread(VMX_PREEMPTION_TIMER_VALUE);

  cpu_vmptrld(guest_vmcs);

  // update vmx preemption timer
  cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE, preempt_timer_value);

  nested_state = NESTED_GUEST_RUNNING;
}

static inline void read_vmcs_fields(uint64_t* fields, uint64_t* values, uint8_t nb_fields) {
  int i;
  for (i = 0; i < nb_fields ; i++) {
    values[i] = cpu_vmread(fields[i]);
  }
}

static inline void write_vmcs_fields(uint64_t* fields, uint64_t* values, uint8_t nb_fields) {
  int i;
  for (i = 0; i < nb_fields ; i++) {
    cpu_vmwrite(fields[i], values[i]);
  }
}
