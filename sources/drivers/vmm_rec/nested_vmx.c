#include <efi.h>
#include <efilib.h>
#include "vmm.h"
#include "nested_vmx.h"
#include "vmcs.h"
#include "vmx.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "nested_vmx.h"
#ifdef _DEBUG_SERVER
#include "debug_server/debug_server.h"
#endif

uint8_t nested_state = NESTED_DISABLED;

static uint64_t fields_values[NB_VMCS_FIELDS];
#define READ_VMCS_FIELDS(fields) \
  read_vmcs_fields(fields, fields_values, sizeof(fields)/sizeof(uint64_t))
#define WRITE_VMCS_FIELDS(fields) \
  write_vmcs_fields(fields, fields_values, sizeof(fields)/sizeof(uint64_t))

static inline void read_vmcs_fields(uint64_t* fields, uint64_t* values, uint8_t nb_fields);
static inline void write_vmcs_fields(uint64_t* fields, uint64_t* values, uint8_t nb_fields);

#ifdef _VMCS_SHADOWING
uint8_t vmread_bitmap [4096] __attribute__((aligned(0x1000))) = {0};
uint8_t vmwrite_bitmap[4096] __attribute__((aligned(0x1000))) = {0};
#endif

enum shadow_err_code {
  SHADOW_OK,
  SHADOW_NO_SPACE,
  SHADOW_NOT_FOUND
};

/**
 * Gestion des VMCS GUEST
 */
uint8_t guest_vmcs[GVMCS_NB][4096] __attribute((aligned(0x1000)));
uint8_t *shadow_vmcs_ptr[GVMCS_NB];
static uint8_t guest_vmcs_idx = 0;
static uint32_t shadow_idx = 0;
// Current shadow VMCS pointer
static uint8_t* shadow_ptr = (uint8_t*)(uint64_t)-1;;

static int shadow_add(uint8_t *ptr, uint32_t *idx) {
  if (guest_vmcs_idx >= GVMCS_NB) {
    return SHADOW_NO_SPACE;
  }
  shadow_vmcs_ptr[guest_vmcs_idx] = ptr;
  if (idx != NULL) {
    *idx = guest_vmcs_idx;
  }
  guest_vmcs_idx++;
  return SHADOW_OK;
}

static int shadow_init(uint32_t idx) {
  // init guest VMCS
  memset((uint8_t *) &guest_vmcs[idx][0], 0, 0x1000);
  *(uint32_t*)&guest_vmcs[idx][0] = *(uint32_t*)&vmcs0[0];
  cpu_vmclear(&guest_vmcs[idx][0]);
  return SHADOW_OK;
}

static int shadow_get(uint8_t *ptr, uint32_t *idx) {
  uint32_t i;
  for (i = 0; i < guest_vmcs_idx; i++) {
    if (shadow_vmcs_ptr[i] == ptr) {
      if (idx != NULL) {
        *idx = i;
      }
      return SHADOW_OK;
    }
  }
  return SHADOW_NOT_FOUND;
}

static int shadow_set(uint8_t *ptr) {
  uint32_t idx;
  if (shadow_get(ptr, &idx) == SHADOW_NOT_FOUND) {
    if (!shadow_add(ptr, &idx)) {
      shadow_init(idx);
    } else {
      ERROR("No more space for VMCSs...\n");
    }
  }
  shadow_ptr = ptr;
  shadow_idx = idx;
  return SHADOW_OK;
}

/**
 * VMX Emulation
 */

static void set_vmcs_link_pointer(uint64_t shadow_vmcs) {
  // Set the VMCS link pointer for vmcs0
  cpu_vmwrite(VMCS_LINK_POINTER, shadow_vmcs & 0xffffffff);
  cpu_vmwrite(VMCS_LINK_POINTER_HIGH, (shadow_vmcs >> 32) & 0xffffffff);
  cpu_vmwrite(SECONDARY_VM_EXEC_CONTROL, cpu_vmread(SECONDARY_VM_EXEC_CONTROL) &
      ~(uint64_t)VMCS_SHADOWING);
  // Exit bitmap
  cpu_vmwrite(VMREAD_BITMAP_ADDR, (uint64_t)&vmread_bitmap & 0xffffffff);
  cpu_vmwrite(VMREAD_BITMAP_ADDR_HIGH, ((uint64_t)&vmread_bitmap >> 32) & 0xffffffff);
  cpu_vmwrite(VMWRITE_BITMAP_ADDR, (uint64_t)&vmwrite_bitmap & 0xffffffff);
  cpu_vmwrite(VMWRITE_BITMAP_ADDR_HIGH, ((uint64_t)&vmwrite_bitmap >> 32) & 0xffffffff);
}

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
  // set guest carry and zero flag to 0
  cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x21);
}

void nested_vmptrld(uint8_t *shadow_vmcs, struct registers *gr) {

  shadow_set(shadow_vmcs);

  if (shadow_vmcs == 0x0) {
    ERROR("Shadow VMCS pointer is null\n");
  }
  // set guest carry and zero flag to 0
  cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x21);

#ifdef _VMCS_SHADOWING
  set_vmcs_link_pointer((uint64_t)shadow_vmcs);
#endif
}

void nested_shadow_to_guest(void) {
  static uint64_t guest_fields[] = { NESTED_COPY_FROM_SHADOW };
  cpu_vmptrld(shadow_ptr);
  READ_VMCS_FIELDS(guest_fields);
  cpu_vmptrld(guest_vmcs[shadow_idx]);
  WRITE_VMCS_FIELDS(guest_fields);
}

void nested_cpu_vmresume(struct registers *guest_regs) {
  /* Correct rbp */
  __asm__ __volatile__("mov %0, %%rbp" : : "m" (guest_regs->rbp));
  /* Launch the vm */
  __asm__ __volatile__("vmresume;"
                       /* everything after should not be executed */
                       "setc %al;"
                       "setz %dl;"
                       "mov %eax, %edi;"
                       "mov %edx, %esi;"
                       "call vmx_transition_display_error");
}

void nested_cpu_vmlaunch(struct registers *guest_regs) {
  /* Correct rbp */
  __asm__ __volatile__("mov %0, %%rbp" : : "m" (guest_regs->rbp));
  /* Launch the vm */
  __asm__ __volatile__("vmlaunch;"
                       /* everything after should not be executed */
                       "setc %al;"
                       "setz %dl;"
                       "mov %eax, %edi;"
                       "mov %edx, %esi;"
                       "call vmx_transition_display_error");
}

void nested_vmlaunch(struct registers *guest_regs) {
//   static uint8_t guest_launched = 0;

  static uint64_t ctrl_host_fields[] = { NESTED_CTRL_FIELDS, NESTED_HOST_FIELDS };
  uint64_t preempt_timer_value = cpu_vmread(VMX_PREEMPTION_TIMER_VALUE);

  // copy ctrl fields + host fields from vmcs0 to guest_vmcs
  READ_VMCS_FIELDS(ctrl_host_fields);
  cpu_vmptrld(guest_vmcs[shadow_idx]);
  WRITE_VMCS_FIELDS(ctrl_host_fields);

  nested_shadow_to_guest();

  // Write the right VPID
  cpu_vmwrite(VIRTUAL_PROCESSOR_ID, shadow_idx + 2); // vmcs0 is 1

  // update vmx preemption timer
  cpu_vmwrite(VMX_PREEMPTION_TIMER_VALUE, preempt_timer_value);

  nested_state = NESTED_GUEST_RUNNING;

//   if (!guest_launched) {
//     guest_launched = 1;
    nested_cpu_vmlaunch(guest_regs);
//   } else {
//     nested_cpu_vmresume(guest_regs);
//   }
}

uint64_t nested_vmread(uint64_t field) {
  uint64_t value;

  // Reading ro_data fields is done in guest_vmcs instead of shadow_vmcs
  if (((field >> 10) & 0x3) == 0x1) { // ro_data fields
    cpu_vmptrld(guest_vmcs[shadow_idx]);
  } else {
    cpu_vmptrld(shadow_ptr);
  }

  value = cpu_vmread(field);

  cpu_vmptrld(vmcs0);

  // XXX We assume that everything went good : CF and ZF = 0
  cpu_vmwrite(GUEST_RFLAGS, cpu_vmread(GUEST_RFLAGS) & ~(uint64_t)0x21);

  return value;
}

void nested_vmwrite(uint64_t field, uint64_t value) {
  cpu_vmptrld(shadow_ptr);
  cpu_vmwrite(field, value);

//   // We forward the modification in guest_vmcs if needed
//   if ( (((field >> 10) & 0x3) == 0x2)     // all guest fields
//     || ( (((field >> 10) & 0x3) == 0x0)   // several ctrl fields
//       && ( (field == VM_ENTRY_CONTROLS)
//         || (field == VM_ENTRY_INTR_INFO_FIELD)
//         || (field == CR0_READ_SHADOW)
//         || (field == CR4_READ_SHADOW)
//         || (field == VIRTUAL_PROCESSOR_ID)))) {
//     cpu_vmptrld(guest_vmcs);
//     cpu_vmwrite(field, value);
//   }

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
  cpu_vmptrld(shadow_ptr);
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

  nested_shadow_to_guest();

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
