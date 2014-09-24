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

static uint64_t fields_values[NB_VMCS_FIELDS];
#define READ_VMCS_FIELDS(fields) \
  read_vmcs_fields(fields, fields_values, sizeof(fields)/sizeof(uint64_t))
#define WRITE_VMCS_FIELDS(fields) \
  write_vmcs_fields(fields, fields_values, sizeof(fields)/sizeof(uint64_t))

static inline void load_shadow_vmcs(void);
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

  nested_state = NESTED_HOST_RUNNING;
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

void nested_vmlaunch(void) {
  static uint64_t ctrl_host_fields[] = { NESTED_CTRL_FIELDS, NESTED_HOST_FIELDS };
  static uint64_t guest_fields[] = { NESTED_GUEST_FIELDS, VIRTUAL_PROCESSOR_ID };
  uint64_t preempt_timer_value = cpu_vmread(VMX_PREEMPTION_TIMER_VALUE);

  // init guest VMCS
  memset((uint8_t *) guest_vmcs, 0, 0x1000);
  *(uint32_t*)guest_vmcs = *(uint32_t*)vmcs0;
  cpu_vmclear(guest_vmcs);

  // copy ctrl fields + host fields from vmcs0 to guest_vmcs
  READ_VMCS_FIELDS(ctrl_host_fields);
  cpu_vmptrld(guest_vmcs);
  WRITE_VMCS_FIELDS(ctrl_host_fields);

  // copy guest fields from shadow_vmcs to guest_vmcs
  load_shadow_vmcs();
  READ_VMCS_FIELDS(guest_fields);
  cpu_vmptrld(guest_vmcs);
  WRITE_VMCS_FIELDS(guest_fields);

  // remove vmcs shadowing in guest_vmcs
  cpu_vmwrite(VMCS_LINK_POINTER, 0xffffffff);
  cpu_vmwrite(VMCS_LINK_POINTER_HIGH, 0xffffffff);
  cpu_vmwrite(SECONDARY_VM_EXEC_CONTROL, cpu_vmread(SECONDARY_VM_EXEC_CONTROL) & ~(uint64_t)VMCS_SHADOWING);

  // update vmx preemption timer
  cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE, preempt_timer_value);

  nested_state = NESTED_GUEST_RUNNING;
  cpu_vmlaunch();
}

void nested_load_host(void) {
  static uint64_t rodata_guest_fields[] = { NESTED_READ_ONLY_DATA_FIELDS, NESTED_GUEST_FIELDS, VM_ENTRY_CONTROLS /* For IA32E_MODE_GUEST */ };
  static uint64_t host_fields[] = { NESTED_HOST_FIELDS };
  static uint64_t host_fields_dest[] = { NESTED_HOST_FIELDS_DEST };
  uint64_t preempt_timer_value = cpu_vmread(VMX_PREEMPTION_TIMER_VALUE);

  // copy all fields updated by vm_exit from guest_vmcs to shadow_vmcs
  READ_VMCS_FIELDS(rodata_guest_fields);
  load_shadow_vmcs();
  WRITE_VMCS_FIELDS(rodata_guest_fields);

  // restore host fields from shadow_vmcs to vmcs0
  READ_VMCS_FIELDS(host_fields);
  cpu_vmptrld(vmcs0);
  WRITE_VMCS_FIELDS(host_fields_dest);

  // update vmx preemption timer
  cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE, preempt_timer_value);

  nested_state = NESTED_HOST_RUNNING;
}

void nested_load_guest(void) {
  static uint64_t guest_fields[] = { NESTED_GUEST_FIELDS, VM_ENTRY_CONTROLS, VM_ENTRY_INTR_INFO_FIELD, CR0_READ_SHADOW, CR4_READ_SHADOW };
  uint64_t preempt_timer_value = cpu_vmread(VMX_PREEMPTION_TIMER_VALUE);

  // copy fields that could have been modified by L2 from shadow_vmcs to guest_vmcs
  load_shadow_vmcs();
  READ_VMCS_FIELDS(guest_fields);
  cpu_vmptrld(guest_vmcs);
  WRITE_VMCS_FIELDS(guest_fields);

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

static inline void load_shadow_vmcs(void) {
  cpu_vmptrld(vmcs0);
  uint8_t *vmcs_shadow = (uint8_t*)((cpu_vmread(VMCS_LINK_POINTER_HIGH) << 32) | cpu_vmread(VMCS_LINK_POINTER));
  if ((uint64_t)vmcs_shadow == (uint64_t)-1) {
    panic("VMCS_LINK_POINTER is not initialized !\n");
  }
  cpu_vmptrld(vmcs_shadow);
}