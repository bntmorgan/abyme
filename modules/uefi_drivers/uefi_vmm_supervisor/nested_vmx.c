#include <efi.h>
#include <efilib.h>
#include "nested_vmx.h"
#include "vmcs.h"
#include "vmx.h"
#include "stdio.h"
#include "debug.h"
#ifdef _DEBUG_SERVER
#include "debug_server/debug_server.h"
#endif

uint8_t nested_state = NESTED_DISABLED;

void nested_vmxon(void) {
  // set guest carry flag to 0
  cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x1);
  // TODO : check if vmcs addr is aligned on 4Ko and check rev identifier ...
}

void nested_vmclear(uint8_t *shadow_vmcs) {
  cpu_vmclear(shadow_vmcs);
  // we set the vmcs shadow bit
  *((uint64_t*)shadow_vmcs) |= ((uint64_t)1 << 31);
  // set guest carry and zero flag to 0
  cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x21);
}

void nested_vmptrld(uint8_t *shadow_vmcs) {
  // set the vmcs shadowing bit
  cpu_vmwrite(SECONDARY_VM_EXEC_CONTROL, cpu_vmread(SECONDARY_VM_EXEC_CONTROL) | VMCS_SHADOWING);
  // write the new @ to VMCS link pointer
  cpu_vmwrite(VMCS_LINK_POINTER, (uint64_t)shadow_vmcs & 0xffffffff);
  cpu_vmwrite(VMCS_LINK_POINTER_HIGH, ((uint64_t)shadow_vmcs >> 32) & 0xffffffff);
  // set guest carry and zero flag to 0
  cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x21);
}

/*
 * copy shadow vmcs guest fields to our vmcs
 * nested_state -> NESTED_GUEST_RUNNING
 */
void nested_copy_guest_fields(void) {
  static uint64_t guest_fields_encoding[] = { NESTED_GUEST_FIELDS };
  static uint64_t guest_fields_value[sizeof(guest_fields_encoding)/sizeof(uint64_t)] = { 0 };
  int i;

  nested_load_shadow_vmcs();

  for(i=0; i < sizeof(guest_fields_encoding)/sizeof(uint64_t); i++) {
    guest_fields_value[i]=cpu_vmread(guest_fields_encoding[i]);
  }

  cpu_vmptrld(vmcs0);
  for(i=0; i < sizeof(guest_fields_encoding)/sizeof(uint64_t); i++) {
    cpu_vmwrite(guest_fields_encoding[i], guest_fields_value[i]);
  }

  nested_state = NESTED_GUEST_RUNNING;
}

/*
 * copy shadow vmcs host fields to our vmcs in guest fields
 * nested_state -> NESTED_HOST_RUNNING
 */
void nested_copy_host_fields(void) {
  static uint64_t host_fields_encoding[] = { NESTED_HOST_FIELDS };
  static uint64_t host_fields_encoding_dest[] = { NESTED_HOST_FIELDS_DEST };
  static uint64_t host_fields_value[sizeof(host_fields_encoding)/sizeof(uint64_t)] = { 0 };
  int i;

  nested_load_shadow_vmcs();

  for(i=0; i < sizeof(host_fields_encoding)/sizeof(uint64_t); i++) {
    host_fields_value[i]=cpu_vmread(host_fields_encoding[i]);
  }

  cpu_vmptrld(vmcs0);
  for(i=0; i < sizeof(host_fields_encoding)/sizeof(uint64_t); i++) {
    cpu_vmwrite(host_fields_encoding_dest[i], host_fields_value[i]);
  }

  /* Then we need to update guest fields
   * that are not stored into shadow vmcs host fields */

  //XXX host L2 is always in IA32e mode
  cpu_vmwrite(VM_ENTRY_CONTROLS, cpu_vmread(VM_ENTRY_CONTROLS) | IA32E_MODE_GUEST);
  // We reinitialize RFLAGS (note : bit9 (IF) is desactivated in order to not be interrupted in Host L2)
  cpu_vmwrite(GUEST_RFLAGS, (uint64_t)(1 << 21)  // enable cpuid support
                          | (uint64_t)(1 << 1)); // reserved 1

  nested_state = NESTED_HOST_RUNNING;
}

/*
 * retrieve all information updated by the vmexit and forward them to the shadow vmcs
 */
void nested_forward_exit_infos(void) {
  static uint64_t exit_infos_encoding[] = { NESTED_READ_ONLY_DATA_FIELDS, NESTED_GUEST_FIELDS };
  static uint64_t exit_infos_values[sizeof(exit_infos_encoding)/sizeof(uint64_t)] = { 0 };
  int i;

  for(i=0; i < sizeof(exit_infos_encoding)/sizeof(uint64_t); i++) {
    exit_infos_values[i]=cpu_vmread(exit_infos_encoding[i]);
  }

  nested_load_shadow_vmcs();
  for(i=0; i < sizeof(exit_infos_encoding)/sizeof(uint64_t); i++) {
    cpu_vmwrite(exit_infos_encoding[i], exit_infos_values[i]);
  }

  cpu_vmptrld(vmcs0);
}

/*
 * Load the shadow vmcs as the current vmcs
 */
inline void nested_load_shadow_vmcs(void) {
  // on charge la vmcs du guest
  uint8_t *vmcs_shadow = (uint8_t*)((cpu_vmread(VMCS_LINK_POINTER_HIGH) << 32) | cpu_vmread(VMCS_LINK_POINTER));
  if ((uint64_t)vmcs_shadow == (uint64_t)-1) {
    panic("VMCS_LINK_POINTER is not initialized !\n");
  }
  cpu_vmptrld(vmcs_shadow);
}
